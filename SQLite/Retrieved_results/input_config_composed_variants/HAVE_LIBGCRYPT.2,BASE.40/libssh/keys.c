/* keys handle the public key related functions */
/* decoding a public key (both rsa and dsa), decoding a signature (rsa and dsa), veryfying them */

/*
Copyright 2003-2005 Aris Adamantiadis

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
#include <string.h>
#include <netinet/in.h>




#include "libssh/priv.h"


/* Public key decoding functions */

char *ssh_type_to_char(int type){
    switch(type){
        case 1:
            return "ssh-dss";
        case 2:
        case 3:
            return "ssh-rsa";
        default:
            return NULL;
    }
}

PUBLIC_KEY *publickey_make_dss(BUFFER *buffer){
    STRING *p,*q,*g,*pubkey;
    PUBLIC_KEY *key=malloc(sizeof(PUBLIC_KEY));
    key->type=1;
    key->type_c="ssh-dss";
    p=buffer_get_ssh_string(buffer);
    q=buffer_get_ssh_string(buffer);
    g=buffer_get_ssh_string(buffer);
    pubkey=buffer_get_ssh_string(buffer);
    buffer_free(buffer); /* we don't need it anymore */
    if(!p || !q || !g || !pubkey){
        ssh_set_error(NULL,2,"Invalid DSA public key");
        if(p)
            free(p);
        if(q)
            free(q);
        if(g)
            free(g);
        if(pubkey)
            free(pubkey);
        free(key);
        return NULL;
    }

    gcry_sexp_build(&key->dsa_pub,NULL,"(public-key(dsa(p %b)(q %b)(g %b)(y %b)))",string_len(p),p->string,string_len(q),q->string,string_len(g),g->string,string_len(pubkey),pubkey->string);

    
    
    
    

    free(p);
    free(q);
    free(g);
    free(pubkey);
    return key;
}

PUBLIC_KEY *publickey_make_rsa(BUFFER *buffer, char *type){
    STRING *e,*n;
    PUBLIC_KEY *key=malloc(sizeof(PUBLIC_KEY));
    if(!strcmp(type,"ssh-rsa"))
        key->type=2;
    else
        key->type=3;
    key->type_c=type;
    e=buffer_get_ssh_string(buffer);
    n=buffer_get_ssh_string(buffer);
    buffer_free(buffer); /* we don't need it anymore */
    if(!e || !n){
        ssh_set_error(NULL,2,"Invalid RSA public key");
        if(e)
            free(e);
        if(n)
            free(n);
        free(key);
        return NULL;
    }

    gcry_sexp_build(&key->rsa_pub,NULL,"(public-key(rsa(n %b)(e %b)))",string_len(n),n->string,string_len(e),e->string);

    
    


    
    

    free(e);
    free(n);
    return key;
}

void publickey_free(PUBLIC_KEY *key){
    if(!key)
        return;
    switch(key->type){
        case 1:

            gcry_sexp_release(key->dsa_pub);


            break;
        case 2:
        case 3:

            gcry_sexp_release(key->rsa_pub);


            break;
        default:
            break;
    }
    free(key);
}

PUBLIC_KEY *publickey_from_string(STRING *pubkey_s){
    BUFFER *tmpbuf=buffer_new();
    STRING *type_s;
    char *type;

    buffer_add_data(tmpbuf,pubkey_s->string,string_len(pubkey_s));
    type_s=buffer_get_ssh_string(tmpbuf);
    if(!type_s){
        buffer_free(tmpbuf);
        ssh_set_error(NULL,2,"Invalid public key format");
        return NULL;
    }
    type=string_to_char(type_s);
    free(type_s);
    if(!strcmp(type,"ssh-dss")){
        free(type);
        return publickey_make_dss(tmpbuf);
    }
    if(!strcmp(type,"ssh-rsa")){
        free(type);
        return publickey_make_rsa(tmpbuf,"ssh-rsa");
    }
    if(!strcmp(type,"ssh-rsa1")){
        free(type);
        return publickey_make_rsa(tmpbuf,"ssh-rsa1");
    }
    ssh_set_error(NULL,2,"unknown public key protocol %s",type);
    buffer_free(tmpbuf);
    free(type);
    return NULL;
}

