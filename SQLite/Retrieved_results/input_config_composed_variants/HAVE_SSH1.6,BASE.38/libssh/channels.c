/* channels.c */
/* It has support for ... ssh channels */
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

#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

#include "libssh/priv.h"
#include "libssh/ssh2.h"
#define WINDOWLIMIT 1024
#define WINDOWBASE 32000

CHANNEL *channel_new(SSH_SESSION *session){
    CHANNEL *channel=malloc(sizeof(CHANNEL));
    memset(channel,0,sizeof(CHANNEL));
    channel->session=session;
    channel->version=session->version;
    channel->stdout_buffer=buffer_new();
    channel->stderr_buffer=buffer_new();
     if(!session->channels){
        session->channels=channel;
        channel->next=channel->prev=channel;
        return channel;
        }
    channel->next=session->channels;
    channel->prev=session->channels->prev;
    channel->next->prev=channel;
    channel->prev->next=channel;
    return channel;
}

u32 ssh_channel_new_id(SSH_SESSION *session){
    u32 ret=session->maxchannel;
    session->maxchannel++;
    return ret;
}

static int channel_open(CHANNEL *channel,char *type_c,int window,
        int maxpacket,BUFFER *payload){
    SSH_SESSION *session=channel->session;
    STRING *type=string_from_char(type_c);
    u32 foo;
    int err;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,90);
    channel->local_channel=ssh_channel_new_id(session);
    channel->local_maxpacket=maxpacket;
    channel->local_window=window;
    ssh_say(2,"creating a channel %d with %d window and %d max packet\n",
            channel->local_channel, window,maxpacket);
    buffer_add_ssh_string(session->out_buffer,type);
    buffer_add_u32(session->out_buffer,htonl(channel->local_channel));
    buffer_add_u32(session->out_buffer,htonl(channel->local_window));
    buffer_add_u32(session->out_buffer,htonl(channel->local_maxpacket));
    free(type);
    if(payload)
        buffer_add_buffer(session->out_buffer,payload);
    packet_send(session);
    ssh_say(2,"Sent a SSH_MSG_CHANNEL_OPEN type %s for channel %d\n",type_c,channel->local_channel);
    err=packet_wait(session,91,1);
    switch(session->in_packet.type){
        case 91:
            buffer_get_u32(session->in_buffer,&foo);
            if(channel->local_channel!=ntohl(foo)){
                ssh_set_error(session,2,"server answered with sender chan num %d instead of given %d",
                ntohl(foo),channel->local_channel);
                return -1;
                }
            buffer_get_u32(session->in_buffer,&foo);
            channel->remote_channel=ntohl(foo);
            buffer_get_u32(session->in_buffer,&foo);
            channel->remote_window=ntohl(foo);
            buffer_get_u32(session->in_buffer,&foo);
            channel->remote_maxpacket=ntohl(foo);
            ssh_say(3,"Received a CHANNEL_OPEN_CONFIRMATION for channel %d:%d\n"
                    ,channel->local_channel,channel->remote_channel);
            ssh_say(3,"Remote window : %ld, maxpacket : %ld\n",
	        channel->remote_window, channel->remote_maxpacket);
	        channel->open=1;
            return 0;
        case 92:
            {
                u32 code;
                STRING *error_s;
                char *error;
                buffer_get_u32(session->in_buffer,&foo);
                buffer_get_u32(session->in_buffer,&code);
                error_s=buffer_get_ssh_string(session->in_buffer);
                error=string_to_char(error_s);
                ssh_set_error(session,1,"Channel opening failure : channel %d error (%d) %s",
                    channel->local_channel,ntohl(code),error);
                free(error);
                free(error_s);
                return -1;
            }
        default:
            ssh_set_error(session,2,"Received unknown packet %d\n",session->in_packet.type);
            return -1;
        }
    return -1;
}

