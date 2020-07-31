/* keyfiles.c */
/* This part of the library handles private and public key files needed for publickey authentication,*/
/* as well as servers public hashes verifications and certifications. Lot of code here handles openssh */
/* implementations (key files aren't standardized yet). */

/*
Copyright 2003,04 Aris Adamantiadis

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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "libssh/priv.h"

#include <openssl/pem.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#include <netinet/in.h>
#define MAXLINESIZE 80

#define MAX_KEY_SIZE 32
#define MAX_PASSPHRASE_SIZE 1024
#define RSA_HEADER_BEGIN "-----BEGIN RSA PRIVATE KEY-----"
#define RSA_HEADER_END "-----END RSA PRIVATE KEY-----"
#define DSA_HEADER_BEGIN "-----BEGIN DSA PRIVATE KEY-----"
#define DSA_HEADER_END "-----END DSA PRIVATE KEY-----"
#define ASN1_INTEGER 2
#define ASN1_SEQUENCE 48
#define PKCS5_SALT_LEN 8

int load_iv(char *header, unsigned char *iv, int iv_len)
{
  int i;
  int j;
  int k;

  memset(iv, 0, iv_len);
  for (i=0; i < iv_len; i++)
  {
    if ((header[2*i] >= '0') && (header[2*i] <= '9'))
      j = header[2*i] - '0';
    else if ((header[2*i] >= 'A') && (header[2*i] <= 'F'))
      j = header[2*i] - 'A' + 10;
    else if ((header[2*i] >= 'a') && (header[2*i] <= 'f'))
      j = header[2*i] - 'a' + 10;
    else
      return 0;
    if ((header[2*i+1] >= '0') && (header[2*i+1] <= '9'))
      k = header[2*i+1] - '0';
    else if ((header[2*i+1] >= 'A') && (header[2*i+1] <= 'F'))
      k = header[2*i+1] - 'A' + 10;
    else if ((header[2*i+1] >= 'a') && (header[2*i+1] <= 'f'))
      k = header[2*i+1] - 'a' + 10;
    else
      return 0;
    iv[i] = (j << 4) + k;
  }
  return 1;
}

u32 char_to_u32(unsigned char *data, u32 size)
{
  u32 ret;
  u32 i;

  for (i=0,ret=0;i<size;ret=ret<<8,ret+=data[i++])
    ;
  return ret;
}

u32 asn1_get_len(BUFFER *buffer)
{
  u32 len;
  unsigned char tmp[4];
  
  if (!buffer_get_data(buffer,tmp,1))
    return 0;
  if (tmp[0] > 127)
  {
    len=tmp[0] & 127;
    if (len>4)
      return 0; /* Length doesn't fit in u32. Can this really happen? */
    if (!buffer_get_data(buffer,tmp,len))
      return 0;
    len=char_to_u32(tmp,len);
  }
  else
    len=char_to_u32(tmp,1);
  return len;
}

STRING *asn1_get_int(BUFFER *buffer)
{
  STRING *ret;
  unsigned char type;
  u32 size;
  
  if (!buffer_get_data(buffer,&type,1) || type != 2)
    return NULL;
  size=asn1_get_len(buffer);
  if (!size)
    return NULL;
  ret=string_new(size);
  if (!buffer_get_data(buffer,ret->string,size))
    return NULL;
  return ret;
}

int asn1_check_sequence(BUFFER *buffer)
{
  unsigned char tmp;
  unsigned char *j;
  int i;
  u32 size;

  if (!buffer_get_data(buffer,&tmp,1) || tmp != 48)
    return 0;
  size=asn1_get_len(buffer);
  if (size != buffer_get_len(buffer) - buffer->pos)
    for (i = buffer_get_len(buffer) - buffer->pos - size,
         j = buffer_get(buffer) + size + buffer->pos; i; i--, j++)
    {
      if (*j != 1)                           /* padding is allowed */
        return 0;                            /* but nothing else */
    }
  return 1;
}

int read_line(char *data, unsigned int len, FILE *fp)
{
  char tmp;
  int i;

  for (i=0; fread(&tmp, 1, 1, fp) && tmp!='\n' && i<len; data[i++]=tmp)
    ;
  if (tmp=='\n')
    return i;
  if (i>=len)
    return -1;
  return 0;
}

