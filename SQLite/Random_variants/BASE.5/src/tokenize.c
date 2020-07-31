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
** An tokenizer for SQL
**
** This file contains C code that splits an SQL input string up into
** individual tokens and sends those tokens one-by-one over to the
** parser for analysis.
**
** $Id: tokenize.c,v 1.1 2000/05/29 14:26:02 drh Exp $
*/
#include "sqliteInt.h"
#include <ctype.h>

/*
** All the keywords of the SQL language are stored as in a hash
** table composed of instances of the following structure.
*/
typedef struct Keyword Keyword;
struct Keyword {
  char *zName;             /* The keyword name */
  int len;                 /* Number of characters in the keyword */
  int tokenType;           /* The token value for this keyword */
  Keyword *pNext;          /* Next keyword with the same hash */
};

/*
** These are the keywords
*/
static Keyword aKeywordTable[] = {
  { "AND",               0, TK_AND,              0 },
  { "AS",                0, TK_AS,               0 },
  { "ASC",               0, TK_ASC,              0 },
  { "BY",                0, TK_BY,               0 },
  { "CHECK",             0, TK_CHECK,            0 },
  { "CONSTRAINT",        0, TK_CONSTRAINT,       0 },
  { "CREATE",            0, TK_CREATE,           0 },
  { "DEFAULT",           0, TK_DEFAULT,          0 },
  { "DELETE",            0, TK_DELETE,           0 },
  { "DESC",              0, TK_DESC,             0 },
  { "DROP",              0, TK_DROP,             0 },
  { "EXPLAIN",           0, TK_EXPLAIN,          0 },
  { "FROM",              0, TK_FROM,             0 },
  { "INDEX",             0, TK_INDEX,            0 },
  { "INSERT",            0, TK_INSERT,           0 },
  { "INTO",              0, TK_INTO,             0 },
  { "IS",                0, TK_IS,               0 },
  { "ISNULL",            0, TK_ISNULL,           0 },
  { "KEY",               0, TK_KEY,              0 },
  { "NOT",               0, TK_NOT,              0 },
  { "NOTNULL",           0, TK_NOTNULL,          0 },
  { "NULL",              0, TK_NULL,             0 },
  { "ON",                0, TK_ON,               0 },
  { "OR",                0, TK_OR,               0 },
  { "ORDER",             0, TK_ORDER,            0 },
  { "PRIMARY",           0, TK_PRIMARY,          0 },
  { "SELECT",            0, TK_SELECT,           0 },
  { "SET",               0, TK_SET,              0 },
  { "TABLE",             0, TK_TABLE,            0 },
  { "UNIQUE",            0, TK_UNIQUE,           0 },
  { "UPDATE",            0, TK_UPDATE,           0 },
  { "VALUES",            0, TK_VALUES,           0 },
  { "WHERE",             0, TK_WHERE,            0 },
};

/*
** This is the hash table
*/
#define KEY_HASH_SIZE 37
static Keyword *apHashTable[37];


/*
** This function looks up an identifier to determine if it is a
** keyword.  If it is a keyword, the token code of that keyword is 
** returned.  If the input is not a keyword, TK_ID is returned.
*/
static int sqliteKeywordCode(const char *z, int n){
  int h;
  Keyword *p;
  if( aKeywordTable[0].len==0 ){
    /* Initialize the keyword hash table */
    int i;
    int n;
    n = sizeof(aKeywordTable)/sizeof(aKeywordTable[0]);
    for(i=0; i<n; i++){
      aKeywordTable[i].len = strlen(aKeywordTable[i].zName);
      h = sqliteHashNoCase(aKeywordTable[i].zName, aKeywordTable[i].len);
      h %= 37;
      aKeywordTable[i].pNext = apHashTable[h];
      apHashTable[h] = &aKeywordTable[i];
    }
  }
  h = sqliteHashNoCase(z, n) % 37;
  for(p=apHashTable[h]; p; p=p->pNext){
    if( p->len==n && sqliteStrNICmp(p->zName, z, n)==0 ){
      return p->tokenType;
    }
  }
  return TK_ID;
}