PUBLIC_KEY *publickey_from_privatekey(PRIVATE_KEY *prv){
    PUBLIC_KEY *key=malloc(sizeof(PUBLIC_KEY));

    gcry_sexp_t sexp;
    const char *tmp;
    size_t size;
    STRING *p,*q,*g,*y,*e,*n;

    key->type=prv->type;
    switch(key->type){
        case 1:

            sexp=gcry_sexp_find_token(prv->dsa_priv,"p",0);
            tmp=gcry_sexp_nth_data(sexp,1,&size);
            p=string_new(size);
            string_fill(p,(char *)tmp,size);
            gcry_sexp_release(sexp);
            sexp=gcry_sexp_find_token(prv->dsa_priv,"q",0);
            tmp=gcry_sexp_nth_data(sexp,1,&size);
            q=string_new(size);
            string_fill(q,(char *)tmp,size);
            gcry_sexp_release(sexp);
            sexp=gcry_sexp_find_token(prv->dsa_priv,"g",0);
            tmp=gcry_sexp_nth_data(sexp,1,&size);
            g=string_new(size);
            string_fill(g,(char *)tmp,size);
            gcry_sexp_release(sexp);
            sexp=gcry_sexp_find_token(prv->dsa_priv,"y",0);
            tmp=gcry_sexp_nth_data(sexp,1,&size);
            y=string_new(size);
            string_fill(y,(char *)tmp,size);
            gcry_sexp_release(sexp);
            gcry_sexp_build(&key->dsa_pub,NULL,"(public-key(dsa(p %b)(q %b)(g %b)(y %b)))",string_len(p),p->string,string_len(q),q->string,string_len(g),g->string,string_len(y),y->string);
            free(p);
            free(q);
            free(g);
            free(y);

            
            
            
            

            break;
        case 2:
        case 3:

            sexp=gcry_sexp_find_token(prv->rsa_priv,"n",0);
            tmp=gcry_sexp_nth_data(sexp,1,&size);
            n=string_new(size);
            string_fill(n,(char *)tmp,size);
            gcry_sexp_release(sexp);
            sexp=gcry_sexp_find_token(prv->rsa_priv,"e",0);
            tmp=gcry_sexp_nth_data(sexp,1,&size);
            e=string_new(size);
            string_fill(e,(char *)tmp,size);
            gcry_sexp_release(sexp);
            gcry_sexp_build(&key->rsa_pub,NULL,"(public-key(rsa(n %b)(e %b)))",string_len(n),n->string,string_len(e),e->string);
            free(e);
            free(n);

            
            

            break;
    }
    key->type_c=ssh_type_to_char(prv->type);
    return key;
}


static void dsa_public_to_string(gcry_sexp_t key, BUFFER *buffer){


    STRING *p,*q,*g,*n;

    const char *tmp;
    size_t size;
    gcry_sexp_t sexp;
    sexp=gcry_sexp_find_token(key,"p",0);
    tmp=gcry_sexp_nth_data(sexp,1,&size);
    p=string_new(size);
    string_fill(p,(char *)tmp,size);
    gcry_sexp_release(sexp);
    sexp=gcry_sexp_find_token(key,"q",0);
    tmp=gcry_sexp_nth_data(sexp,1,&size);
    q=string_new(size);
    string_fill(q,(char *)tmp,size);
    gcry_sexp_release(sexp);
    sexp=gcry_sexp_find_token(key,"g",0);
    tmp=gcry_sexp_nth_data(sexp,1,&size);
    g=string_new(size);
    string_fill(g,(char *)tmp,size);
    gcry_sexp_release(sexp);
    sexp=gcry_sexp_find_token(key,"y",0);
    tmp=gcry_sexp_nth_data(sexp,1,&size);
    n=string_new(size);
    string_fill(n,(char *)tmp,size);
    gcry_sexp_release(sexp);

    
    
    

    buffer_add_ssh_string(buffer,p);
    buffer_add_ssh_string(buffer,q);
    buffer_add_ssh_string(buffer,g);
    buffer_add_ssh_string(buffer,n);
    free(p);
    free(q);
    free(g);
    free(n);
}


