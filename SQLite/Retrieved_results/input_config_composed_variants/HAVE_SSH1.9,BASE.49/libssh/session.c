/* session.c */
/* contains the non-networking functions ssh_* */
/*
 * Copyright 2005 Aris Adamantiadis
 *
 * This file is part of the SSH Library
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA. */

/* ssh_new() returns a newly allocated SSH_SESSION structure pointer */
#include <string.h>
#include <stdlib.h>
#include "libssh/libssh.h"
#include "libssh/priv.h"
#include "libssh/server.h"
#define FIRST_CHANNEL 42

SSH_SESSION *ssh_new() {
    SSH_SESSION *session=malloc(sizeof (SSH_SESSION));
    memset(session,0,sizeof(SSH_SESSION));
    session->next_crypto=crypto_new();
    session->maxchannel=42;
    session->fd=-1;
    session->blocking=1;
    return session;
}

void ssh_cleanup(SSH_SESSION *session){
    int i;
    if(session->serverbanner)
        free(session->serverbanner);
    if(session->clientbanner)
        free(session->clientbanner);
    if(session->in_buffer)
        buffer_free(session->in_buffer);
    if(session->out_buffer)
        buffer_free(session->out_buffer);
    if(session->banner)
        free(session->banner);
    if(session->options)
        ssh_options_free(session->options);
    if(session->current_crypto)
        crypto_free(session->current_crypto);
    if(session->next_crypto)
        crypto_free(session->next_crypto);

    // delete all channels
    while(session->channels)
        channel_free(session->channels);
    if(session->client_kex.methods)
        for(i=0;i<10;i++)
            if(session->client_kex.methods[i])
                free(session->client_kex.methods[i]);
    if(session->server_kex.methods)
        for(i=0;i<10;++i)
            if(session->server_kex.methods[i])
                free(session->server_kex.methods[i]);
    free(session->client_kex.methods);
    free(session->server_kex.methods);
    if(session->dsa_key)
        private_key_free(session->dsa_key);
    if(session->rsa_key)
        private_key_free(session->rsa_key);
    if(session->ssh_message){
        ssh_message_free(session->ssh_message);
        free(session->ssh_message);
    }
    memset(session,'X',sizeof(SSH_SESSION)); /* burn connection, it could hangs 
                                                sensitive datas */
    free(session);
}

void ssh_silent_disconnect(SSH_SESSION *session){
    close(session->fd);
    session->alive=0;
    ssh_disconnect(session);
}

void ssh_set_options(SSH_SESSION *session, SSH_OPTIONS *options){
    session->options=options;
}

void ssh_set_blocking(SSH_SESSION *session,int blocking){
    session->blocking=blocking?1:0;
}

int ssh_get_fd(SSH_SESSION *session){
    return session->fd;
}

void ssh_set_fd_toread(SSH_SESSION *session){
    session->data_to_read=1;
}

void ssh_set_fd_towrite(SSH_SESSION *session){
    session->data_to_write=1;
}

void ssh_set_fd_except(SSH_SESSION *session){
    session->data_except=1;
}

/* looks if there is data to read on the socket and parse it. */
int ssh_handle_packets(SSH_SESSION *session){
    int w,err,r;
    do {
        r=ssh_fd_poll(session,&w,&err);
        if(r<=0)
            return r; // error or no data available
        /* if an exception happened, it will be trapped by packet_read() */
        if(packet_read(session)||packet_translate(session))
            return -1;
        packet_parse(session);
    } while(r>0);
    return r;
}

int ssh_get_status(SSH_SESSION *session){
    int ret=0;
    if(session->closed)
        ret |= (1<<0);
    if(session->channel_bytes_toread > 0 || session->data_to_read)
        ret |= (1<<1);
    if(session->closed && session->closed_by_except)
        ret |= (1<<2);
    return ret;
}

const char *ssh_get_disconnect_message(SSH_SESSION *session){
    if(!session->closed)
        ssh_set_error(session,1,"Connection not closed"
                " yet");
    else if(session->closed_by_except)
        ssh_set_error(session,1,"Connection closed by "
                "socket error");
    else if(!session->discon_msg)
        ssh_set_error(session,2,"Connection correctly closed but "
                "no disconnect message");
    else
        return session->discon_msg;
    return NULL;
}

int ssh_get_version(SSH_SESSION *session){
    return session->version;
}
