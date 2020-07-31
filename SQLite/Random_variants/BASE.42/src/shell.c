/*
** Copyright (c) 1999, 2000 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** You should have received a copy of the GNU General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*************************************************************************
** This file contains code to implement the "sqlite" command line
** utility for accessing SQLite databases.
**
** $Id: shell.c,v 1.6 2000/06/02 13:27:59 drh Exp $
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sqlite.h"
#include <unistd.h>
#include <ctype.h>


#include <readline/readline.h>
#include <readline/history.h>





/*
** This routine reads a line of text from standard input, stores
** the text in memory obtained from malloc() and returns a pointer
** to the text.  NULL is returned at end of file, or if malloc()
** fails.
**
** The interface is like "readline" but no command-line editing
** is done.
*/
static char *getline(char *zPrompt){
  char *zLine;
  int nLine;
  int n;
  int eol;

  if( zPrompt && *zPrompt ){
    printf("%s",zPrompt);
    fflush(stdout);
  }
  nLine = 100;
  zLine = malloc( nLine );
  if( zLine==0 ) return 0;
  n = 0;
  eol = 0;
  while( !eol ){
    if( n+100>nLine ){
      nLine = nLine*2 + 100;
      zLine = realloc(zLine, nLine);
      if( zLine==0 ) return 0;
    }
    if( fgets(&zLine[n], nLine - n, stdin)==0 ){
      if( n==0 ){
        free(zLine);
        return 0;
      }
      zLine[n] = 0;
      eol = 1;
      break;
    }
    while( zLine[n] ){ n++; }
    if( n>0 && zLine[n-1]=='\n' ){
      n--;
      zLine[n] = 0;
      eol = 1;
    }
  }
  zLine = realloc( zLine, n+1 );
  return zLine;
}

/*
** Retrieve a single line of input text.  "isatty" is true if text
** is coming from a terminal.  In that case, we issue a prompt and
** attempt to use "readline" for command-line editing.  If "isatty"
** is false, use "getline" instead of "readline" and issue to prompt.
**
** zPrior is a string of prior text retrieved.  If not the empty
** string, then issue a continuation prompt.
*/
static char *one_input_line(const char *zPrior, int isatty){
  char *zPrompt;
  char *zResult;
  if( !isatty ){
    return getline(0);
  }
  if( zPrior && zPrior[0] ){
    zPrompt = "   ...> ";
  }else{
    zPrompt = "sqlite> ";
  }
  zResult = readline(zPrompt);
  add_history(zResult);
  return zResult;
}

/*
** An pointer to an instance of this structure is passed from
** the main program to the callback.  This is used to communicate
** state and mode information.
*/
struct callback_data {
  int cnt;           /* Number of records displayed so far */
  FILE *out;         /* Write results here */
  int mode;          /* An output mode setting */
  int showHeader;    /* True to show column names in List or Column mode */
  char separator[20];/* Separator character for MODE_List */
  int colWidth[30];  /* Width of each column when in column mode */
};

/*
** These are the allowed modes.
*/
#define MODE_Line 0
#define MODE_Column 1
#define MODE_List 2
#define MODE_Html 3

/*
** Number of elements in an array
*/
#define ArraySize(X) (sizeof(X)/sizeof(X[0]))