static void rsa_public_to_string(gcry_sexp_t key, BUFFER *buffer){


    STRING *e, *n;

    const char *tmp;
    size_t size;
    gcry_sexp_t sexp;
    sexp=gcry_sexp_find_token(key,"n",0);
    tmp=gcry_sexp_nth_data(sexp,1,&size);
    n=string_new(size);
    string_fill(n,(char *)tmp,size);
    gcry_sexp_release(sexp);
    sexp=gcry_sexp_find_token(key,"e",0);
    tmp=gcry_sexp_nth_data(sexp,1,&size);
    e=string_new(size);
    string_fill(e,(char *)tmp,size);
    gcry_sexp_release(sexp);

    

    buffer_add_ssh_string(buffer,e);
    buffer_add_ssh_string(buffer,n);
    free(e);
    free(n);
}

STRING *publickey_to_string(PUBLIC_KEY *key){
    STRING *type;
    STRING *ret;
    BUFFER *buf;
    type=string_from_char(ssh_type_to_char(key->type));
    buf=buffer_new();
    buffer_add_ssh_string(buf,type);
    switch(key->type){
        case 1:
            dsa_public_to_string(key->dsa_pub,buf);
            break;
        case 2:
        case 3:
            rsa_public_to_string(key->rsa_pub,buf);
            break;
    }
    ret=string_new(buffer_get_len(buf));
    string_fill(ret,buffer_get(buf),buffer_get_len(buf));
    buffer_free(buf);
    free(type);
    return ret;
}

/* Signature decoding functions */

STRING *signature_to_string(SIGNATURE *sign){
    STRING *str;
    STRING *rs;

    const char *r,*s;
    gcry_sexp_t sexp;
    size_t size;


    unsigned char buffer[40];
    BUFFER *tmpbuf=buffer_new();
    STRING *tmp;
    tmp=string_from_char(ssh_type_to_char(sign->type));
    buffer_add_ssh_string(tmpbuf,tmp);
    free(tmp);
    switch(sign->type){
        case 1:
            memset(buffer,0,40);

            sexp=gcry_sexp_find_token(sign->dsa_sign,"r",0);
            r=gcry_sexp_nth_data(sexp,1,&size);
            if (*r == 0)      /* libgcrypt put 0 when first bit is set */
            {
              size--;
              r++;
            }
            memcpy(buffer,r + size - 20,20);
            gcry_sexp_release(sexp);
            sexp=gcry_sexp_find_token(sign->dsa_sign,"s",0);
            s=gcry_sexp_nth_data(sexp,1,&size);
            if (*s == 0)
            {
              size--;
              s++;
            }
            memcpy(buffer+ 20, s + size - 20, 20);
            gcry_sexp_release(sexp);

            
            
            
            
            
            

            rs=string_new(40);
            string_fill(rs,buffer,40);
            buffer_add_ssh_string(tmpbuf,rs);
            free(rs);
            break;
        case 2:
        case 3:

            sexp=gcry_sexp_find_token(sign->rsa_sign,"s",0);
            s=gcry_sexp_nth_data(sexp,1,&size);
            if (*s == 0)
            {
              size--;
              s++;
            }
            rs=string_new(size);
            string_fill(rs,(char *)s,size);
            buffer_add_ssh_string(tmpbuf,rs);
            gcry_sexp_release(sexp);
            free(rs);


            break;
    }
    str=string_new(buffer_get_len(tmpbuf));
    string_fill(str,buffer_get(tmpbuf),buffer_get_len(tmpbuf));
    buffer_free(tmpbuf);
    return str;
}

