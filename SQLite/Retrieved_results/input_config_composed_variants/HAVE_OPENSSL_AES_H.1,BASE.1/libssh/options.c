/* options.c */
/* handle pre-connection options */
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include "libssh/priv.h"

/* by default, ssh1 support is not allowed */
SSH_OPTIONS *ssh_options_new(){
    SSH_OPTIONS *option=malloc(sizeof(SSH_OPTIONS));
    memset(option,0,sizeof(SSH_OPTIONS));
    option->port=22; /* set the default port */
    option->fd=-1;
    option->ssh2allowed=1;
    option->ssh1allowed=0;
    return option;
}

void ssh_options_set_port(SSH_OPTIONS *opt, unsigned int port){
    opt->port=port&0xffff;
}
SSH_OPTIONS *ssh_options_copy(SSH_OPTIONS *opt){
	SSH_OPTIONS *ret=ssh_options_new();    
    int i;
	ret->fd=opt->fd;
    ret->port=opt->port;
    if(opt->username)
    	ret->username=strdup(opt->username);
    if(opt->host)
    	ret->host=strdup(opt->host);
    if(opt->bindaddr)
    	ret->host=strdup(opt->bindaddr);
    if(opt->identity)
    	ret->identity=strdup(opt->identity);
    if(opt->ssh_dir)
        ret->ssh_dir=strdup(opt->ssh_dir);
	if(opt->known_hosts_file)
		ret->known_hosts_file=strdup(opt->known_hosts_file);
	for(i=0;i<10;++i)
		if(opt->wanted_methods[i])
			ret->wanted_methods[i]=strdup(opt->wanted_methods[i]);
	ret->passphrase_function=opt->passphrase_function;
	ret->connect_status_function=opt->connect_status_function;
	ret->connect_status_arg=opt->connect_status_arg;
	ret->timeout=opt->timeout;
	ret->timeout_usec=opt->timeout_usec;
    ret->ssh2allowed=opt->ssh2allowed;
    ret->ssh1allowed=opt->ssh1allowed;
	return ret;
}

void ssh_options_free(SSH_OPTIONS *opt){
    int i;
    if(opt->username)
        free(opt->username);
    if(opt->identity)
        free(opt->identity);
    /* we don't touch the banner. if the implementation did use it, they have to free it */
    if(opt->host)
        free(opt->host);
    if(opt->bindaddr)
        free(opt->bindaddr);
    if(opt->ssh_dir)
        free(opt->ssh_dir);
    for(i=0;i<10;i++)
        if(opt->wanted_methods[i])
            free(opt->wanted_methods[i]);
    memset(opt,0,sizeof(SSH_OPTIONS));
    free(opt);
}


void ssh_options_set_host(SSH_OPTIONS *opt, const char *hostname){
    char *ptr=strdup(hostname);
    char *ptr2=strchr(ptr,'@');
    if(opt->host) // don't leak memory
        free(opt->host);
    if(ptr2){
        *ptr2=0;
        opt->host=strdup(ptr2+1);
        if(opt->username)
            free(opt->username);
        opt->username=strdup(ptr);
        free(ptr);
    } else
        opt->host=ptr;
}

void ssh_options_set_username(SSH_OPTIONS *opt, char *username){
    if(opt->username)
        free(opt->username);
    opt->username=strdup(username);
}

void ssh_options_set_fd(SSH_OPTIONS *opt, int fd){
    opt->fd=fd;
}

void ssh_options_set_bind(SSH_OPTIONS *opt, char *bindaddr,int port){
    opt->bindaddr=strdup(bindaddr);
    opt->bindport=port;
}

void ssh_options_set_ssh_dir(SSH_OPTIONS *opt, char *dir){
    char buffer[1024];
    snprintf(buffer,1024,dir,ssh_get_user_home_dir());
    opt->ssh_dir=strdup(buffer);
}

void ssh_options_set_known_hosts_file(SSH_OPTIONS *opt, char *dir){
    char buffer[1024];
    snprintf(buffer,1024,dir,ssh_get_user_home_dir());
    opt->known_hosts_file=strdup(buffer);
}

void ssh_options_set_identity(SSH_OPTIONS *opt, char *identity){
    char buffer[1024];
    snprintf(buffer,1024,identity,ssh_get_user_home_dir());
    opt->identity=strdup(buffer);
}

void ssh_options_set_banner(SSH_OPTIONS *opt, char *banner){
    if(opt->banner)
        free(opt->banner);
    opt->banner=strdup(banner);
}

/* what's the deal here ? some options MUST be set before authentication or key exchange,
 * otherwise default values are going to be used. what must be configurable :
 * Public key certification method *
 * key exchange method (dh-sha1 for instance)*
 * c->s, s->c ciphers *
 * c->s s->c macs *
 * c->s s->c compression */

/* they all return 0 if all went well, 1 or !=0 if not. the most common error is unmatched algo (unimplemented) */
/* don't forget other errors can happen if no matching algo is found in sshd answer */

int ssh_options_set_wanted_algos(SSH_OPTIONS *opt,int algo, char *list){
    if(algo > 9 || algo < 0){
        ssh_set_error(NULL,1,"algo %d out of range",algo);
        return -1;
    }
    if( (!opt->use_nonexisting_algo) && !verify_existing_algo(algo,list)){
        ssh_set_error(NULL,1,"Setting method : no algorithm "
                "for method \"%s\" (%s)\n",ssh_kex_nums[algo],list);
        return -1;
    }
    if(opt->wanted_methods[algo])
        free(opt->wanted_methods[algo]);
    opt->wanted_methods[algo]=strdup(list);    
    return 0;
}