int passphrase_to_key(char *data, unsigned int datalen, unsigned char *salt, unsigned char *key,unsigned int keylen)
{
  MD5CTX md;
  unsigned char digest[16];
  unsigned int i;
  unsigned int j;
  unsigned int md_not_empty;

  for (j=0,md_not_empty=0;j<keylen;)
  {
    md = md5_init();
    if (!md)
      return 0;
    if (md_not_empty)
      md5_update(md,digest,16);
    else
      md_not_empty=1;
    md5_update(md,data,datalen);
    if (salt)
      md5_update(md, salt, 8);
    md5_final(digest,md);
    for (i = 0; j < keylen && i < 16; j++, i++)
      if (key)
        key[j] = digest[i];
  }
  return 1;
}

int privatekey_decrypt(int algo, int mode, unsigned int key_len,
                       unsigned char *iv, unsigned int iv_len,
                       BUFFER *data, int cb(char *,int , int , char *),
                       char *desc)
{
  gcry_cipher_hd_t cipher;
  unsigned int passphrase_len;
  char passphrase[1024];
  unsigned char key[32];
  unsigned char *tmp;
  gcry_error_t err;
  
  if (!algo)
    return 1;
  passphrase_len=cb(passphrase, 1024, 0, desc);
  if (passphrase_len <= 0)
    return 0;
  passphrase_to_key(passphrase, passphrase_len, iv, key, key_len);
  if (gcry_cipher_open(&cipher, algo, mode, GCRY_CIPHER_SECURE)
      || gcry_cipher_setkey(cipher, key, key_len)
      || gcry_cipher_setiv(cipher, iv, iv_len)
      || !(tmp = malloc(buffer_get_len(data) * sizeof (char)))
      || (err = gcry_cipher_decrypt(cipher, tmp, buffer_get_len(data),
                                    buffer_get(data), buffer_get_len(data))))
  {
    gcry_cipher_close(cipher);
    return 0;
  }
  memcpy(buffer_get(data), tmp, buffer_get_len(data));
  gcry_cipher_close(cipher);
  return 1;
} 

int privatekey_dek_header(char *header, unsigned int header_len, int *algo, int *mode, unsigned int *key_len, unsigned char **iv, unsigned int *iv_len)
{
  unsigned int iv_pos;
  
  if (header_len > 13 && !strncmp("DES-EDE3-CBC", header, 12))
  {
    *algo = GCRY_CIPHER_3DES;
    iv_pos = 13;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 24;
    *iv_len = 8;
  }
  else if (header_len > 8 && !strncmp("DES-CBC", header, 7))
  {
    *algo = GCRY_CIPHER_DES;
    iv_pos = 8;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 8;
    *iv_len = 8;
  }
  else if (header_len > 12 && !strncmp("AES-128-CBC", header, 11))
  {
    *algo = GCRY_CIPHER_AES128;
    iv_pos = 12;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 16;
    *iv_len = 16;
  }
  else if (header_len > 12 && !strncmp("AES-192-CBC", header, 11))
  {
    *algo = GCRY_CIPHER_AES192;
    iv_pos = 12;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 24;
    *iv_len = 16;
  }
  else if (header_len > 12 && !strncmp("AES-256-CBC", header, 11))
  {
    *algo = GCRY_CIPHER_AES256;
    iv_pos = 12;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 32;
    *iv_len = 16;
  }
  else
    return 0;
  *iv = malloc(*iv_len);
  load_iv(header + iv_pos, *iv, *iv_len);
  return 1;
}

