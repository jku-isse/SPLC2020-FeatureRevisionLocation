/* wrapper.c */
/* wrapping functions for crypto functions. */
/* why a wrapper ? let's say you want to port libssh from libcrypto of openssl to libfoo */
/* you are going to spend hours to remove every references to SHA1_Update() to libfoo_sha1_update */
/* after the work is finished, you're going to have only this file to modify */
/* it's not needed to say that your modifications are welcome */

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

#include "libssh/priv.h"
#include "libssh/crypto.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>




    
    
    


    


    
    
    


    



    
    
    


    


    
    
    



    
    
        
            
            
        
            
            
        
            
        
    
    


    



    
    
    



    




    
        
        
        
        
    



    



    



    
        
        
          
            
            
          
            
            
          
            
            
        
        
        
    



    



    



    
        
        
        
        
    



        
    



        
    



    
        
        
        
        
        
        
        
        
        
        
    



        
    
    
    



        
    
    
    




    
    
    
    
    
        
    
        
    

























    
    
    


    


    
    


    



    
    
    


    


    
    



    
    

    

    
        
            
            
        
            
            
        
            
            
        
    


    


   

   

   

   



    





    
        
        
    



    



    




    
        
        
    


    
        
        
    


    


    




    
        
        
        
        
        
        
        
    



        
    



        
    



        

    

    
    
            
    
            

    




        

    

    
            
    
            
    
            

    







    
        


    
        
    
        
    
        


    
        
    
        

     



/* it allocates a new cipher structure based on its offset into the global table */
struct crypto_struct *cipher_new(int offset){
    struct crypto_struct *cipher=malloc(sizeof(struct crypto_struct));
    /* note the memcpy will copy the pointers : so, you shouldn't free them */
    memcpy(cipher,&ssh_ciphertab[offset],sizeof(*cipher));
    return cipher;
}

void cipher_free(struct crypto_struct *cipher){

    

    if(cipher->key){

        
          

        

        free(cipher->key);
    }
    free(cipher);
}

CRYPTO *crypto_new(){
    CRYPTO *crypto=malloc(sizeof (CRYPTO));
    memset(crypto,0,sizeof(*crypto));
    return crypto;
}

void crypto_free(CRYPTO *crypto){
    if(crypto->server_pubkey)
        free(crypto->server_pubkey);
    if(crypto->in_cipher)
        cipher_free(crypto->in_cipher);
    if(crypto->out_cipher)
        cipher_free(crypto->out_cipher);
    if(crypto->e)
        bignum_free(crypto->e);
    if(crypto->f)
        bignum_free(crypto->f);
    if(crypto->x)
        bignum_free(crypto->x);
    if(crypto->k)
        bignum_free(crypto->k);
    /* lot of other things */
    /* i'm lost in my own code. good work */
    memset(crypto,0,sizeof(*crypto));
    free(crypto);
}

static int crypt_set_algorithms2(SSH_SESSION *session){
    /* we must scan the kex entries to find crypto algorithms and set their appropriate structure */
    int i=0;
    /* out */
    char *wanted=session->client_kex.methods[2];
    while(ssh_ciphertab[i].name && strcmp(wanted,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(session,2,"Crypt_set_algorithms : no crypto algorithm function found for %s",wanted);
        return -1;
    }
    ssh_say(2,"Set output algorithm %s\n",wanted);
    session->next_crypto->out_cipher=cipher_new(i);
    i=0;
    /* in */
    wanted=session->client_kex.methods[3];
    while(ssh_ciphertab[i].name && strcmp(wanted,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(session,2,"Crypt_set_algorithms : no crypto algorithm function found for %s",wanted);
        return -1;
    }
    ssh_say(2,"Set input algorithm %s\n",wanted);
    session->next_crypto->in_cipher=cipher_new(i);
    /* compression */
    if(strstr(session->client_kex.methods[6],"zlib"))
        session->next_crypto->do_compress_out=1;
    if(strstr(session->client_kex.methods[7],"zlib"))
        session->next_crypto->do_compress_in=1;
    return 0;
}

static int crypt_set_algorithms1(SSH_SESSION *session){
    int i=0;
    /* right now, we force 3des-cbc to be taken */
    while(ssh_ciphertab[i].name && strcmp(ssh_ciphertab[i].name,"3des-cbc-ssh1"))
        ++i;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(NULL,2,"cipher 3des-cbc-ssh1 not found !");
        return -1;
    }
    session->next_crypto->out_cipher=cipher_new(i);
    session->next_crypto->in_cipher=cipher_new(i);
    return 0;
}

int crypt_set_algorithms(SSH_SESSION *session){
    return session->version==1?crypt_set_algorithms1(session):
        crypt_set_algorithms2(session);
}

int crypt_set_algorithms_server(SSH_SESSION *session){
    /* we must scan the kex entries to find crypto algorithms and set their appropriate structure */
    int i=0;
    /* out */
    char *server=session->server_kex.methods[3];
    char *client=session->client_kex.methods[3];
    char *match=ssh_find_matching(client,server);
    while(ssh_ciphertab[i].name && strcmp(match,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(session,2,"Crypt_set_algorithms : no crypto algorithm function found for %s",server);
        return -1;
    }
    ssh_say(2,"Set output algorithm %s\n",match);
    session->next_crypto->out_cipher=cipher_new(i);
    i=0;
    /* in */
    client=session->client_kex.methods[2];
    server=session->server_kex.methods[3];
    match=ssh_find_matching(client,server); 
    while(ssh_ciphertab[i].name && strcmp(match,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(session,2,"Crypt_set_algorithms : no crypto algorithm function found for %s",server);
        return -1;
    }
    ssh_say(2,"Set input algorithm %s\n",match);
    session->next_crypto->in_cipher=cipher_new(i);
    /* compression */
    client=session->client_kex.methods[2];
    server=session->server_kex.methods[2];
    match=ssh_find_matching(client,server);
    if(match && !strcmp(match,"zlib")){
        ssh_say(2,"enabling C->S compression\n");
        session->next_crypto->do_compress_in=1;
    }
    
    client=session->client_kex.methods[3];
    server=session->server_kex.methods[3];
    match=ssh_find_matching(client,server);
    if(match && !strcmp(match,"zlib")){
        ssh_say(2,"enabling S->C compression\n");
        session->next_crypto->do_compress_out=1;
    }

    server=session->server_kex.methods[1];
    client=session->client_kex.methods[1];
    match=ssh_find_matching(client,server);
    if(!strcmp(match,"ssh-dss"))
        session->hostkeys=1;
    else if(!strcmp(match,"ssh-rsa"))
        session->hostkeys=2;
    else {
        ssh_set_error(session,2,"cannot know what %s is into %s",match,server);
        return -1;
    }
    return 0;
}
