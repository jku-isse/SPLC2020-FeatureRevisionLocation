/* packet.c */	
/* packet building functions */
/*
Copyright 2003 Aris Adamantiadis

This file is part of the SSH Library

The SSH Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The SSH Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the SSH Library; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "libssh/priv.h"
#include "libssh/ssh2.h"
#include "libssh/ssh1.h"
#include <netdb.h>
#include <errno.h>
#include "libssh/crypto.h"

/* XXX include selected mac size */
static int macsize=SHA_DIGEST_LENGTH;

/* completeread will read blocking until len bytes have been read */
static int completeread(int fd, void *buffer, int len){
    int r;
    int total=0;
    int toread=len;
    while((r=read(fd,buffer+total,toread))){
        if(r==-1)
            return -1;
        total += r;
        toread-=r;
        if(total==len)
            return len;
        if(r==0)
            return 0;
    }
    return total ; /* connection closed */
}

static int packet_read2(SSH_SESSION *session){
    u32 len;
    void *packet=NULL;
    char mac[30];
    char buffer[16];
    int be_read,i;
    int to_be_read;
    u8 padding;
    unsigned int blocksize=(session->current_crypto?session->current_crypto->in_cipher->blocksize:8);
    session->data_to_read=0; /* clear the dataavailable flag */
    memset(&session->in_packet,0,sizeof(PACKET));
    if(session->in_buffer)
        buffer_free(session->in_buffer);
    session->in_buffer=buffer_new();

    be_read=completeread(session->fd,buffer,blocksize);
    if(be_read!=blocksize){
        if(be_read<=0){
            session->alive=0;
            close(session->fd);
            session->fd=-1;
            ssh_set_error(session,2,
            (be_read==0)?"Connection closed by remote host" : "Error reading socket");
            return -1;
        }
        ssh_set_error(session,2,"read_packet(): asked %d bytes, received %d",blocksize,be_read);
        return -1;
    }
    len=packet_decrypt_len(session,buffer);
    buffer_add_data(session->in_buffer,buffer,blocksize);
    
    if(len> 262144){
        ssh_set_error(session,2,"read_packet(): Packet len too high(%uld %.8lx)",len,len);
        return -1;
    }
    to_be_read=len-be_read+sizeof(u32);
    if(to_be_read<0){
        /* remote sshd is trying to get me ?*/
        ssh_set_error(session,2,"given numbers of bytes left to be read <0 (%d)!",to_be_read);
        return -1;
    }
    /* handle the case in which the whole packet size = blocksize */
    if(to_be_read !=0){
        packet=malloc(to_be_read);
        i=completeread(session->fd,packet,to_be_read);
        if(i<=0){
            session->alive=0;
            close(session->fd);
            session->fd=-1;
            ssh_set_error(session,2,"Server closed connection");
            return -1;
        }
        if(i!=to_be_read){
            free(packet);
            packet=NULL;
            ssh_say(3,"Read only %d, wanted %d\n",i,to_be_read);
            ssh_set_error(session,2,"read_packet(): read only %d, wanted %d",i,to_be_read);
            return -1;
        }
        ssh_say(3,"Read a %d bytes packet\n",len);
        buffer_add_data(session->in_buffer,packet,to_be_read);
        free(packet);
    }
    if(session->current_crypto){
        packet_decrypt(session,buffer_get(session->in_buffer)+blocksize,buffer_get_len(session->in_buffer)-blocksize);
        if((i=completeread(session->fd,mac,macsize))!=macsize){
            if(i<=0){
                session->alive=0;
                close(session->fd);
                session->fd=-1;
                ssh_set_error(session,2,"Server closed connection");
                return -1;
            }
            ssh_set_error(session,2,"read_packet(): wanted %d, had %d",i,macsize);
            return -1;
        }
        if(packet_hmac_verify(session,session->in_buffer,mac)){
            ssh_set_error(session,2,"HMAC error");
            return -1;
        }
    }
    buffer_pass_bytes(session->in_buffer,sizeof(u32));   /*pass the size which has been processed before*/
    if(!buffer_get_u8(session->in_buffer,&padding)){
        ssh_set_error(session,2,"Packet too short to read padding");
        return -1;
    }
    ssh_say(3,"%hhd bytes padding\n",padding);
    if(padding > buffer_get_rest_len(session->in_buffer)){
        ssh_set_error(session,2,"invalid padding: %d (%d resting)",padding,buffer_get_rest_len(session->in_buffer));

        

        return -1;
    }
    buffer_pass_bytes_end(session->in_buffer,padding);

    
        
    

    session->recv_seq++;
    return 0;
}