CHANNEL *ssh_channel_from_local(SSH_SESSION *session,u32 num){
    // we assume we are always the local
    CHANNEL *initchan,*channel;
    initchan=session->channels;
    if(!initchan)
        return NULL;
    for(channel=initchan;channel->local_channel!=num;channel=channel->next){
        if(channel->next==initchan)
            return NULL;
    }
    return channel;
}

static void grow_window(SSH_SESSION *session, CHANNEL *channel){
    u32 new_window=32000;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,93);
    buffer_add_u32(session->out_buffer,htonl(channel->remote_channel));
    buffer_add_u32(session->out_buffer,htonl(new_window));
    packet_send(session);
    ssh_say(3,"growing window (channel %d:%d) to %d bytes\n",
        channel->local_channel,channel->remote_channel,
        channel->local_window + new_window);
    channel->local_window+=new_window;
}

static CHANNEL *channel_from_msg(SSH_SESSION *session){
    u32 chan;
    CHANNEL *channel;
    if (buffer_get_u32(session->in_buffer,&chan)!=sizeof(u32)){
        ssh_set_error(session,2,"Getting channel from message : short read");
        return NULL;
    }
    channel=ssh_channel_from_local(session,ntohl(chan));
    if(!channel)
        ssh_set_error(session,2,"Server specified invalid channel %d",ntohl(chan));
    return channel;
}

static void channel_rcv_change_window(SSH_SESSION *session){
    u32 bytes;
    CHANNEL *channel;
    int err;
    channel=channel_from_msg(session);
    if(!channel)
        ssh_say(0,"%s\n",ssh_get_error(session));
    err = buffer_get_u32(session->in_buffer,&bytes);
    if(!channel || err!= sizeof(u32)){
        ssh_say(1,"Error getting a window adjust message : invalid packet\n");
        return;
    }
    bytes=ntohl(bytes);
    ssh_say(3,"Adding %d bytes to channel (%d:%d) (from %d bytes)\n",bytes,
        channel->local_channel,channel->remote_channel,channel->remote_window);
    channel->remote_window+=bytes;
}

/* is_stderr is set to 1 if the data are extended, ie stderr */
static void channel_rcv_data(SSH_SESSION *session,int is_stderr){
    STRING *str;
    CHANNEL *channel;
    channel=channel_from_msg(session);
    if(!channel){
        ssh_say(0,"%s",ssh_get_error(session));
        return;
    }
    if(is_stderr){
        u32 ignore;
        /* uint32 data type code. we can ignore it */
        buffer_get_u32(session->in_buffer,&ignore);
    }
    str=buffer_get_ssh_string(session->in_buffer);

    if(!str){
        ssh_say(0,"Invalid data packet !\n");
        return;
    }
    ssh_say(3,"adding %d bytes data in %d\n",string_len(str),is_stderr);
    /* what shall we do in this case ? let's accept it anyway */
    if(string_len(str)>channel->local_window)
        ssh_say(0,"Data packet too big for our window(%d vs %d)",string_len(str),channel->local_window);
    channel_default_bufferize(channel,str->string,string_len(str), is_stderr);
    if(string_len(str)>=channel->local_window)
        channel->local_window-=string_len(str);
    else
        channel->local_window=0; /* buggy remote */
    if(channel->local_window < 1024)
        grow_window(session,channel); /* i wonder if this is the correct place to do that */
    free(str);
}

static void channel_rcv_eof(SSH_SESSION *session){
    CHANNEL *channel;
    channel=channel_from_msg(session);
    if(!channel){
        ssh_say(0,"%s\n",ssh_get_error(session));
        return;
    }
    ssh_say(2,"Received eof on channel (%d:%d)\n",channel->local_channel,
        channel->remote_channel);
//    channel->remote_window=0;
    channel->remote_eof=1;
}

