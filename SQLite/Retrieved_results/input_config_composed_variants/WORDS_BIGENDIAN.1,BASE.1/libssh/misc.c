/* misc.c */
/* some misc routines than aren't really part of the ssh protocols but can be useful to the client */

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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <netdb.h>
#include "libssh/libssh.h"

/* if the program was executed suid root, don't trust the user ! */
static int is_trusted(){
    if(geteuid()!=getuid())
        return 0;
    return 1;
}

static char *get_homedir_from_uid(int uid){
    struct passwd *pwd;
    char *home;
    while((pwd=getpwent())){
        if(pwd->pw_uid == uid){
            home=strdup(pwd->pw_dir);
            endpwent();
            return home;
        }
    }
    endpwent();
    return NULL;
}

static char *get_homedir_from_login(char *user){
    struct passwd *pwd;
    char *home;
    while((pwd=getpwent())){
        if(!strcmp(pwd->pw_name,user)){
            home=strdup(pwd->pw_dir);
            endpwent();
            return home;
        }
    }
    endpwent();
    return NULL;
}
        
char *ssh_get_user_home_dir(){
    char *home;
    char *user;
    int trusted=is_trusted();
    if(trusted){
        if((home=getenv("HOME")))
            return strdup(home);
        if((user=getenv("USER")))
            return get_homedir_from_login(user);
    }
    return get_homedir_from_uid(getuid());
}

/* we have read access on file */
int ssh_file_readaccess_ok(char *file){
    if(!access(file,R_OK))
        return 1;
    return 0;
}


u64 ntohll(u64 a){

    return a;

    
    
    
    
    

}