/* a slighty modified packet_read2() for SSH-1 protocol */
static int packet_read1(SSH_SESSION *session){
    u32 len;
    void *packet=NULL;
//    char buffer[16];
    int be_read,i;
    int to_be_read;
    u32 padding;
    u32 crc;
    ssh_say(3,"packet_read1()\n");
//    unsigned int blocksize=8;
    session->data_to_read=0; /* clear the dataavailable flag */
    memset(&session->in_packet,0,sizeof(PACKET));
    if(session->in_buffer)
        buffer_free(session->in_buffer);
    session->in_buffer=buffer_new();

    be_read=completeread(session->fd,&len,sizeof(u32));
    if(be_read!=sizeof(u32)){
        if(be_read<=0){
            session->alive=0;
            close(session->fd);
            session->fd=-1;
            ssh_set_error(session,2,
            (be_read==0)?"Connection closed by remote host" : "Error reading socket");
            return -1;
        }
        ssh_set_error(session,2,"read_packet(): asked %d bytes, received %d",sizeof(u32),be_read);
        return -1;
    }
    /* len is not encrypted */
    len=ntohl(len);
    
    if(len> 262144){
        ssh_set_error(session,2,"read_packet(): Packet len too high(%uld %.8lx)",len,len);
        return -1;
    }
    ssh_say(3,"%d bytes packet\n",len);
    /* SSH-1 has a fixed padding lenght */
    padding=8-(len % 8);
    to_be_read=len+padding;
   /* handle the case in which the whole packet size = blocksize */
    if(to_be_read !=0){
        packet=malloc(to_be_read);
        i=completeread(session->fd,packet,to_be_read);
        if(i<=0){
            session->alive=0;
            close(session->fd);
            session->fd=-1;
            ssh_set_error(session,2,"Server closed connection");
            return -1;
        }
        if(i!=to_be_read){
            free(packet);
            packet=NULL;
            ssh_say(3,"Read only %d, wanted %d\n",i,to_be_read);
            ssh_set_error(session,2,"read_packet(): read only %d, wanted %d",i,to_be_read);
            return -1;
        }
        ssh_say(3,"Read a %d bytes packet\n",len);
        buffer_add_data(session->in_buffer,packet,to_be_read);
        free(packet);
    }

    
            

    if(session->current_crypto){
        /* we decrypt everything, missing the lenght part (which was previously
         * read, unencrypted, and is not part of the buffer
         */
        packet_decrypt(session,buffer_get(session->in_buffer),buffer_get_len(session->in_buffer));
    }



    ssh_say(3,"%d bytes padding\n",padding);
    if((len+padding) != buffer_get_rest_len(session->in_buffer) || (len+padding) < sizeof(u32)){
        ssh_say(2,"no crc32 in packet\n");
        ssh_set_error(session,2,"no crc32 in packet");
        return -1;
    }
    memcpy(&crc,buffer_get_rest(session->in_buffer)+(len+padding)-sizeof(u32),
            sizeof(u32));
    buffer_pass_bytes_end(session->in_buffer,sizeof(u32));
    crc=ntohl(crc);
    if(ssh_crc32(buffer_get_rest(session->in_buffer),(len+padding)-sizeof(u32))!=crc){

        
                

        ssh_say(2,"invalid crc32\n");
        ssh_set_error(session,2,"invalid crc32 : expected %.8lx, "
                "got %.8lx",crc,
                ssh_crc32(buffer_get_rest(session->in_buffer),len+padding-sizeof(u32)) );
        return -1;
    }
    buffer_pass_bytes(session->in_buffer,padding);   /*pass the padding*/
    ssh_say(3,"the packet is valid\n");
/* will do that later 
#ifdef HAVE_LIBZ
    if(session->current_crypto && session->current_crypto->do_compress_in){
        decompress_buffer(session,session->in_buffer);
    }
#endif
*/
    session->recv_seq++;
    return 0;
}



