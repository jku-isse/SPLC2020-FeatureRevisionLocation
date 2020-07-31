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
#include <string.h>

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>









#include <openssl/des.h>


#define OLD_CRYPTO 


SHACTX *sha1_init(){
    SHACTX *c=malloc(sizeof(SHACTX));
    SHA1_Init(c);
    return c;
}
void sha1_update(SHACTX *c, const void *data, unsigned long len){
    SHA1_Update(c,data,len);
}
void sha1_final(unsigned char *md,SHACTX *c){
    SHA1_Final(md,c);
    free(c);
}
void sha1(unsigned char *digest,int len,unsigned char *hash){
    SHA1(digest,len,hash);
}

MD5CTX *md5_init(){
    MD5CTX *c=malloc(sizeof(MD5CTX));
    MD5_Init(c);
    return c;
}
void md5_update(MD5CTX *c, const void *data, unsigned long len){
    MD5_Update(c,data,len);
}
void md5_final(unsigned char *md,MD5CTX *c){
    MD5_Final(md,c);
    free(c);
}

HMACCTX *hmac_init(const void *key, int len,int type){
    HMAC_CTX *ctx;
    ctx=malloc(sizeof(HMAC_CTX));

    

    switch(type){
        case 1:
            HMAC_Init(ctx,key,len,EVP_sha1());
            break;
        case 2:
            HMAC_Init(ctx,key,len,EVP_md5());
            break;
        default:
            free(ctx);
            ctx=NULL;
        }
    return ctx;
}
void hmac_update(HMACCTX *ctx,const void *data, unsigned long len){
    HMAC_Update(ctx,data,len);
}
void hmac_final(HMACCTX *ctx,unsigned char *hashmacbuf,int *len){
   HMAC_Final(ctx,hashmacbuf,len);

   

   HMAC_cleanup(ctx);

   free(ctx);
}

static void alloc_key(struct crypto_struct *cipher){
    cipher->key=malloc(cipher->keylen);
}




    
        
        
    



    



    




    
        
        
    


    
        
        
    


    


    



static void des3_set_key(struct crypto_struct *cipher, void *key){
    if(!cipher->key){
        alloc_key(cipher);
        DES_set_odd_parity(key);
        DES_set_odd_parity(key+8);
        DES_set_odd_parity(key+16);
        DES_set_key_unchecked(key,cipher->key);
        DES_set_key_unchecked(key+8,cipher->key+sizeof(DES_key_schedule));
        DES_set_key_unchecked(key+16,cipher->key+2*sizeof(DES_key_schedule));
    }
}

static void des3_encrypt(struct crypto_struct *cipher, void *in, void *out, 
        unsigned long len, void *IV){
    DES_ede3_cbc_encrypt(in,out,len,cipher->key,cipher->key+sizeof(DES_key_schedule),cipher->key+2*sizeof(DES_key_schedule),IV,1);
}

static void des3_decrypt(struct crypto_struct *cipher, void *in, void *out, 
        unsigned long len, void *IV){
    DES_ede3_cbc_encrypt(in,out,len,cipher->key,cipher->key+sizeof(DES_key_schedule),cipher->key+2*sizeof(DES_key_schedule),IV,0);
}

static void des3_1_encrypt(struct crypto_struct *cipher, void *in, void *out, 
        unsigned long len, void *IV){

    ssh_print_hexa("encrypt IV before",IV,24);

    DES_ncbc_encrypt(in,out,len, cipher->key, IV, 1);
    DES_ncbc_encrypt(out,in,len, cipher->key + sizeof(DES_key_schedule),
            IV+8,0);
    DES_ncbc_encrypt(in,out,len, cipher->key + 2*sizeof(DES_key_schedule),
            IV+16,1);

    ssh_print_hexa("encrypt IV after",IV,24);

}

static void des3_1_decrypt(struct crypto_struct *cipher, void *in, void *out, 
        unsigned long len, void *IV){

    ssh_print_hexa("decrypt IV before",IV,24);

    DES_ncbc_encrypt(in,out,len, cipher->key + 2*sizeof(DES_key_schedule),
            IV, 0);
    DES_ncbc_encrypt(out,in,len, cipher->key + sizeof(DES_key_schedule),
            IV+8,1);
    DES_ncbc_encrypt(in,out,len, cipher->key,
            IV+16,0);

    ssh_print_hexa("decrypt IV after",IV,24);

}



    
/* the table of supported ciphers */
static struct crypto_struct ssh_ciphertab[]={

    
        


    
        
    
        
    
        

    { "3des-cbc",8,sizeof(DES_key_schedule)*3,NULL,192,des3_set_key, 
        des3_set_key,des3_encrypt, des3_decrypt},
    { "3des-cbc-ssh1",8,sizeof(DES_key_schedule)*3,NULL,192,des3_set_key, 
        des3_set_key,des3_1_encrypt, des3_1_decrypt},
     { NULL,0,0,NULL,0,NULL,NULL,NULL}
};


/* it allocates a new cipher structure based on its offset into the global table */
struct crypto_struct *cipher_new(int offset){
    struct crypto_struct *cipher=malloc(sizeof(struct crypto_struct));
    /* note the memcpy will copy the pointers : so, you shouldn't free them */
    memcpy(cipher,&ssh_ciphertab[offset],sizeof(*cipher));
    return cipher;
}

void cipher_free(struct crypto_struct *cipher){
    if(cipher->key){
        // destroy the key
        memset(cipher->key,0,cipher->keylen);
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
        BN_clear_free(crypto->e);
    if(crypto->f)
        BN_clear_free(crypto->f);
    if(crypto->x)
        BN_clear_free(crypto->x);
    if(crypto->k)
        BN_clear_free(crypto->k);
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