static void channel_rcv_close(SSH_SESSION *session){
    CHANNEL *channel;
    channel=channel_from_msg(session);
    if(!channel){
        ssh_say(0,"%s\n",ssh_get_error(session));
        return;
    }
    ssh_say(2,"Received close on channel (%d:%d)\n",channel->local_channel,
        channel->remote_channel);
    if((channel->stdout_buffer && buffer_get_rest_len(channel->stdout_buffer)>0)
            || (channel->stderr_buffer && buffer_get_rest_len(channel->stderr_buffer)>0))
        channel->delayed_close=1;
    else
        channel->open=0;
    if(!channel->remote_eof)
        ssh_say(2,"Remote host not polite enough to send an eof before close\n");
    channel->remote_eof=1;
    /* the remote eof doesn't break things if there was still data into read
     * buffer because the eof is ignored until the buffer is empty. */
}

static void channel_rcv_request(SSH_SESSION *session){
    STRING *request_s;
    char *request;
    u32 status;
    CHANNEL *channel=channel_from_msg(session);
    if(!channel){
        ssh_say(1,"%s\n",ssh_get_error(session));
        return;
    }
    request_s=buffer_get_ssh_string(session->in_buffer);
    if(!request_s){
        ssh_say(0,"Invalid MSG_CHANNEL_REQUEST\n");
        return;
    }
    buffer_get_u8(session->in_buffer,(u8 *)&status);
    request=string_to_char(request_s);
    if(!strcmp(request,"exit-status")){
        buffer_get_u32(session->in_buffer,&status);
        status=ntohl(status);
/* XXX do something with status, we might need it */
        free(request_s);
        free(request);
        return ;
    }
    if(!strcmp(request,"exit-signal")){
        STRING *signal_s;
        char *signal;
        char *core="(core dumped)";
        u8 i;
        signal_s=buffer_get_ssh_string(session->in_buffer);
        if(!signal_s){
            ssh_say(0,"Invalid MSG_CHANNEL_REQUEST\n");
            free(request_s);
            free(request);
            return;
        }
        signal=string_to_char(signal_s);
        buffer_get_u8(session->in_buffer,&i);
        if(!i)
            core="";
        ssh_say(0,"Remote connection closed by signal SIG%s %s\n",signal,core);
        free(signal_s);
        free(signal);
        free(request_s);
        free(request);
        return;
    }
    ssh_say(0,"Unknown request %s\n",request);
    free(request_s);
    free(request);
}

/* channel_handle is called by wait_packet, ie, when there is channel informations to handle . */
void channel_handle(SSH_SESSION *session, int type){
    ssh_say(3,"Channel_handle(%d)\n",type);
    switch(type){
        case 93:
            channel_rcv_change_window(session);
            break;
        case 94:
            channel_rcv_data(session,0);
            break;
        case 95:
            channel_rcv_data(session,1);
            break;
        case 96:
            channel_rcv_eof(session);
            break;
        case 97:
            channel_rcv_close(session);
            break;
        case 98:
            channel_rcv_request(session);
            break;
        default:
            ssh_say(0,"Unexpected message %d\n",type);
        }
}

/* when data has been received from the ssh server, it can be applied to the known
    user function, with help of the callback, or inserted here */
/* XXX is the window changed ? */
void channel_default_bufferize(CHANNEL *channel, void *data, int len, int is_stderr){
    ssh_say(3,"placing %d bytes into channel buffer (stderr=%d)\n",len,is_stderr);
    if(!is_stderr){
        /* stdout */
        if(!channel->stdout_buffer)
            channel->stdout_buffer=buffer_new();
        buffer_add_data(channel->stdout_buffer,data,len);
    } else {
        /* stderr */
        if(!channel->stderr_buffer)
            channel->stderr_buffer=buffer_new();
        buffer_add_data(channel->stderr_buffer,data,len);
    }
}

int channel_open_session(CHANNEL *channel){

    if(channel->session->version==2)

        return channel_open(channel,"session",64000,32000,NULL);

    else
        return channel_open_session1(channel);

}    

/* tcpip forwarding */

int channel_open_forward(CHANNEL *channel,char *remotehost, int remoteport, char *sourcehost, int localport){
    BUFFER *payload=buffer_new();
    STRING *str=string_from_char(remotehost);
    int ret;
    buffer_add_ssh_string(payload,str);
    free(str);
    str=string_from_char(sourcehost);
    buffer_add_u32(payload,htonl(remoteport));
    buffer_add_ssh_string(payload,str);
    free(str);
    buffer_add_u32(payload,htonl(localport));
    ret=channel_open(channel,"direct-tcpip",64000,32000,payload);
    buffer_free(payload);
    return ret;
}