BUFFER *privatekey_file_to_buffer(FILE *fp, int type, int cb(char *, int , int , char *), char *desc)
{
  char buf[80];
  char *header_begin;
  unsigned int header_begin_size;
  char *header_end;
  unsigned int header_end_size;
  BUFFER *buffer=buffer_new();
  BUFFER *ret;
  int len;
  int algo = 0;
  int mode = 0;
  unsigned int key_len = 0;
  unsigned char *iv = NULL;
  unsigned int iv_len = 0;

  switch(type)
  {
    case 1:
      header_begin="-----BEGIN DSA PRIVATE KEY-----";
      header_end="-----END DSA PRIVATE KEY-----";
      break;
    case 2:
      header_begin="-----BEGIN RSA PRIVATE KEY-----";
      header_end="-----END RSA PRIVATE KEY-----";
      break;
    default:
      return NULL;
  }
  header_begin_size=strlen(header_begin);
  header_end_size=strlen(header_end);
  while (read_line(buf,80,fp) && strncmp(buf,header_begin,header_begin_size))
    ;
  len = read_line(buf, 80, fp);
  if (len > 11 && !strncmp("Proc-Type: 4,ENCRYPTED", buf, 11))
  {
    len = read_line(buf, 80, fp);
    if (len > 10 && !strncmp("DEK-Info: ", buf, 10))
    {
      if (!privatekey_dek_header(buf + 10, len - 10, &algo, &mode, &key_len,
                                 &iv, &iv_len)
          || read_line(buf, 80, fp))
      {
        buffer_free(buffer);
        free(iv);
        return NULL;
      }
    }
    else
    {
      buffer_free(buffer);
      free(iv);
      return NULL;
    }
  }
  else
    buffer_add_data(buffer,buf,len);
  while ((len = read_line(buf,80,fp))
         && strncmp(buf,header_end,header_end_size))
  {
    if (len == -1)
    {
      buffer_free(buffer);
      free(iv);
      return NULL;
    }
    buffer_add_data(buffer,buf,len);
  }
  if (strncmp(buf,header_end,header_end_size))
  {
    buffer_free(buffer);
    free(iv);
    return NULL;
  }
  buffer_add_data(buffer,"\0",1);
  ret=base64_to_bin(buffer_get(buffer));
  buffer_free(buffer);
  if (algo)
  {
    if (!privatekey_decrypt(algo, mode, key_len, iv, iv_len, ret, cb, desc))
    {
      free(iv);
      return NULL;
    }
  }
  free(iv);
  return ret;
}

int read_rsa_privatekey(FILE *fp, gcry_sexp_t *r,
                        int cb(char *, int , int , char *), char *desc)
{
  STRING *n;
  STRING *e;
  STRING *d;
  STRING *p;
  STRING *q;
  STRING *unused1;
  STRING *unused2;
  STRING *u;
  STRING *v;
  BUFFER *buffer;
 
  if (!(buffer=privatekey_file_to_buffer(fp, 2, cb, desc)))
    return 0;
  if (!asn1_check_sequence(buffer))
  {
    buffer_free(buffer);
    return 0;
  }
  v=asn1_get_int(buffer);
  if (ntohl(v->size)!=1 || v->string[0]!=0)
  {
    buffer_free(buffer);
    return 0;
  }
  n=asn1_get_int(buffer);
  e=asn1_get_int(buffer);
  d=asn1_get_int(buffer);
  q=asn1_get_int(buffer);
  p=asn1_get_int(buffer);
  unused1=asn1_get_int(buffer);
  unused2=asn1_get_int(buffer);
  u=asn1_get_int(buffer);
  buffer_free(buffer);
  if (!n || !e || !d || !p || !q || !unused1 || !unused2 || !u)
    return 0;
  gcry_sexp_build(r,NULL,"(private-key(rsa(n %b)(e %b)(d %b)(p %b)(q %b)(u %b)))",ntohl(n->size),n->string,ntohl(e->size),e->string,ntohl(d->size),d->string,ntohl(p->size),p->string,ntohl(q->size),q->string,ntohl(u->size),u->string);
  free(n);
  free(e);
  free(d);
  free(p);
  free(q);
  free(unused1);
  free(unused2);
  free(u);
  free(v);
  return 1;
}
int read_dsa_privatekey(FILE *fp, gcry_sexp_t *r, int cb(char *, int , int , char *), char *desc)
{
  STRING *p;
  STRING *q;
  STRING *g;
  STRING *y;
  STRING *x;
  STRING *v;
  BUFFER *buffer;

 
  if (!(buffer=privatekey_file_to_buffer(fp, 1, cb, desc)))
    return 0;
  if (!asn1_check_sequence(buffer))
  {
    buffer_free(buffer);
    return 0;
  }
  v=asn1_get_int(buffer);
  if (ntohl(v->size)!=1 || v->string[0]!=0)
  {
    buffer_free(buffer);
    return 0;
  }
  p=asn1_get_int(buffer);
  q=asn1_get_int(buffer);
  g=asn1_get_int(buffer);
  y=asn1_get_int(buffer);
  x=asn1_get_int(buffer);
  buffer_free(buffer);
  if (!p || !q || !g || !y || !x)
    return 0;
  gcry_sexp_build(r,NULL,"(private-key(dsa(p %b)(q %b)(g %b)(y %b)(x %b)))",ntohl(p->size),p->string,ntohl(q->size),q->string,ntohl(g->size),g->string,ntohl(y->size),y->string,ntohl(x->size),x->string);
  free(p);
  free(q);
  free(g);
  free(y);
  free(x);
  free(v);
  return 1;
}


