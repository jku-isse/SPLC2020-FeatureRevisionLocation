/* auth1.c deals with authentication with SSH-1 protocol */
/*
Copyright 2005 Aris Adamantiadis

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

#include "libssh/priv.h"
#include "libssh/ssh1.h"
#include <string.h>
#include <netdb.h>

/*
static void burn(char *ptr){
    if(ptr)
        memset(ptr,'X',strlen(ptr));
}
*/

static int wait_auth1_status(SSH_SESSION *session){
    /* wait for a packet */
    if(packet_read(session))
        return -1;
    if(packet_translate(session))
        return -1;
    switch(session->in_packet.type){
        case 14:
            return 0;
        case 15:
            return 1;
    }
    ssh_set_error(session,2,"Was waiting for a SUCCESS or "
    "FAILURE, got %d",session->in_packet.type);
    return -1;
}
static int send_username(SSH_SESSION *session, char *username){
    STRING *user;
    /* returns SSH_AUTH_SUCCESS or SSH_AUTH_DENIED */
    if(session->auth_service_asked)
        return session->auth_service_asked; 
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,4);
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return session->auth_service_asked=-1;
            else
                username=session->options->username;
        }
    user=string_from_char(username);
    buffer_add_ssh_string(session->out_buffer,user);
    free(user);
    packet_send(session);
    session->auth_service_asked=wait_auth1_status(session);
    return session->auth_service_asked;
}

/* use the "none" authentication question */

int ssh_userauth1_none(SSH_SESSION *session,char *username){
    return send_username(session,username);
}

/*
int ssh_userauth_offer_pubkey(SSH_SESSION *session, char *username,int type, STRING *publickey){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *algo;
    int err=SSH_AUTH_ERROR;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return SSH_AUTH_ERROR;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return SSH_AUTH_ERROR;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("publickey");
    algo=string_from_char(ssh_type_to_char(type));

    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_u8(session->out_buffer,0);
    buffer_add_ssh_string(session->out_buffer,algo);
    buffer_add_ssh_string(session->out_buffer,publickey);
    packet_send(session);
    err=wait_auth_status(session,0);
    free(user);
    free(method);
    free(service);
    free(algo);
    return err;
}
*/
int ssh_userauth1_offer_pubkey(SSH_SESSION *session, char *username, int type,
        STRING *pubkey){
    return 1;
}

/*
int ssh_userauth_pubkey(SSH_SESSION *session, char *username, STRING *publickey, PRIVATE_KEY *privatekey){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *algo;
    STRING *sign;
    int err=SSH_AUTH_ERROR;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return err;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return err;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("publickey");
    algo=string_from_char(ssh_type_to_char(privatekey->type));
    
    
*/    /* we said previously the public key was accepted */
/*    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_u8(session->out_buffer,1);
    buffer_add_ssh_string(session->out_buffer,algo);
    buffer_add_ssh_string(session->out_buffer,publickey);
    sign=ssh_do_sign(session,session->out_buffer,privatekey);
    if(sign){
        buffer_add_ssh_string(session->out_buffer,sign);
        free(sign);
        packet_send(session);
        err=wait_auth_status(session,0);
    }
    free(user);
    free(service);
    free(method);
    free(algo);
    return err;
}
*/

int ssh_userauth1_password(SSH_SESSION *session,char *username,char *password){
    STRING *password_s;
    int err;
    err=send_username(session,username);
    if(err!=1)
        return err;
    /* we trick a bit here. A known flaw in SSH1 protocol is that it's
     * easy to guess password sizes.
     * not that sure ...
     */
    if(strlen(password)>=128){
        /* not risky to disclose the size of such a big password .. */
        password_s=string_from_char(password);
    } else {
        /* fill the password string from random things. the strcpy
         * ensure there is at least a nul byte after the password.
         * most implementation won't see the garbage at end.
         * why garbage ? because nul bytes will be compressed by
         * gzip and disclose password len.
         */
        password_s=string_new(128);
        ssh_get_random(password_s->string,128);
        strcpy(password_s->string,password);
    }

    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,9);
    buffer_add_ssh_string(session->out_buffer,password_s);
    string_burn(password_s);
    free(password_s);
    packet_send(session);
    return wait_auth1_status(session); 
}