void channel_free(CHANNEL *channel){
    SSH_SESSION *session=channel->session;
    if(session->alive && channel->open)
        channel_close(channel);
    /* handle the "my channel is first on session list" case */
    if(session->channels==channel)
        session->channels=channel->next;
    /* handle the "my channel is the only on session list" case */
    if(channel->next == channel){
        session->channels=NULL;
    } else {
        channel->prev->next=channel->next;
        channel->next->prev=channel->prev;
    }
    if(channel->stdout_buffer)
        buffer_free(channel->stdout_buffer);
    if(channel->stderr_buffer)
        buffer_free(channel->stderr_buffer);
    /* debug trick to catch use after frees */
    memset(channel,'X',sizeof(CHANNEL));
    free(channel);
}

int channel_send_eof(CHANNEL *channel){
    SSH_SESSION *session=channel->session;
    int ret;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,96);
    buffer_add_u32(session->out_buffer,htonl(channel->remote_channel));
    ret=packet_send(session);
    ssh_say(1,"Sent a EOF on client channel (%d:%d)\n",channel->local_channel,
        channel->remote_channel);
    channel->local_eof=1;
    return ret;
}

int channel_close(CHANNEL *channel){
    SSH_SESSION *session=channel->session;
    int ret=0;
    if(!channel->local_eof)
        ret=channel_send_eof(channel);
    if(ret)
        return ret;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,97);
    buffer_add_u32(session->out_buffer,htonl(channel->remote_channel));
    ret=packet_send(session);
    ssh_say(1,"Sent a close on client channel (%d:%d)\n",channel->local_channel,
        channel->remote_channel);
    if(!ret)
        channel->open =0;
    return ret;
}

/* Blocking write */
/* The exact len is written */
int channel_write(CHANNEL *channel ,void *data,int len){
    SSH_SESSION *session=channel->session;
    int effectivelen;
    int origlen=len;
    if(channel->local_eof){
        ssh_set_error(session,1,"Can't write to channel %d:%d"
            " after EOF was sent",channel->local_channel,channel->remote_channel);
        return -1;
    }
    if(!channel->open || channel->delayed_close){
        ssh_set_error(session,1,"Remote channel is closed");
        return -1;
    }

    if(channel->version==1)
        return channel_write1(channel,data,len);

    while(len >0){
        if(channel->remote_window<len){
	    ssh_say(2,"Remote window is %d bytes. going to write %d bytes\n",
	    channel->remote_window,len);
	    ssh_say(2,"Waiting for a growing window message...\n");
            // wonder what happens when the channel window is zero
            while(channel->remote_window==0){
                // parse every incoming packet
                packet_wait(channel->session,0,0);
            }
            effectivelen=len>channel->remote_window?channel->remote_window:len;
        } else
            effectivelen=len;
        packet_clear_out(session);
        buffer_add_u8(session->out_buffer,94);
        buffer_add_u32(session->out_buffer,htonl(channel->remote_channel));
        buffer_add_u32(session->out_buffer,htonl(effectivelen));
        buffer_add_data(session->out_buffer,data,effectivelen);
        packet_send(session);
        ssh_say(2,"channel_write wrote %d bytes\n",effectivelen);
        channel->remote_window-=effectivelen;
        len -= effectivelen;
        data+=effectivelen;
    }
    return origlen;
}

int channel_is_open(CHANNEL *channel){
    return (channel->open!=0);
}

int channel_is_eof(CHANNEL *channel){
    if((channel->stdout_buffer && buffer_get_rest_len(channel->stdout_buffer)
                >0) || (channel->stderr_buffer && buffer_get_rest_len(
                    channel->stderr_buffer)>0))
        return 0;
    return (channel->remote_eof!=0);
}