/* TODO : split this function in two so it becomes smaller */
SIGNATURE *signature_from_string(STRING *signature,PUBLIC_KEY *pubkey,int needed_type){

    gcry_sexp_t sig;

    

    SIGNATURE *sign=malloc(sizeof(SIGNATURE));
    BUFFER *tmpbuf=buffer_new();
    STRING *rs;
    STRING *type_s,*e;
    int len,rsalen;
    char *type;
    buffer_add_data(tmpbuf,signature->string,string_len(signature));
    type_s=buffer_get_ssh_string(tmpbuf);
    if(!type_s){
        ssh_set_error(NULL,2,"Invalid signature packet");
        buffer_free(tmpbuf);
        return NULL;
    }
    type=string_to_char(type_s);
    free(type_s);
    switch(needed_type){
        case 1:
            if(strcmp(type,"ssh-dss")){
                ssh_set_error(NULL,2,"Invalid signature type : %s",type);
                buffer_free(tmpbuf);
                free(type);
                return NULL;
            }
            break;
        case 2:
            if(strcmp(type,"ssh-rsa")){
                ssh_set_error(NULL,2,"Invalid signature type : %s",type);
                buffer_free(tmpbuf);
                free(type);
                return NULL;
            }
            break;
        default:
            ssh_set_error(NULL,2,"Invalid signature type : %s",type);
            free(type);
            buffer_free(tmpbuf);
            return NULL;
    }
    free(type);
    switch(needed_type){
        case 1:
            rs=buffer_get_ssh_string(tmpbuf);
            buffer_free(tmpbuf);
            if(!rs || string_len(rs)!=40){ /* 40 is the dual signature blob len. */
                if(rs)
                    free(rs);
                return NULL;
            }
            /* we make use of strings (because we have all-made functions to convert them to bignums (ou pas ;)*/

            gcry_sexp_build(&sig,NULL,"(sig-val(dsa(r %b)(s %b)))",20,rs->string,20,rs->string+20);

            
            
            
            
            
            
            
            


            
            

            free(rs);
            sign->type=1;
            sign->dsa_sign=sig;
            return sign;
        case 2:
            e=buffer_get_ssh_string(tmpbuf);
            buffer_free(tmpbuf);
            if(!e){
                return NULL;
            }
            len=string_len(e);

            rsalen=(gcry_pk_get_nbits(pubkey->rsa_pub)+7)/8;


            if(len>rsalen){
                free(e);
                free(sign);
                ssh_set_error(NULL,2,"signature too big ! %d instead of %d",len,rsalen);
                return NULL;
            }
            if(len<rsalen)
                ssh_say(0,"Len %d < %d\n",len,rsalen);
            sign->type=2;

            gcry_sexp_build(&sig,NULL,"(sig-val(rsa(s %b)))",string_len(e),e->string);
            sign->rsa_sign=sig;



            
            


            free(e);

            return sign;
        default:
            return NULL;
    }
}

void signature_free(SIGNATURE *sign){
    if(!sign)
        return;
    switch(sign->type){
        case 1:

            gcry_sexp_release(sign->dsa_sign);


            break;
        case 2:
        case 3:

            gcry_sexp_release(sign->rsa_sign);


            break;
        default:
            ssh_say(1,"freeing a signature with no type !\n");
    }
    free(sign);
}





    
    
    
    
    
    
        
        
    
    
    
    
    