static int default_get_password(char *buf, int size,int rwflag, char *descr){
    char *pass;
    char buffer[256];
    int len;
    snprintf(buffer,256,"Please enter passphrase for %s",descr);
    pass=getpass(buffer);
    snprintf(buf,size,"%s",pass);
    len=strlen(buf);
    memset(pass,0,strlen(pass));
    return len;
}

/* in case the passphrase has been given in parameter */
static int get_password_specified(char *buf,int size, int rwflag, char *password){
    snprintf(buf,size,"%s",password);
    return strlen(buf);
}

/* TODO : implement it to read both DSA and RSA at once */
PRIVATE_KEY  *privatekey_from_file(SSH_SESSION *session,char *filename,int type,char *passphrase){
    FILE *file=fopen(filename,"r");
    PRIVATE_KEY *privkey;

    gcry_sexp_t dsa=NULL;
    gcry_sexp_t rsa=NULL;
    int valid;

    

    if(!file){
        ssh_set_error(session,1,"Error opening %s : %s",filename,strerror(errno));
        return NULL;
    }
    if(type==1){
        if(!passphrase){
            if(session && session->options->passphrase_function)

                valid = read_dsa_privatekey(file,&dsa, session->options->passphrase_function,"DSA private key");
            else
                valid = read_dsa_privatekey(file,&dsa,(void *)default_get_password, "DSA private key");
        }
        else
            valid = read_dsa_privatekey(file,&dsa,(void *)get_password_specified,passphrase);
        fclose(file);
        if(!valid){
            ssh_set_error(session,2,"parsing private key %s",filename);

            
                
        
        
            
        
        
            
                

        return NULL;
        }
    }
    else if (type==2){
        if(!passphrase){
            if(session && session->options->passphrase_function)

                valid = read_rsa_privatekey(file,&rsa, session->options->passphrase_function,"RSA private key");
            else
                valid = read_rsa_privatekey(file,&rsa,(void *)default_get_password, "RSA private key");
        }
        else
            valid = read_rsa_privatekey(file,&rsa,(void *)get_password_specified,passphrase);
        fclose(file);
        if(!valid){
            ssh_set_error(session,2,"parsing private key %s",filename);

            
                
        
        
            
        
        
            
                

        return NULL;
        }
    } else {
        ssh_set_error(session,2,"Invalid private key type %d",type);
        return NULL;
    }    
    
    privkey=malloc(sizeof(PRIVATE_KEY));
    privkey->type=type;
    privkey->dsa_priv=dsa;
    privkey->rsa_priv=rsa;
    return privkey;
}

/* same that privatekey_from_file() but without any passphrase things. */
PRIVATE_KEY  *_privatekey_from_file(void *session,char *filename,int type){
    FILE *file=fopen(filename,"r");
    PRIVATE_KEY *privkey;

    gcry_sexp_t dsa=NULL;
    gcry_sexp_t rsa=NULL;
    int valid;

    

    if(!file){
        ssh_set_error(session,1,"Error opening %s : %s",filename,strerror(errno));
        return NULL;
    }
    if(type==1){

        valid=read_dsa_privatekey(file,&dsa,NULL,NULL);
        fclose(file);
        if(!valid){
            ssh_set_error(session,2,"parsing private key %s"
                    ,filename);

        
        
            
                    

            return NULL;
        }
    }
    else if (type==2){

        valid=read_rsa_privatekey(file,&rsa,NULL,NULL);
        fclose(file);
        if(!valid){
            ssh_set_error(session,2,"parsing private key %s"
                    ,filename);

        
        
            
                    

            return NULL;
        }
    } else {
        ssh_set_error(session,2,"Invalid private key type %d",type);
        return NULL;
    }
    privkey=malloc(sizeof(PRIVATE_KEY));
    privkey->type=type;
    privkey->dsa_priv=dsa;
    privkey->rsa_priv=rsa;
    return privkey;
}