void channel_set_blocking(CHANNEL *channel, int blocking){
    channel->blocking=blocking;
}

static int channel_request(CHANNEL *channel,char *request, BUFFER *buffer,int reply){
    STRING *request_s=string_from_char(request);
    SSH_SESSION *session=channel->session;
    int err;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,98);
    buffer_add_u32(session->out_buffer,htonl(channel->remote_channel));
    buffer_add_ssh_string(session->out_buffer,request_s);
    buffer_add_u8(session->out_buffer,reply?1:0);
    if(buffer)
        buffer_add_data(session->out_buffer,buffer_get(buffer),buffer_get_len(buffer));
    packet_send(session);
    ssh_say(3,"Sent a SSH_MSG_CHANNEL_REQUEST %s\n",request);
    free(request_s);
    if(!reply)
        return 0;
    err=packet_wait(session,99,1);
    if(err)
        if(session->in_packet.type==100){
            ssh_say(2,"%s channel request failed\n",request);
            ssh_set_error(session,1,"Channel request %s failed",request);
        }
        else
            ssh_say(3,"Received an unexpected %d message\n",session->in_packet.type);
    else
        ssh_say(3,"Received a SUCCESS\n");
    return err;
}

int channel_request_pty_size(CHANNEL *channel, char *terminal, int col, int row){
    STRING *term;
    BUFFER *buffer;
    int err;

    if(channel->version==1)
        return channel_request_pty_size1(channel,terminal, col, row);

    term=string_from_char(terminal);
    buffer=buffer_new();
    buffer_add_ssh_string(buffer,term);
    buffer_add_u32(buffer,htonl(col));
    buffer_add_u32(buffer,htonl(row));
    buffer_add_u32(buffer,0);
    buffer_add_u32(buffer,0);
/* a 0byte string */
    buffer_add_u32(buffer,htonl(1));
    buffer_add_u8(buffer,0);
    free(term);
    err=channel_request(channel,"pty-req",buffer,1);
    buffer_free(buffer);
    return err;
}

int channel_request_pty(CHANNEL *channel){
    return channel_request_pty_size(channel,"xterm",80,24);
}

int channel_change_pty_size(CHANNEL *channel,int cols,int rows){
    BUFFER *buffer;
    int err;

    if(channel->version==1)
        return channel_change_pty_size1(channel,cols,rows);

    buffer=buffer_new();
    //buffer_add_u8(buffer,0);
    buffer_add_u32(buffer,htonl(cols));
    buffer_add_u32(buffer,htonl(rows));
    buffer_add_u32(buffer,0);
    buffer_add_u32(buffer,0);
    err=channel_request(channel,"window-change",buffer,0);
    buffer_free(buffer);
    return err;
}    
    
int channel_request_shell(CHANNEL *channel){

    if(channel->version==1)
        return channel_request_shell1(channel);

    return channel_request(channel,"shell",NULL,1);
}

int channel_request_subsystem(CHANNEL *channel, char *system){
    BUFFER* buffer=buffer_new();
    int ret;
    STRING *subsystem=string_from_char(system);
    buffer_add_ssh_string(buffer,subsystem);
    free(subsystem);
    ret=channel_request(channel,"subsystem",buffer,1);
    buffer_free(buffer);
    return ret;
}
    
int channel_request_sftp( CHANNEL *channel){
    return channel_request_subsystem(channel, "sftp");
}


int channel_request_env(CHANNEL *channel,char *name, char *value){
    BUFFER *buffer=buffer_new();
    int ret;
    STRING *string=string_from_char(name);
    buffer_add_ssh_string(buffer,string);
    free(string);
    string=string_from_char(value);
    buffer_add_ssh_string(buffer,string);
    free(string);
    ret=channel_request(channel,"env",buffer,1);
    buffer_free(buffer);
    return ret;
}