/*
** This is the callback routine that the SQLite library
** invokes for each row of a query result.
*/
static int callback(void *pArg, int nArg, char **azArg, char **azCol){
  int i;
  struct callback_data *p = (struct callback_data*)pArg;
  switch( p->mode ){
    case 0: {
      if( p->cnt++>0 ) fprintf(p->out,"\n");
      for(i=0; i<nArg; i++){
        fprintf(p->out,"%s = %s\n", azCol[i], azArg[i]);
      }
      break;
    }
    case 1: {
      if( p->cnt++==0 && p->showHeader ){
        for(i=0; i<nArg; i++){
          int w;
          if( i<(sizeof(p->colWidth)/sizeof(p->colWidth[0])) && p->colWidth[i]>0 ){
             w = p->colWidth[i]; 
          }else{
             w = 10;
          }
          fprintf(p->out,"%-*.*s%s",w,w,azCol[i], i==nArg-1 ? "\n": "  ");
        }
        for(i=0; i<nArg; i++){
          int w;
          if( i<(sizeof(p->colWidth)/sizeof(p->colWidth[0])) && p->colWidth[i]>0 ){
             w = p->colWidth[i];
          }else{
             w = 10;
          }
          fprintf(p->out,"%-*.*s%s",w,w,"-------------------------------------"
                 "------------------------------------------------------------",
                  i==nArg-1 ? "\n": "  ");
        }
      }
      for(i=0; i<nArg; i++){
        int w;
        if( i<(sizeof(p->colWidth)/sizeof(p->colWidth[0])) && p->colWidth[i]>0 ){
           w = p->colWidth[i];
        }else{
           w = 10;
        }
        fprintf(p->out,"%-*.*s%s",w,w,azArg[i], i==nArg-1 ? "\n": "  ");
      }
      break;
    }
    case 2: {
      if( p->cnt++==0 && p->showHeader ){
        for(i=0; i<nArg; i++){
          fprintf(p->out,"%s%s",azCol[i], i==nArg-1 ? "\n" : p->separator);
        }
      }
      for(i=0; i<nArg; i++){
        fprintf(p->out,"%s%s",azArg[i], i==nArg-1 ? "\n" : p->separator);
      }
      break;
    }
    case 3: {
      if( p->cnt++==0 && p->showHeader ){
        fprintf(p->out,"<TR>");
        for(i=0; i<nArg; i++){
          fprintf(p->out,"<TH>%s</TH>",azCol[i]);
        }
        fprintf(p->out,"</TR>\n");
      }
      for(i=0; i<nArg; i++){
        fprintf(p->out,"<TR>");
        for(i=0; i<nArg; i++){
          fprintf(p->out,"<TD>%s</TD>",azArg[i]);
        }
        fprintf(p->out,"</TD>\n");
      }
      break;
    }
  }      
  return 0;
}

/*
** Text of a help message
*/
static char zHelp[] = 
  ".exit                  Exit this program\n"
  ".explain               Set output mode suitable for EXPLAIN\n"
  ".header ON|OFF         Turn display of headers on or off\n"
  ".help                  Show this message\n"
  ".indices TABLE         Show names of all indices on TABLE\n"
  ".mode MODE             Set mode to one of \"line\", \"column\", "
                                      "\"list\", or \"html\"\n"
  ".output FILENAME       Send output to FILENAME\n"
  ".output stdout         Send output to the screen\n"
  ".schema ?TABLE?        Show the CREATE statements\n"
  ".separator STRING      Change separator string for \"list\" mode\n"
  ".tables                List names all tables in the database\n"
  ".width NUM NUM ...     Set column widths for \"column\" mode\n"
;