void private_key_free(PRIVATE_KEY *prv){

    if(prv->dsa_priv)
        gcry_sexp_release(prv->dsa_priv);
    if(prv->rsa_priv)
        gcry_sexp_release(prv->rsa_priv);

        
    
        

    memset(prv,0,sizeof(PRIVATE_KEY));
    free(prv);
}

STRING *publickey_from_file(SSH_SESSION *session,char *filename,int *_type){
    BUFFER *buffer;
    int type;
    STRING *str;
    char buf[4096]; /* noone will have bigger keys that that */
                        /* where have i head that again ? */
    int fd=open(filename,O_RDONLY);
    int r;
    char *ptr;
    if(fd<0){
        ssh_set_error(session,1,"nonexistent public key file");
        return NULL;
    }
    if(read(fd,buf,8)!=8){
        close(fd);
        ssh_set_error(session,1,"Invalid public key file");
        return NULL;
    }
    buf[7]=0;
    if(!strcmp(buf,"ssh-dss"))
        type=1;
    else if (!strcmp(buf,"ssh-rsa"))
        type=2;
    else {
        close(fd);
        ssh_set_error(session,1,"Invalid public key file");
        return NULL;
    }
    r=read(fd,buf,sizeof(buf)-1);
    close(fd);
    if(r<=0){
        ssh_set_error(session,1,"Invalid public key file");
        return NULL;
    }
    buf[r]=0;
    ptr=strchr(buf,' ');
    if(ptr)
        *ptr=0; /* eliminates the garbage at end of file */
    buffer=base64_to_bin(buf);
    if(buffer){
        str=string_new(buffer_get_len(buffer));
        string_fill(str,buffer_get(buffer),buffer_get_len(buffer));
        buffer_free(buffer);
        if(_type)
            *_type=type;
        return str;
    } else {
        ssh_set_error(session,1,"Invalid public key file");
        return NULL; /* invalid file */
    }
}


/* why recursing ? i'll explain. on top, publickey_from_next_file will be executed until NULL returned */
/* we can't return null if one of the possible keys is wrong. we must test them before getting over */
STRING *publickey_from_next_file(SSH_SESSION *session,char **pub_keys_path,char **keys_path,
                            char **privkeyfile,int *type,int *count){
    static char *home=NULL;
    char public[256];
    char private[256];
    char *priv;
    char *pub;
    STRING *pubkey;
    if(!home)
        home=ssh_get_user_home_dir();
    if(home==NULL) {
        ssh_set_error(session,2,"User home dir impossible to guess");
        return NULL;
    }
    ssh_set_error(session,1,"no public key matched");
    if((pub=pub_keys_path[*count])==NULL)
        return NULL;
    if((priv=keys_path[*count])==NULL)
        return NULL;
    ++*count;
    /* are them readable ? */
    snprintf(public,256,pub,home);
    ssh_say(2,"Trying to open %s\n",public);
    if(!ssh_file_readaccess_ok(public)){
        ssh_say(2,"Failed\n");
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    } 
    snprintf(private,256,priv,home);
    ssh_say(2,"Trying to open %s\n",private);
    if(!ssh_file_readaccess_ok(private)){
        ssh_say(2,"Failed\n");
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    }
    ssh_say(2,"Okay both files ok\n");
    /* ok, we are sure both the priv8 and public key files are readable : we return the public one as a string,
        and the private filename in arguments */
    pubkey=publickey_from_file(session,public,type);
    if(!pubkey){
        ssh_say(2,"Wasn't able to open public key file %s : %s\n",public,ssh_get_error(session));
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    }
    *privkeyfile=realloc(*privkeyfile,strlen(private)+1);
    strcpy(*privkeyfile,private);
    return pubkey;
}