/* that's where i'd like C to be object ... */
int packet_read(SSH_SESSION *session){

    if(session->version==1)
        return packet_read1(session);
    else

        return packet_read2(session);
}

int packet_translate(SSH_SESSION *session){
    memset(&session->in_packet,0,sizeof(PACKET));
    if(!session->in_buffer)
        return -1;
    ssh_say(3,"Final size %d\n",buffer_get_rest_len(session->in_buffer));
    if(!buffer_get_u8(session->in_buffer,&session->in_packet.type)){
        ssh_set_error(session,2,"Packet too short to read type");
        return -1;
    }
    ssh_say(3,"type %hhd\n",session->in_packet.type);
    session->in_packet.valid=1;
    return 0;
}

static int atomic_write(int fd, void *buffer, int len){
    int written;
    int total=0;
    do {
        written=write(fd,buffer,len);
        if(written==0)
            return 0;
        if(written==-1)
            return total;
        total+=written;
        len-=written;
        buffer+=written;
    } while (len > 0);
    return total;
}

static int packet_send2(SSH_SESSION *session){
    char padstring[32];
    u32 finallen;
    u8 padding;
    u32 currentlen=buffer_get_len(session->out_buffer);
    char *hmac;
    int ret=0;
    unsigned int blocksize=(session->current_crypto?session->current_crypto->out_cipher->blocksize:8);
    ssh_say(3,"Writing on the wire a packet having %ld bytes before",currentlen);

    
        
        
    

    padding=(blocksize- ((currentlen+5) % blocksize));
    if(padding<4)
        padding+=blocksize;
    if(session->current_crypto)
        ssh_get_random(padstring,padding);
    else
        memset(padstring,0,padding);
    finallen=htonl(currentlen+padding+1);
    ssh_say(3,",%d bytes after comp + %d padding bytes = %d bytes packet\n",currentlen,padding,(ntohl(finallen)));
    buffer_add_data_begin(session->out_buffer,&padding,sizeof(u8));
    buffer_add_data_begin(session->out_buffer,&finallen,sizeof(u32));
    buffer_add_data(session->out_buffer,padstring,padding);
    hmac=packet_encrypt(session,buffer_get(session->out_buffer),buffer_get_len(session->out_buffer));
    if(hmac)
        buffer_add_data(session->out_buffer,hmac,20);
    if(atomic_write(session->fd,buffer_get(session->out_buffer),buffer_get_len(session->out_buffer))!=buffer_get_len(session->out_buffer)){
        session->alive=0;
        close(session->fd);
        session->fd=-1;
        ssh_set_error(session,2,"Writing packet : error on socket (or connection closed): %s",
            strerror(errno));
        ret=-1;
    }
    session->send_seq++;
    buffer_reinit(session->out_buffer);
    return ret;
}


static int packet_send1(SSH_SESSION *session){
    char padstring[32];
    u32 finallen;
    u8 padding;
    u32 crc;
    u32 currentlen=buffer_get_len(session->out_buffer)+sizeof(u32);
    int ret=0;
    unsigned int blocksize=(session->current_crypto?session->current_crypto->out_cipher->blocksize:8);
    ssh_say(3,"Writing on the wire a packet having %ld bytes before",currentlen);
/*
#ifdef HAVE_LIBZ
    if(session->current_crypto && session->current_crypto->do_compress_out){
        compress_buffer(session,session->out_buffer);
        currentlen=buffer_get_len(session->out_buffer);
    }
#endif
*/
    padding=blocksize-(currentlen % blocksize);
    if(session->current_crypto)
        ssh_get_random(padstring,padding);
    else
        memset(padstring,0,padding);
    finallen=htonl(currentlen);
    ssh_say(3,",%d bytes after comp + %d padding bytes = %d bytes packet\n",currentlen,padding,(ntohl(finallen)));
    buffer_add_data_begin(session->out_buffer,&padstring,padding);
    buffer_add_data_begin(session->out_buffer,&finallen,sizeof(u32));
    crc=ssh_crc32(buffer_get(session->out_buffer)+sizeof(u32),buffer_get_len(session->out_buffer)-sizeof(u32));
    buffer_add_u32(session->out_buffer,ntohl(crc));

    
            

    packet_encrypt(session,buffer_get(session->out_buffer)+sizeof(u32),buffer_get_len(session->out_buffer)-sizeof(u32));

    
            

    if(atomic_write(session->fd,buffer_get(session->out_buffer),buffer_get_len(session->out_buffer))!=buffer_get_len(session->out_buffer)){
        session->alive=0;
        close(session->fd);
        session->fd=-1;
        ssh_set_error(session,2,"Writing packet : error on socket (or connection closed): %s",
            strerror(errno));
        ret=-1;
    }
    session->send_seq++;
    buffer_reinit(session->out_buffer);
    return ret;
}