/*
** Return the length of the token that begins at z[0].  Return
** -1 if the token is (or might be) incomplete.  Store the token
** type in *tokenType before returning.
*/
int sqliteGetToken(const char *z, int *tokenType){
  int i;
  switch( *z ){
    case ' ': case '\t': case '\n': case '\f': {
      for(i=1; z[i] && isspace(z[i]); i++){}
      *tokenType = TK_SPACE;
      return i;
    }
    case '-': {
      if( z[1]==0 ) return -1;
      if( z[1]=='-' ){
        for(i=2; z[i] && z[i]!='\n'; i++){}
        *tokenType = TK_COMMENT;
        return i;
      }
      *tokenType = TK_MINUS;
      return 1;
    }
    case '(': {
      *tokenType = TK_LP;
      return 1;
    }
    case ')': {
      *tokenType = TK_RP;
      return 1;
    }
    case ';': {
      *tokenType = TK_SEMI;
      return 1;
    }
    case '+': {
      *tokenType = TK_PLUS;
      return 1;
    }
    case '*': {
      *tokenType = TK_STAR;
      return 1;
    }
    case '/': {
      *tokenType = TK_SLASH;
      return 1;
    }
    case '=': {
      *tokenType = TK_EQ;
      return 1 + (z[1]=='=');
    }
    case '<': {
      if( z[1]=='=' ){
        *tokenType = TK_LE;
        return 2;
      }else if( z[1]=='>' ){
        *tokenType = TK_NE;
        return 2;
      }else{
        *tokenType = TK_LT;
        return 1;
      }
    }
    case '>': {
      if( z[1]=='=' ){
        *tokenType = TK_GE;
        return 2;
      }else{
        *tokenType = TK_GT;
        return 1;
      }
    }
    case '!': {
      if( z[1]!='=' ){
        *tokenType = TK_ILLEGAL;
        return 1;
      }else{
        *tokenType = TK_NE;
        return 2;
      }
    }
    case ',': {
      *tokenType = TK_COMMA;
      return 1;
    }
    case '\'': case '"': {
      int delim = z[0];
      for(i=1; z[i]; i++){
        if( z[i]==delim ){
          if( z[i+1]==delim ){
            i++;
          }else{
            break;
          }
        }
      }
      if( z[i] ) i++;
      *tokenType = TK_STRING;
      return i;
    }
    case '.': {
      if( !isdigit(z[1]) ){
        *tokenType = TK_DOT;
        return 1;
      }
      /* Fall thru into the next case */
    }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      for(i=1; z[i] && isdigit(z[i]); i++){}
      if( z[i]=='.' ){
        i++;
        while( z[i] && isdigit(z[i]) ){ i++; }
        if( (z[i]=='e' || z[i]=='E') &&
           ( isdigit(z[i+1]) 
            || ((z[i+1]=='+' || z[i+1]=='-') && isdigit(z[i+2]))
           )
        ){
          i += 2;
          while( z[i] && isdigit(z[i]) ){ i++; }
        }
        *tokenType = TK_FLOAT;
      }else if( z[0]=='.' ){
        *tokenType = TK_FLOAT;
      }else{
        *tokenType = TK_INTEGER;
      }
      return i;
    }
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '_':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': {
      for(i=1; z[i] && (isalnum(z[i]) || z[i]=='_'); i++){}
      *tokenType = sqliteKeywordCode(z, i);
      return i;
    }
    default: {
      break;
    }
  }
  *tokenType = TK_ILLEGAL;
  return 1;
}

/*
** Run the parser on the given SQL string.  The parser structure is
** passed in.  Return the number of errors.
*/
int sqliteRunParser(Parse *pParse, char *zSql, char **pzErrMsg){
  int nErr = 0;
  int i;
  void *pEngine;
  int once = 1;
  static FILE *trace = 0;
  extern void *sqliteParserAlloc(void*(*)(int));
  extern void sqliteParserFree(void*, void(*)(void*));
  extern int sqliteParser(void*, int, ...);
  extern void sqliteParserTrace(FILE*, char *);

  i = 0;
  pEngine = sqliteParserAlloc(sqliteMalloc);
  if( pEngine==0 ){
    sqliteSetString(pzErrMsg, "out of memory", 0);
    return 1;
  }
  sqliteParserTrace(trace, "parser: ");
  while( nErr==0 && i>=0 && zSql[i]!=0 ){
    int tokenType;
    
    pParse->sLastToken.z = &zSql[i];
    pParse->sLastToken.n = sqliteGetToken(&zSql[i], &tokenType);
    i += pParse->sLastToken.n;
    if( once ){
      pParse->sFirstToken = pParse->sLastToken;
      once = 0;
    }
    switch( tokenType ){
      case TK_SPACE:
        break;
      case TK_COMMENT: {
        /* Various debugging modes can be turned on and off using
        ** special SQL comments.  Check for the special comments
        ** here and take approriate action if found.
        */
        char *z = pParse->sLastToken.z;
        if( sqliteStrNICmp(z,"--parser-trace-on--",19)==0 ){
          trace = stderr;
          sqliteParserTrace(trace, "parser: ");
        }else if( sqliteStrNICmp(z,"--parser-trace-off--", 20)==0 ){
          trace = 0;
          sqliteParserTrace(trace, "parser: ");
        }else if( sqliteStrNICmp(z,"--vdbe-trace-on--",17)==0 ){
          pParse->db->flags |= 0x00000001;
        }else if( sqliteStrNICmp(z,"--vdbe-trace-off--", 19)==0 ){
          pParse->db->flags &= ~0x00000001;
        }
        break;
      }
      case TK_ILLEGAL:
        sqliteSetNString(pzErrMsg, "illegal token: \"", -1, 
           pParse->sLastToken.z, pParse->sLastToken.n, 0);
        nErr++;
        break;
      default:
        sqliteParser(pEngine, tokenType, pParse->sLastToken, pParse);
        if( pParse->zErrMsg ){
          sqliteSetNString(pzErrMsg, "near \"", -1, 
             pParse->sErrToken.z, pParse->sErrToken.n,
             "\": ", -1,
             pParse->zErrMsg, -1,
             0);
          nErr++;
        }
        break;
    }
  }
  if( nErr==0 ){
    sqliteParser(pEngine, 0, pParse->sLastToken, pParse);
    if( pParse->zErrMsg ){
       sqliteSetNString(pzErrMsg, "near \"", -1, 
          pParse->sErrToken.z, pParse->sErrToken.n,
          "\": ", -1,
          pParse->zErrMsg, -1,
          0);
       nErr++;
    }
  }
  sqliteParserFree(pEngine, sqliteFree);
  if( pParse->zErrMsg ){
    if( pzErrMsg ){
      *pzErrMsg = pParse->zErrMsg;
    }else{
      sqliteFree(pParse->zErrMsg);
    }
    if( !nErr ) nErr++;
  }
  if( pParse->pVdbe ){
    sqliteVdbeDelete(pParse->pVdbe);
    pParse->pVdbe = 0;
  }
  if( pParse->pNewTable ){
    sqliteDeleteTable(pParse->db, pParse->pNewTable);
    pParse->pNewTable = 0;
  }
  return nErr;
}