/* this function signs the session id (known as H) as a string then the content of sigbuf */
STRING *ssh_do_sign(SSH_SESSION *session,BUFFER *sigbuf, PRIVATE_KEY *privatekey){
    SHACTX ctx;
    STRING *session_str=string_new(20);
    unsigned char hash[20+1];

    gcry_sexp_t gcryhash;

    SIGNATURE *sign;
    STRING *signature;
    CRYPTO *crypto=session->current_crypto?session->current_crypto:session->next_crypto;
    string_fill(session_str,crypto->session_id,20);
    ctx=sha1_init();
    sha1_update(ctx,session_str,string_len(session_str)+4);
    sha1_update(ctx,buffer_get(sigbuf),buffer_get_len(sigbuf));
    sha1_final(hash+1,ctx);
    hash[0]=0;

    

    free(session_str);
    sign=malloc(sizeof(SIGNATURE));
    switch(privatekey->type){
        case 1:

            gcry_sexp_build(&gcryhash,NULL,"%b",20+1,hash);
            gcry_pk_sign(&sign->dsa_sign,gcryhash,privatekey->dsa_priv);


            
            


            sign->rsa_sign=NULL;
            break;
        case 2:

            gcry_sexp_build(&gcryhash,NULL,"(data(flags pkcs1)(hash sha1 %b))",20,hash+1);
            gcry_pk_sign(&sign->rsa_sign,gcryhash,privatekey->rsa_priv);


            sign->dsa_sign=NULL;
            break;
    }

    gcry_sexp_release(gcryhash);

    sign->type=privatekey->type;
    if(!sign->dsa_sign && !sign->rsa_sign){

        ssh_set_error(session,2,"Signing : libcrypt error");


        signature_free(sign);
        return NULL;
    }
    signature=signature_to_string(sign);
    signature_free(sign);
    return signature;
}

STRING *ssh_encrypt_rsa1(SSH_SESSION *session, STRING *data, PUBLIC_KEY *key){
    int len=string_len(data);

    STRING *ret;
    gcry_sexp_t ret_sexp;
    gcry_sexp_t data_sexp;
    const char *tmp;
    size_t size;
    gcry_sexp_build(&data_sexp,NULL,"(data(flags pkcs1)(value %b))",len,data->string);
    gcry_pk_encrypt(&ret_sexp,data_sexp,key->rsa_pub);
    gcry_sexp_release(data_sexp);
    data_sexp=gcry_sexp_find_token(ret_sexp,"a",0);
    tmp=gcry_sexp_nth_data(data_sexp,1,&size);
    if (*tmp == 0)
    {
      size--;
      tmp++;
    }
    ret=string_new(size);
    string_fill(ret,(char *)tmp,size);
    gcry_sexp_release(ret_sexp);

    
    
            

    return ret;
}


/* this function signs the session id */
STRING *ssh_sign_session_id(SSH_SESSION *session, PRIVATE_KEY *privatekey){
    SHACTX ctx;
    unsigned char hash[20+1];
    SIGNATURE *sign;
    STRING *signature;
    CRYPTO *crypto=session->current_crypto?session->current_crypto:session->next_crypto;

    gcry_sexp_t data_sexp;

    ctx=sha1_init();
    sha1_update(ctx,crypto->session_id,20);
    sha1_final(hash+1,ctx);
    hash[0]=0;

    

    sign=malloc(sizeof(SIGNATURE));
    switch(privatekey->type){
        case 1:

            gcry_sexp_build(&data_sexp,NULL,"%b",20+1,hash);
            gcry_pk_sign(&sign->dsa_sign,data_sexp,privatekey->dsa_priv);


            
            


            sign->rsa_sign=NULL;
            break;
        case 2:

            gcry_sexp_build(&data_sexp,NULL,"(data(flags pkcs1)(hash sha1 %b))",20,hash+1);
            gcry_pk_sign(&sign->rsa_sign,data_sexp,privatekey->rsa_priv);


            sign->dsa_sign=NULL;
            break;
    }

    gcry_sexp_release(data_sexp);

    sign->type=privatekey->type;
    if(!sign->dsa_sign && !sign->rsa_sign){

        ssh_set_error(session,2,"Signing : libgcrypt error");


        signature_free(sign);
        return NULL;
    }
    signature=signature_to_string(sign);
    signature_free(sign);
    return signature;
}