/*
** If an input line begins with "." then invoke this routine to
** process that line.
*/
static void do_meta_command(char *zLine, sqlite *db, struct callback_data *p){
  int i = 1;
  int nArg = 0;
  int n, c;
  char *azArg[50];

  /* Parse the input line into tokens.
  */
  while( zLine[i] && nArg<(sizeof(azArg)/sizeof(azArg[0])) ){
    while( isspace(zLine[i]) ){ i++; }
    if( zLine[i]=='\'' || zLine[i]=='"' ){
      int delim = zLine[i++];
      azArg[nArg++] = &zLine[i];
      while( zLine[i] && zLine[i]!=delim ){ i++; }
      if( zLine[i]==delim ){
        zLine[i++] = 0;
      }
    }else{
      azArg[nArg++] = &zLine[i];
      while( zLine[i] && !isspace(zLine[i]) ){ i++; }
      if( zLine[i] ) zLine[i++] = 0;
    }
  }

  /* Process the input line.
  */
  if( nArg==0 ) return;
  n = strlen(azArg[0]);
  c = azArg[0][0];

  if( c=='e' && strncmp(azArg[0], "exit", n)==0 ){
    exit(0);
  }else

  if( c=='e' && strncmp(azArg[0], "explain", n)==0 ){
    p->mode = 1;
    p->showHeader = 1;
    p->colWidth[0] = 4;
    p->colWidth[1] = 12;
    p->colWidth[2] = 5;
    p->colWidth[3] = 5;
    p->colWidth[4] = 40;
  }else

  if( c=='h' && strncmp(azArg[0], "header", n)==0 && nArg>1 ){
    int j;
    char *z = azArg[1];
    int val = atoi(azArg[1]);
    for(j=0; z[j]; j++){
      if( isupper(z[j]) ) z[j] = tolower(z[j]);
    }
    if( strcmp(z,"on")==0 ){
      val = 1;
    }else if( strcmp(z,"yes")==0 ){
      val = 1;
    } 
    p->showHeader = val;
  }else

  if( c=='h' && strncmp(azArg[0], "help", n)==0 ){
    fprintf(stderr,zHelp);
  }else

  if( c=='i' && strncmp(azArg[0], "indices", n)==0 && nArg>1 ){
    struct callback_data data;
    char *zErrMsg = 0;
    char zSql[1000];
    memcpy(&data, p, sizeof(data));
    data.showHeader = 0;
    data.mode = 2;
    sprintf(zSql, "SELECT name FROM sqlite_master "
                  "WHERE type='index' AND tbl_name LIKE '%.00s' "
                  "ORDER BY name", azArg[1]);
    sqlite_exec(db, zSql, callback, &data, &zErrMsg);
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      free(zErrMsg);
    }
  }else

  if( c=='m' && strncmp(azArg[0], "mode", n)==0 && nArg==2 ){
    int n2 = strlen(azArg[1]);
    if( strncmp(azArg[1],"line",n2)==0 ){
      p->mode = 0;
    }else if( strncmp(azArg[1],"column",n2)==0 ){
      p->mode = 1;
    }else if( strncmp(azArg[1],"list",n2)==0 ){
      p->mode = 2;
    }else if( strncmp(azArg[1],"html",n2)==0 ){
      p->mode = 3;
    }
  }else

  if( c=='o' && strncmp(azArg[0], "output", n)==0 && nArg==2 ){
    if( p->out!=stdout ){
      fclose(p->out);
    }
    if( strcmp(azArg[1],"stdout")==0 ){
      p->out = stdout;
    }else{
      p->out = fopen(azArg[1], "w");
      if( p->out==0 ){
        fprintf(stderr,"can't write to \"%s\"\n", azArg[1]);
        p->out = stdout;
      }
    }
  }else

  if( c=='s' && strncmp(azArg[0], "schema", n)==0 ){
    struct callback_data data;
    char *zErrMsg = 0;
    char zSql[1000];
    memcpy(&data, p, sizeof(data));
    data.showHeader = 0;
    data.mode = 2;
    if( nArg>1 ){
      sprintf(zSql, "SELECT sql FROM sqlite_master WHERE name LIKE '%.900s'",
         azArg[1]);
    }else{
      sprintf(zSql, "SELECT sql FROM sqlite_master "
         "ORDER BY tbl_name, type DESC, name");
    }
    sqlite_exec(db, zSql, callback, &data, &zErrMsg);
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      free(zErrMsg);
    }
  }else

  if( c=='s' && strncmp(azArg[0], "separator", n)==0 && nArg==2 ){
    sprintf(p->separator, "%.*s", (int)(sizeof(p->separator)/sizeof(p->separator[0]))-1, azArg[1]);
  }else

  if( c=='t' && strncmp(azArg[0], "tables", n)==0 ){
    struct callback_data data;
    char *zErrMsg = 0;
    static char zSql[] = 
      "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name";
    memcpy(&data, p, sizeof(data));
    data.showHeader = 0;
    data.mode = 2;
    sqlite_exec(db, zSql, callback, &data, &zErrMsg);
    if( zErrMsg ){
      fprintf(stderr,"Error: %s\n", zErrMsg);
      free(zErrMsg);
    }
  }else

  if( c=='w' && strncmp(azArg[0], "width", n)==0 ){
    int j;
    for(j=1; j<nArg && j<(sizeof(p->colWidth)/sizeof(p->colWidth[0])); j++){
      p->colWidth[j-1] = atoi(azArg[j]);
    }
  }else

  {
    fprintf(stderr, "unknown command: \"%s\". Enter \".help\" for help\n",
      azArg[0]);
  }
}