int packet_send(SSH_SESSION *session){

    if (session->version==1)
        return packet_send1(session);
    else 

        return packet_send2(session);
}

void packet_parse(SSH_SESSION *session){
    int type=session->in_packet.type;
    u32 foo;
    STRING *error_s;
    char *error=NULL;

    if(session->version==1){
        /* SSH-1 */
        switch(type){
            case 1:
                ssh_say(2,"Received SSH_MSG_DISCONNECT\n");
                ssh_set_error(session,2,"Received SSH_MSG_DISCONNECT");
                close(session->fd);
                session->fd=-1;
                session->alive=0;
                return;
            case 17:
            case 18:
            case 20:
                channel_handle1(session,type);
                return;
            default:
                ssh_say(2,"Unexpected message code %d\n",type);
            }
        return;
    } else {

    switch(type){
        case 1:
            buffer_get_u32(session->in_buffer,&foo);
            error_s=buffer_get_ssh_string(session->in_buffer);
            if(error_s)
                error=string_to_char(error_s);
            ssh_say(2,"Received SSH_MSG_DISCONNECT\n");
            ssh_set_error(session,2,"Received SSH_MSG_DISCONNECT : %s",error);
            if(error_s){
                free(error_s);
                free(error);
            }
            close(session->fd);
            session->fd=-1;
            session->alive=0;
            return;
        case 93:
        case 94:
        case 95:
        case 98:
        case 96:
        case 97:

            channel_handle(session,type);
        case 2:
            return;
        default:
            ssh_say(0,"Received unhandled msg %d\n",type);
    }

    }

}    


static int packet_wait1(SSH_SESSION *session,int type,int blocking){
    ssh_say(3,"packet_wait1 waiting for %d\n",type);
    while(1){
        if(packet_read1(session))
            return -1;
        if(packet_translate(session))
            return -1;
        ssh_say(3,"packet_wait 1 received %d\n",session->in_packet.type);
        switch(session->in_packet.type){
            case 1:
                packet_parse(session);
                return -1;
            case 17:
            case 18:
            case 20:
                channel_handle1(session,type);
                break;
/*          case SSH2_MSG_CHANNEL_CLOSE:
               packet_parse(session);
                break;;
           case SSH2_MSG_IGNORE:
               break;
*/               
            default:
               if(type && (type != session->in_packet.type)){
                   ssh_set_error(session,2,"waitpacket(): Received a %d type packet, was waiting for a %d\n",session->in_packet.type,type);
                   return -1;
               }
               return 0;
           }
        if(blocking==0)
            return 0;
    }
    return 0;
}

static int packet_wait2(SSH_SESSION *session,int type,int blocking){
    while(1){
        if(packet_read2(session))
            return -1;
        if(packet_translate(session))
            return -1;
        switch(session->in_packet.type){
           case 1:
               packet_parse(session);
                return -1;
           case 93:
           case 94:
           case 95:
           case 98:
           case 96:
           case 97:
               packet_parse(session);
                break;;
           case 2:
               break;
           default:
               if(type && (type != session->in_packet.type)){
                   ssh_set_error(session,2,"waitpacket(): Received a %d type packet, was waiting for a %d\n",session->in_packet.type,type);
                   return -1;
               }
               return 0;
           }
        if(blocking==0)
            return 0;
    }
    return 0;
}
int packet_wait(SSH_SESSION *session, int type, int block){

    if(session->version==1)
        return packet_wait1(session,type,block);
    else

        return packet_wait2(session,type,block);
}


void packet_clear_out(SSH_SESSION *session){
    if(session->out_buffer)
        buffer_reinit(session->out_buffer);
    else
        session->out_buffer=buffer_new();
}