static char *get_username_from_uid(int uid){
    struct passwd *pwd;
    char *user;
    while((pwd=getpwent())){
        if(pwd->pw_uid == uid){
            user=strdup(pwd->pw_name);
            endpwent();
            return user;
        }
    }
    endpwent();
    ssh_set_error(NULL,2,"uid %d doesn't exist !",uid);
    return NULL;
}

/* this function must be called when no specific username has been asked. it has to guess it */
int ssh_options_default_username(SSH_OPTIONS *opt){
    char *user;
    if(opt->username)
        return 0;
    user=getenv("USER");
    if(user){
        opt->username=strdup(user);
        return 0;
    }
    user=get_username_from_uid(getuid());
    if(user){
        opt->username=user;
        return 0;
    }
    return -1;
}

int ssh_options_default_ssh_dir(SSH_OPTIONS *opt){
    char buffer[256];
    if(opt->ssh_dir)
        return 0;
    snprintf(buffer,256,"%s/.ssh/",ssh_get_user_home_dir());
    opt->ssh_dir=strdup(buffer);
    return 0;
}

int ssh_options_default_known_hosts_file(SSH_OPTIONS *opt){
    char buffer[1024];
    if(opt->known_hosts_file)
        return 0;
    ssh_options_default_ssh_dir(opt);
    snprintf(buffer,1024,"%s/known_hosts",opt->ssh_dir);
    opt->known_hosts_file=strdup(buffer);
    return 0;
}

void ssh_options_set_status_callback(SSH_OPTIONS *opt, void (*callback)(void *arg, float status), void *arg ){
    opt->connect_status_function=callback;
    opt->connect_status_arg=arg;
}

void ssh_options_set_timeout(SSH_OPTIONS *opt, long seconds,long usec){
    opt->timeout=seconds;
    opt->timeout_usec=usec;
}

void ssh_options_allow_ssh1(SSH_OPTIONS *opt, int allow){
    if(allow)
        opt->ssh1allowed=1;
    else
        opt->ssh1allowed=0;
}

void ssh_options_allow_ssh2(SSH_OPTIONS *opt, int allow){
    if(allow)
        opt->ssh2allowed=1;
    else
        opt->ssh2allowed=0;
}

int ssh_options_getopt(SSH_OPTIONS *options, int *argcptr, char **argv){
    int i;
    int argc=*argcptr;
    char *user=NULL;
    int port=22;
    int debuglevel=0;
    int usersa=0;
    int usedss=0;
    int compress=0;
    int cont=1;
    char *cipher=NULL;
    char *localaddr=NULL;
    char *identity=NULL;
    char **save=malloc(argc * sizeof(char *));
    int current=0;
    int ssh1=0;
    int ssh2=1;
    
    int saveoptind=optind; /* need to save 'em */
    int saveopterr=opterr;
    opterr=0; /* shut up getopt */
    while(cont && ((i=getopt(argc,argv,"c:i:Cl:p:vb:rd12"))!=-1)){

        switch(i){
            case 'l':
                user=optarg;
                break;
            case 'p':
                port=atoi(optarg)&0xffff;
                break;
            case 'v':
                debuglevel++;
                break;
            case 'r':
                usersa++;
                break;
            case 'd':
                usedss++;
                break;
            case 'c':
                cipher=optarg;
                break;
            case 'i':
                identity=optarg;
                break;
            case 'b':
                localaddr=optarg;
                break;
            case 'C':
                compress++;
                break;
            case '2':
                ssh2=1;
                ssh1=0;
                break;
            case '1':
                ssh2=0;
                ssh1=1;
                break;
            default:
                {
                char opt[3]="- ";
                opt[1]=optopt;
                save[current++]=strdup(opt);
                if(optarg)
                    save[current++]=argv[optind+1];
            }
        }
    }
    opterr=saveopterr;
    while(optind < argc)
        save[current++]=argv[optind++];
        
    if(usersa && usedss){
        ssh_set_error(NULL,2,"either RSA or DSS must be chosen");
        cont=0;
    }
    ssh_set_verbosity(debuglevel);
    optind=saveoptind;
    if(!cont){
        free(save);
        return -1;
    }
    /* first recopy the save vector into original's */
    for(i=0;i<current;i++)
        argv[i+1]=save[i]; // don't erase argv[0]
    argv[current+1]=NULL;
    *argcptr=current+1;
    free(save);
    /* set a new option struct */
    if(compress){
        if(ssh_options_set_wanted_algos(options,6,"zlib"))
            cont=0;
        if(ssh_options_set_wanted_algos(options,7,"zlib"))
            cont=0;
    }
    if(cont &&cipher){
        if(ssh_options_set_wanted_algos(options,2,cipher))
            cont=0;
        if(cont && ssh_options_set_wanted_algos(options,3,cipher))
            cont=0;
    }
    if(cont && usersa)
        if(ssh_options_set_wanted_algos(options,1,"ssh-rsa"))
            cont=0;
    if(cont && usedss)
        if(ssh_options_set_wanted_algos(options,1,"ssh-dss"))
            cont=0;
    if(cont && user)
        ssh_options_set_username(options,user);
    if(cont && identity)
        ssh_options_set_identity(options,identity);
    if(cont && localaddr)
        ssh_options_set_bind(options,localaddr,0);
    ssh_options_set_port(options,port);
    if(ssh1){
        ssh_options_allow_ssh1(options,1);
        ssh_options_allow_ssh2(options,0);
    } else { // default behaviour 
        ssh_options_allow_ssh1(options,0);
        ssh_options_allow_ssh2(options,1);
    }
        
    if(!cont){
        return -1;
    } else
        return 0 ;   
}
