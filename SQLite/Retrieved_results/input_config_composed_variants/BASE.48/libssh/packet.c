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
static int macsize=SHA_DIGEST_LEN;

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

/* in nonblocking mode, socket_read will read as much as it can, and return */
/* SSH_OK if it has read at least len bytes, otherwise, SSH_AGAIN. */
/* in blocking mode, it will read at least len bytes and will block until it's ok. */

static int socket_read(SSH_SESSION *session,int len){
    int except, can_write;
    int ret;
    int to_read;
    int r;
    char *buf;
    char buffer[4096];
    if(!session->in_socket_buffer)
        session->in_socket_buffer=buffer_new();
    to_read=len - buffer_get_rest_len(session->in_socket_buffer);
    if(to_read <= 0)
        return 0;
    if(session->blocking){
        buf=malloc(to_read);
        ret=completeread(session->fd,buf,to_read);
        session->data_to_read=0;
        if(ret==-1 || ret ==0){
            ssh_set_error(session,2,
                (ret==0)?"Connection closed by remote host" : "Error reading socket");
            close(session->fd);
            session->fd=-1;
            session->data_except=1;
            return -1;
        }
            
        buffer_add_data(session->in_socket_buffer,buf,to_read);
        free(buf);
        return 0;
    /* nonblocking read */
    do {
        ssh_fd_poll(session,&can_write,&except); /* internally sets data_to_read */
        if(!session->data_to_read)
            return 1;
        session->data_to_read=0;
        /* read as much as we can */
        r=read(session->fd,buffer,sizeof(buffer));
        if(r<=0){
            ssh_set_error(session,2,
                (ret==0)?"Connection closed by remote host" : "Error reading socket");
    }
            close(session->fd);
            session->fd=-1;
            session->data_except=1;
        buffer_add_data(session->in_socket_buffer,buffer,r);
    } while(buffer_get_rest_len(session->in_socket_buffer)<len);
    return 0;
}

#define PACKET_STATE_INIT 0
#define PACKET_STATE_SIZEREAD 1

static int packet_read2(SSH_SESSION *session){
    u32 len;
    void *packet=NULL;
    unsigned char mac[30];
    char buffer[16];
    int be_read,i;
    int to_be_read;
    int ret;
    u8 padding;
    unsigned int blocksize=(session->current_crypto?session->current_crypto->in_cipher->blocksize:8);
    int current_macsize=session->current_crypto?macsize:0;
    if(!session->alive || session->data_except)
        return -1; // the error message was already set into this session
    switch(session->packet_state){
        case 0:    
            memset(&session->in_packet,0,sizeof(PACKET));
            if(session->in_buffer)
                buffer_reinit(session->in_buffer);
            else
                session->in_buffer=buffer_new();
            ret=socket_read(session,blocksize);
            if(ret != 0)
                return ret; // can be SSH_ERROR or SSH_AGAIN
//    be_read=completeread(session->fd,buffer,blocksize);
            memcpy(buffer,buffer_get_rest(session->in_socket_buffer),blocksize);
            buffer_pass_bytes(session->in_socket_buffer,blocksize); // mark them as read
            len=packet_decrypt_len(session,buffer);
            buffer_add_data(session->in_buffer,buffer,blocksize);
            if(len> 262144){
                ssh_set_error(session,2,"read_packet(): Packet len too high(%uld %.8lx)",len,len);
                return -1;
            }
            to_be_read=len-blocksize+sizeof(u32);
            if(to_be_read<0){
                /* remote sshd sends invalid sizes?*/
                ssh_set_error(session,2,"given numbers of bytes left to be read <0 (%d)!",to_be_read);
                return -1;
            }
            /* saves the status of the current operations */
            session->in_packet.len=len;
            session->packet_state=1;
        case 1:
            len=session->in_packet.len;
            to_be_read=len-blocksize+sizeof(u32) + current_macsize;
            /* if to_be_read is zero, the whole packet was blocksize bytes. */
            if(to_be_read != 0){ 
                ret=socket_read(session,to_be_read);
                if(ret!=0)
                    return ret;
                packet=malloc(to_be_read);
                memcpy(packet,buffer_get_rest(session->in_socket_buffer),to_be_read-current_macsize);
                buffer_pass_bytes(session->in_socket_buffer,to_be_read-current_macsize);
                ssh_say(3,"Read a %d bytes packet\n",len);
                buffer_add_data(session->in_buffer,packet,to_be_read-current_macsize);
                free(packet);
            }
            if(session->current_crypto){
                /* decrypt the rest of the packet (blocksize bytes already have been decrypted */
                packet_decrypt(session,buffer_get(session->in_buffer)+blocksize,buffer_get_len(session->in_buffer)-blocksize);
                memcpy(mac,buffer_get_rest(session->in_socket_buffer),macsize);
                buffer_pass_bytes(session->in_socket_buffer,macsize);
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
                return -1;
            }
            ssh_say(3,"%hhd bytes padding, %d bytes left in buffer\n",padding,buffer_get_rest_len(session->in_buffer));
            if(padding > buffer_get_rest_len(session->in_buffer)){
                ssh_set_error(session,2,"invalid padding: %d (%d resting)",padding,buffer_get_rest_len(session->in_buffer));

                

                return -1;
            }
            buffer_pass_bytes_end(session->in_buffer,padding);
            ssh_say(3,"After padding, %d bytes left in buffer\n",buffer_get_rest_len(session->in_buffer));


            session->recv_seq++;
            session->packet_state=0;
            return 0;
    }
    ssh_set_error(session,2,"Invalid state into packet_read2() : %d",session->packet_state);
    return -1;
}




    
    

    
    
    
    
    

    
    
        
    
    
        
    

    
    
        
            
            
            
            
            
            
            
        
        
        
    
    
    
    
    
        
        
    
    
    
    
    
   
    
        
        
        
            
            
            
            
            
            
        
        
            
            
            
            
            
        
        
        
        
    

    
            

    
        


        
    



    
    
        
        
        
    
    
            
    
    
    

        
                

        
        
                
                
        
    
    
    







    
    




/* that's where i'd like C to be object ... */
int packet_read(SSH_SESSION *session){

    
        
    

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
    do {
        written=write(fd,buffer,len);
        if(written==0 || written==-1)
            return -1;
        len-=written;
        buffer+=written;
    } while (len > 0);
    return 0;
}

int packet_nonblocking_flush(SSH_SESSION *session){
    int except, can_write;
    int w;
    ssh_fd_poll(session,&can_write,&except); /* internally sets data_to_write */
    while(session->data_to_write && buffer_get_rest_len(session->out_socket_buffer)>0){
        w=write(session->fd,buffer_get_rest(session->out_socket_buffer),
                buffer_get_rest_len(session->out_socket_buffer));
        if(w<0){
            session->data_to_write=0;
            session->data_except=1;
            session->alive=0;
            close(session->fd);
            session->fd=-1;
            ssh_set_error(session,2,"Writing packet : error on socket (or connection closed): %s",
                          strerror(errno));
            return -1;
        }
        buffer_pass_bytes(session->out_socket_buffer,w);
    }
    if(buffer_get_rest_len(session->out_socket_buffer)>0)
        return 1;  /* there is data pending */
    return 0; // all data written
}

int packet_flush(SSH_SESSION *session){
    if(atomic_write(session->fd,buffer_get_rest(session->out_socket_buffer),
       buffer_get_rest_len(session->out_socket_buffer))){
        session->data_to_write=0;
        session->data_except=1;
        session->alive=0;
        close(session->fd);
        session->fd=-1;
        ssh_set_error(session,2,"Writing packet : error on socket (or connection closed): %s",
                         strerror(errno));
        return -1;
    }
    session->data_to_write=0;
    buffer_reinit(session->out_socket_buffer);
    return 0; // no data pending
}

/* this function places the outgoing packet buffer into an outgoing socket buffer */
static int socket_write(SSH_SESSION *session){
    if(!session->out_socket_buffer){
        session->out_socket_buffer=buffer_new();
    }
    buffer_add_data(session->out_socket_buffer,
               buffer_get(session->out_buffer),buffer_get_len(session->out_buffer));
    if(!session->blocking){
        return packet_nonblocking_flush(session);
    } else
        return packet_flush(session);
}

static int packet_send2(SSH_SESSION *session){
    char padstring[32];
    u32 finallen;
    u8 padding;
    u32 currentlen=buffer_get_len(session->out_buffer);
    unsigned char *hmac;
    int ret=0;
    unsigned int blocksize=(session->current_crypto?session->current_crypto->out_cipher->blocksize:8);
    ssh_say(3,"Writing on the wire a packet having %ld bytes before",currentlen);

    
        
        
        
    

    padding=(blocksize- ((currentlen+5) % blocksize));
    if(padding<4)
        padding+=blocksize;
    if(session->current_crypto)
        ssh_get_random(padstring,padding,0);
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
    ret=socket_write(session);
    session->send_seq++;
    buffer_reinit(session->out_buffer);
    return ret; /* SSH_OK, AGAIN or ERROR */
}



    
    
    
    
    
    
    
    








    
    
        
    
        
    
    
    
    
    
    

    
            

    

    
            

    
    
    
    




int packet_send(SSH_SESSION *session){

    
        
    

        return packet_send2(session);
}

void packet_parse(SSH_SESSION *session){
    int type=session->in_packet.type;
    u32 foo;
    STRING *error_s;
    char *error=NULL;

    
        
        
            
                
                
                
                
                
                
            
            
            
                
                
            
                
            
        
    

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

    
        
    

        return packet_wait2(session,type,block);
}


void packet_clear_out(SSH_SESSION *session){
    if(session->out_buffer)
        buffer_reinit(session->out_buffer);
    else
        session->out_buffer=buffer_new();
}