#define FOUND_OTHER ( (void *)-1)
#define FILE_NOT_FOUND ((void *)-2)
/* will return a token array containing [host,]ip keytype key */
/* NULL if no match was found, FOUND_OTHER if the match is on an other */
/* type of key (ie dsa if type was rsa) */
static char **ssh_parse_knownhost(char *filename, char *hostname, char *type){
    FILE *file=fopen(filename,"r");
    char buffer[4096];
    char *ptr;
    char **tokens;
    char **ret=NULL;
    if(!file)
        return ((void *)-2);
    while(fgets(buffer,sizeof(buffer),file)){
        ptr=strchr(buffer,'\n');
        if(ptr) *ptr=0;
        if((ptr=strchr(buffer,'\r'))) *ptr=0;
        if(!buffer[0])
            continue; /* skip empty lines */
        tokens=space_tokenize(buffer);
        if(!tokens[0] || !tokens[1] || !tokens[2]){
            /* it should have exactly 3 tokens */
            free(tokens[0]);
            free(tokens);
            continue;
        }
        if(tokens[3]){
            /* 3 tokens only, not four */
            free(tokens[0]);
            free(tokens);
            continue;
        }
        ptr=tokens[0];
        while(*ptr==' ')
            ptr++; /* skip the initial spaces */
        /* we allow spaces or ',' to follow the hostname. It's generaly an IP */
        /* we don't care about ip, if the host key match there is no problem with ip */
        if(strncasecmp(ptr,hostname,strlen(hostname))==0){
            if(ptr[strlen(hostname)]==' ' || ptr[strlen(hostname)]=='\0' 
                    || ptr[strlen(hostname)]==','){
                if(strcasecmp(tokens[1],type)==0){
                    fclose(file);
                    return tokens;
                } else {
                    ret=( (void *)-1);
                }
            }
        }
        /* not the good one */
        free(tokens[0]);
        free(tokens);
    }
    fclose(file);
    /* we did not find */
    return ret;
}

/* public function to test if the server is known or not */
int ssh_is_server_known(SSH_SESSION *session){
    char *pubkey_64;
    BUFFER *pubkey_buffer;
    STRING *pubkey=session->current_crypto->server_pubkey;
    char **tokens;
    ssh_options_default_known_hosts_file(session->options);
    if(!session->options->host){
        ssh_set_error(session,2,"Can't verify host in known hosts if the hostname isn't known");
        return -1;
    }
    tokens=ssh_parse_knownhost(session->options->known_hosts_file,
        session->options->host,session->current_crypto->server_pubkey_type);
    if(tokens==NULL)
        return 0;
    if(tokens==( (void *)-1))
        return 3;
    if(tokens==((void *)-2)){
        ssh_set_error(session,2,"verifying that server is a known host : file %s not found",session->options->known_hosts_file);
        return -1;
    }
    /* ok we found some public key in known hosts file. now un-base64it */
    /* Some time, we may verify the IP address did not change. I honestly think */
    /* it's not an important matter as IP address are known not to be secure */
    /* and the crypto stuff is enough to prove the server's identity */
    pubkey_64=tokens[2];
    pubkey_buffer=base64_to_bin(pubkey_64);
    /* at this point, we may free the tokens */
    free(tokens[0]);
    free(tokens);
    if(!pubkey_buffer){
        ssh_set_error(session,2,"verifying that server is a known host : base 64 error");
        return -1;
    }
    if(buffer_get_len(pubkey_buffer)!=string_len(pubkey)){
        buffer_free(pubkey_buffer);
        return 2;
    }
    /* now test that they are identical */
    if(memcmp(buffer_get(pubkey_buffer),pubkey->string,buffer_get_len(pubkey_buffer))!=0){
        buffer_free(pubkey_buffer);
        return 2;
    }
    buffer_free(pubkey_buffer);
    return 1;
}

int ssh_write_knownhost(SSH_SESSION *session){
    unsigned char *pubkey_64;
    STRING *pubkey=session->current_crypto->server_pubkey;
    char buffer[4096];
    FILE *file;
    ssh_options_default_known_hosts_file(session->options);
    if(!session->options->host){
        ssh_set_error(session,2,"Cannot write host in known hosts if the hostname is unknown");
        return -1;
    }
    /* a = append only */
    file=fopen(session->options->known_hosts_file,"a");
    if(!file){
        ssh_set_error(session,2,"Opening known host file %s for appending : %s",
        session->options->known_hosts_file,strerror(errno));
        return -1;
    }
    pubkey_64=bin_to_base64(pubkey->string,string_len(pubkey));
    snprintf(buffer,sizeof(buffer),"%s %s %s\n",session->options->host,session->current_crypto->server_pubkey_type,pubkey_64);
    free(pubkey_64);
    fwrite(buffer,strlen(buffer),1,file);
    fclose(file);
    return 0;
}