int main(int argc, char **argv){
  sqlite *db;
  char *zErrMsg = 0;
  char *argv0 = argv[0];
  struct callback_data data;

  memset(&data, 0, sizeof(data));
  data.mode = 2;
  strcpy(data.separator,"|");
  data.showHeader = 0;
  while( argc>=2 && argv[1][0]=='-' ){
    if( strcmp(argv[1],"-html")==0 ){
      data.mode = 3;
      argc--;
      argv++;
    }else if( strcmp(argv[1],"-list")==0 ){
      data.mode = 2;
      argc--;
      argv++;
    }else if( strcmp(argv[1],"-line")==0 ){
      data.mode = 0;
      argc--;
      argv++;
    }else if( argc>=3 && strcmp(argv[0],"-separator")==0 ){
      sprintf(data.separator,"%.*s",(int)sizeof(data.separator)-1,argv[2]);
      argc -= 2;
      argv += 2;
    }else if( strcmp(argv[1],"-header")==0 ){
      data.showHeader = 1;
      argc--;
      argv++;
    }else if( strcmp(argv[1],"-noheader")==0 ){
      data.showHeader = 0;
      argc--;
      argv++;
    }else{
      fprintf(stderr,"%s: unknown option: %s\n", argv0, argv[1]);
      return 1;
    }
  }
  if( argc!=2 && argc!=3 ){
    fprintf(stderr,"Usage: %s ?OPTIONS? FILENAME ?SQL?\n", argv0);
    exit(1);
  }
  db = sqlite_open(argv[1], 0666, &zErrMsg);
  if( db==0 ){
    fprintf(stderr,"Unable to open database \"%s\": %s\n", argv[1], zErrMsg);
    exit(1);
  }
  data.out = stdout;
  if( argc==3 ){
    if( sqlite_exec(db, argv[2], callback, &data, &zErrMsg)!=0 && zErrMsg!=0 ){
      fprintf(stderr,"SQL error: %s\n", zErrMsg);
      exit(1);
    }
  }else{
    char *zLine;
    char *zSql = 0;
    int nSql = 0;
    int istty = isatty(0);
    if( istty ){
      printf(
        "Enter \".help\" for instructions\n"
      );
    }
    while( (zLine = one_input_line(zSql, istty))!=0 ){
      if( zLine && zLine[0]=='.' ){
        do_meta_command(zLine, db, &data);
        free(zLine);
        continue;
      }
      if( zSql==0 ){
        nSql = strlen(zLine);
        zSql = malloc( nSql+1 );
        strcpy(zSql, zLine);
      }else{
        int len = strlen(zLine);
        zSql = realloc( zSql, nSql + len + 2 );
        if( zSql==0 ){
          fprintf(stderr,"%s: out of memory!\n", argv0);
          exit(1);
        }
        strcpy(&zSql[nSql++], "\n");
        strcpy(&zSql[nSql], zLine);
        nSql += len;
      }
      free(zLine);
      if( sqlite_complete(zSql) ){
        data.cnt = 0;
        if( sqlite_exec(db, zSql, callback, &data, &zErrMsg)!=0 
             && zErrMsg!=0 ){
          printf("SQL error: %s\n", zErrMsg);
          free(zErrMsg);
          zErrMsg = 0;
        }
        free(zSql);
        zSql = 0;
        nSql = 0;
      }
    }
  }
  sqlite_close(db);
  return 0;
}