int channel_request_exec(CHANNEL *channel, char *cmd){
    BUFFER *buffer;
    int ret;

    if(channel->version==1)
        return channel_request_exec1(channel, cmd);

    buffer=buffer_new();
    STRING *command=string_from_char(cmd);
    buffer_add_ssh_string(buffer,command);
    free(command);
    ret=channel_request(channel,"exec",buffer,1);
    buffer_free(buffer);
    return ret;
}

/* TODO : fix the delayed close thing */
/* TODO : fix the blocking behaviours */
/* reads into a channel and put result into buffer */
/* returns number of bytes read, 0 if eof or such and -1 in case of error */
/* if bytes != 0, the exact number of bytes are going to be read */

int channel_read(CHANNEL *channel, BUFFER *buffer,int bytes,int is_stderr){
    BUFFER *stdbuf=NULL;
    int len;
    buffer_reinit(buffer);
    /* maybe i should always set a buffer to avoid races between channel_default_bufferize and channel_read */
    if(is_stderr)
        stdbuf=channel->stderr_buffer;
    else 
        stdbuf=channel->stdout_buffer;
    
    /* block reading if asked bytes=0 */
    while((buffer_get_rest_len(stdbuf)==0) || (buffer_get_rest_len(stdbuf) < bytes)){
        if(channel->remote_eof && buffer_get_rest_len(stdbuf)==0)
            return 0;
        if(channel->remote_eof)
            break; /* return the resting bytes in buffer */
        if(packet_read(channel->session)||packet_translate(channel->session))
            return -1;
        packet_parse(channel->session);
    }
    
    if(bytes==0){
        /* write the ful buffer informations */
        buffer_add_data(buffer,buffer_get_rest(stdbuf),buffer_get_rest_len(stdbuf));
        buffer_reinit(stdbuf);
    } else {
        len=buffer_get_rest_len(stdbuf);
        len= (len>bytes?bytes:len); /* read bytes bytes if len is greater, everything otherwise */
        buffer_add_data(buffer,buffer_get_rest(stdbuf),len);
        buffer_pass_bytes(stdbuf,len);
    }
    return buffer_get_len(buffer);
}

/* returns the number of bytes available, 0 if nothing is currently available, -1 if error */
int channel_poll(CHANNEL *channel, int is_stderr){
    BUFFER *buffer;
    int r=0,w=0,err=0;
    if(is_stderr)
        buffer=channel->stderr_buffer;
    else 
        buffer=channel->stdout_buffer;
    
    while(buffer_get_rest_len(buffer)==0 && !channel->remote_eof){
        r=ssh_fd_poll(channel->session,&w,&err);
        if(r<=0)
            return r; // error or no data available
        /* if an exception happened, it will be trapped by packet_read() */
        if(packet_read(channel->session)||packet_translate(channel->session))
            return -1;
        packet_parse(channel->session);
    }
    if(channel->remote_eof)
        return 1;
    return buffer_get_len(buffer);
}

/* nonblocking read on the specified channel. it will return <=len bytes of data read
 atomicly. */
int channel_read_nonblocking(CHANNEL *channel, char *dest, int len, int is_stderr){
    int to_read=channel_poll(channel,is_stderr);
    int lu;
    BUFFER *buffer=buffer_new();
    if(to_read<=0){
        buffer_free(buffer);
        return to_read; /* may be an error code */
    }
    if(to_read>len)
        to_read=len;
    lu=channel_read(channel,buffer,to_read,is_stderr);
    memcpy(dest,buffer_get(buffer),lu>=0?lu:0);
    buffer_free(buffer);
    return lu;
}

SSH_SESSION *channel_get_session(CHANNEL *channel){
    return channel->session;
}
/*
int channel_select(CHANNEL *readchans, CHANNEL *writechans, CHANNEL *exceptchans, struct 
        timeval * timeout){
    fd_set rset;
    fd_set wset;
    fd_set eset;
    int rmax=-1, wmax=-1, emax=-1; // nothing to do with the low quality editor :)
    int i;
    FD_ZERO(rset);
    FD_ZERO(wset);
    FD_ZERO(eset);
    for(i=0;readchans[i];++i){
        if(!readchans[i].
            
*/
