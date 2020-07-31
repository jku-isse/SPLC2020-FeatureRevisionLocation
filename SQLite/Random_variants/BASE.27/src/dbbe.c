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
** This file contains code to implement the database baseend (DBBE)
** for sqlite.  The database backend is the interface between
** sqlite and the code that does the actually reading and writing
** of information to the disk.
**
** This file uses GDBM as the database backend.  It should be
** relatively simple to convert to a different database such
** as NDBM, SDBM, or BerkeleyDB.
**
** $Id: dbbe.c,v 1.4 2000/05/31 20:00:52 drh Exp $
*/
#include "sqliteInt.h"
#include <gdbm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

/*
** Each open database file is an instance of this structure.
*/
typedef struct BeFile BeFile;
struct BeFile {
  char *zName;            /* Name of the file */
  GDBM_FILE dbf;          /* The file itself */
  int nRef;               /* Number of references */
  int delOnClose;         /* Delete when closing */
  BeFile *pNext, *pPrev;  /* Next and previous on list of open files */
};

/*
** The following are state variables for the RC4 algorithm.  We
** use RC4 as a random number generator.  Each call to RC4 gives
** a random 8-bit number.
*/
struct rc4 {
  int i, j;
  int s[256];
};

/*
** The complete database is an instance of the following structure.
*/
struct Dbbe {
  char *zDir;        /* The directory containing the database */
  int write;         /* True for write permission */
  BeFile *pOpen;     /* List of open files */
  int nTemp;         /* Number of temporary files created */
  FILE **apTemp;     /* Space to hold temporary file pointers */
  struct rc4 rc4;    /* The random number generator */
};

/*
** Each file within the database is an instance of this
** structure.
*/
struct DbbeTable {
  Dbbe *pBe;         /* The database of which this record is a part */
  BeFile *pFile;     /* The database file for this table */
  datum key;         /* Most recently used key */
  datum data;        /* Most recent data */
  int needRewind;    /* Next key should be the first */
  int readPending;   /* The fetch hasn't actually been done yet */
};

/*
** Initialize the RC4 algorithm.
*/
static void rc4init(struct rc4 *p, char *key, int keylen){
  int i;
  char k[256];
  p->j = 0;
  p->i = 0;
  for(i=0; i<256; i++){
    p->s[i] = i;
    k[i] = key[i%keylen];
  }
  for(i=0; i<256; i++){
    int t;
    p->j = (p->j + p->s[i] + k[i]) & 0xff;
    t = p->s[p->j];
    p->s[p->j] = p->s[i];
    p->s[i] = t;
  }
}

/*
** Get a single 8-bit random value from the RC4 algorithm.
*/
static int rc4byte(struct rc4 *p){
  int t;
  p->i = (p->i + 1) & 0xff;
  p->j = (p->j + p->s[p->i]) & 0xff;
  t = p->s[p->i];
  p->s[p->i] = p->s[p->j];
  p->s[p->j] = t;
  t = p->s[p->i] + p->s[p->j];
  return t & 0xff;
}

/*
** This routine opens a new database.  For the current driver scheme,
** the database name is the name of the directory
** containing all the files of the database.
*/
Dbbe *sqliteDbbeOpen(
  const char *zName,     /* The name of the database */
  int write,             /* True if we will be writing to the database */
  int create,            /* True to create database if it doesn't exist */
  char **pzErrMsg        /* Write error messages (if any) here */
){
  Dbbe *pNew;
  struct stat statbuf;

  if( stat(zName, &statbuf)!=0 ){
    sqliteSetString(pzErrMsg, "can't find file \"", zName, "\"", 0);
    return 0;
  }
  if( !S_ISDIR(statbuf.st_mode) ){
    sqliteSetString(pzErrMsg, "not a directory: \"", zName, "\"", 0);
    return 0;
  }
  pNew = sqliteMalloc(sizeof(Dbbe) + strlen(zName) + 1);
  if( pNew==0 ){
    sqliteSetString(pzErrMsg, "out of memory", 0);
    return 0;
  }
  pNew->zDir = (char*)&pNew[1];
  strcpy(pNew->zDir, zName);
  pNew->write = write;
  pNew->pOpen = 0;
  time(&statbuf.st_ctime);
  rc4init(&pNew->rc4, (char*)&statbuf, sizeof(statbuf));
  return pNew;
}

/*
** Completely shutdown the given database.  Close all files.  Free all memory.
*/
void sqliteDbbeClose(Dbbe *pBe){
  BeFile *pFile, *pNext;
  for(pFile=pBe->pOpen; pFile; pFile=pNext){
    pNext = pFile->pNext;
    gdbm_close(pFile->dbf);
    memset(pFile, 0, sizeof(*pFile));   
    sqliteFree(pFile);
  }
  memset(pBe, 0, sizeof(*pBe));
  sqliteFree(pBe);
}

/*
** Translate the name of a table into the name of a file that holds
** that table.  Space to hold the filename is obtained from
** sqliteMalloc() and must be freed by the calling function.
*/
static char *sqliteFileOfTable(Dbbe *pBe, const char *zTable){
  char *zFile = 0;
  int i;
  sqliteSetString(&zFile, pBe->zDir, "/", zTable, ".tbl", 0);
  if( zFile==0 ) return 0;
  for(i=strlen(pBe->zDir)+1; zFile[i]; i++){
    int c = zFile[i];
    if( isupper(c) ){
      zFile[i] = tolower(c);
    }else if( !isalnum(c) && c!='-' && c!='_' && c!='.' ){
      zFile[i] = '+';
    }
  }
  return zFile;
}

/*
** Generate a random filename with the given prefix.
*/
static void randomName(struct rc4 *pRc4, char *zBuf, char *zPrefix){
  int i, j;
  static const char zRandomChars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  strcpy(zBuf, zPrefix);
  j = strlen(zBuf);
  for(i=0; i<15; i++){
    int c = (rc4byte(pRc4) & 0x7f) % (sizeof(zRandomChars) - 1);
    zBuf[j++] = zRandomChars[c];
  }
  zBuf[j] = 0;
}


/*
** Open a new table cursor
*/
DbbeTable *sqliteDbbeOpenTable(
  Dbbe *pBe,              /* The database the table belongs to */
  const char *zTable,     /* The name of the table */
  int writeable           /* True to open for writing */
){
  char *zFile;            /* Name of the table file */
  DbbeTable *pTable;      /* The new table cursor */
  BeFile *pFile;          /* The underlying data file for this table */

  pTable = sqliteMalloc( sizeof(*pTable) );
  if( pTable==0 ) return 0;
  if( zTable ){
    zFile = sqliteFileOfTable(pBe, zTable);
    for(pFile=pBe->pOpen; pFile; pFile=pFile->pNext){
      if( strcmp(pFile->zName,zFile)==0 ) break;
    }
  }else{
    pFile = 0;
    zFile = 0;
  }
  if( pFile==0 ){
    pFile = sqliteMalloc( sizeof(*pFile) );
    if( pFile==0 ){
      sqliteFree(zFile);
      return 0;
    }
    pFile->zName = zFile;
    pFile->nRef = 1;
    pFile->pPrev = 0;
    if( pBe->pOpen ){
      pBe->pOpen->pPrev = pFile;
    }
    pFile->pNext = pBe->pOpen;
    pBe->pOpen = pFile;
    if( pFile->zName ){
      pFile->dbf = gdbm_open(pFile->zName, 0, GDBM_WRCREAT|GDBM_FAST, 0640, 0);
    }else{
      int i, j, limit;
      struct rc4 *pRc4;
      char zRandom[50];
      pRc4 = &pBe->rc4;
      zFile = 0;
      limit = 5;
      do {
        randomName(&pBe->rc4, zRandom, "_temp_table_");
        sqliteFree(zFile);
        zFile = sqliteFileOfTable(pBe, zRandom);
        pFile->dbf = gdbm_open(zFile, 0, GDBM_WRCREAT|GDBM_FAST, 0640, 0);
      }while( pFile->dbf==0 && limit-- >= 0);
      pFile->zName = zFile;
      pFile->delOnClose = 1;
    }
  }else{
    sqliteFree(zFile);
    pFile->nRef++;
  }
  pTable->pBe = pBe;
  pTable->pFile = pFile;
  pTable->readPending = 0;
  pTable->needRewind = 1;
  return pTable;
}

/*
** Drop a table from the database.
*/
void sqliteDbbeDropTable(Dbbe *pBe, const char *zTable){
  char *zFile;            /* Name of the table file */

  zFile = sqliteFileOfTable(pBe, zTable);
  unlink(zFile);
  sqliteFree(zFile);
}

/*
** Reorganize a table to reduce search times and disk usage.
*/
void sqliteDbbeReorganizeTable(Dbbe *pBe, const char *zTable){
  char *zFile;            /* Name of the table file */
  DbbeTable *pTab;

  pTab = sqliteDbbeOpenTable(pBe, zTable, 1);
  if( pTab && pTab->pFile && pTab->pFile->dbf ){
    gdbm_reorganize(pTab->pFile->dbf);
  }
  if( pTab ){
    sqliteDbbeCloseTable(pTab);
  }
}

/*
** Close a table previously opened by sqliteDbbeOpenTable().
*/
void sqliteDbbeCloseTable(DbbeTable *pTable){
  BeFile *pFile;
  Dbbe *pBe;
  if( pTable==0 ) return;
  pFile = pTable->pFile;
  pBe = pTable->pBe;
  pFile->nRef--;
  if( pFile->dbf!=NULL ){
    gdbm_sync(pFile->dbf);
  }
  if( pFile->nRef<=0 ){
    if( pFile->dbf!=NULL ){
      gdbm_close(pFile->dbf);
    }
    if( pFile->pPrev ){
      pFile->pPrev->pNext = pFile->pNext;
    }else{
      pBe->pOpen = pFile->pNext;
    }
    if( pFile->pNext ){
      pFile->pNext->pPrev = pFile->pPrev;
    }
    if( pFile->delOnClose ){
      unlink(pFile->zName);
    }
    sqliteFree(pFile->zName);
    memset(pFile, 0, sizeof(*pFile));
    sqliteFree(pFile);
  }
  if( pTable->key.dptr ) free(pTable->key.dptr);
  if( pTable->data.dptr ) free(pTable->data.dptr);
  memset(pTable, 0, sizeof(*pTable));
  sqliteFree(pTable);
}

/*
** Clear the given datum
*/
static void datumClear(datum *p){
  if( p->dptr ) free(p->dptr);
  p->dptr = 0;
  p->dsize = 0;
}

/*
** Fetch a single record from an open table.  Return 1 on success
** and 0 on failure.
*/
int sqliteDbbeFetch(DbbeTable *pTable, int nKey, char *pKey){
  datum key;
  key.dsize = nKey;
  key.dptr = pKey;
  datumClear(&pTable->key);
  datumClear(&pTable->data);
  if( pTable->pFile && pTable->pFile->dbf ){
    pTable->data = gdbm_fetch(pTable->pFile->dbf, key);
  }
  return pTable->data.dptr!=0;
}

/*
** Return 1 if the given key is already in the table.  Return 0
** if it is not.
*/
int sqliteDbbeTest(DbbeTable *pTable, int nKey, char *pKey){
  datum key;
  int result = 0;
  key.dsize = nKey;
  key.dptr = pKey;
  if( pTable->pFile && pTable->pFile->dbf ){
    result = gdbm_exists(pTable->pFile->dbf, key);
  }
  return result;
}

/*
** Copy bytes from the current key or data into a buffer supplied by
** the calling function.  Return the number of bytes copied.
*/
int sqliteDbbeCopyKey(DbbeTable *pTable, int offset, int size, char *zBuf){
  int n;
  if( offset>=pTable->key.dsize ) return 0;
  if( offset+size>pTable->key.dsize ){
    n = pTable->key.dsize - offset;
  }else{
    n = size;
  }
  memcpy(zBuf, &pTable->key.dptr[offset], n);
  return n;
}
int sqliteDbbeCopyData(DbbeTable *pTable, int offset, int size, char *zBuf){
  int n;
  if( pTable->readPending && pTable->pFile && pTable->pFile->dbf ){
    pTable->data = gdbm_fetch(pTable->pFile->dbf, pTable->key);
    pTable->readPending = 0;
  }
  if( offset>=pTable->data.dsize ) return 0;
  if( offset+size>pTable->data.dsize ){
    n = pTable->data.dsize - offset;
  }else{
    n = size;
  }
  memcpy(zBuf, &pTable->data.dptr[offset], n);
  return n;
}

/*
** Return a pointer to bytes from the key or data.  The data returned
** is ephemeral.
*/
char *sqliteDbbeReadKey(DbbeTable *pTable, int offset){
  if( offset<0 || offset>=pTable->key.dsize ) return "";
  return &pTable->key.dptr[offset];
}
char *sqliteDbbeReadData(DbbeTable *pTable, int offset){
  if( pTable->readPending && pTable->pFile && pTable->pFile->dbf ){
    pTable->data = gdbm_fetch(pTable->pFile->dbf, pTable->key);
    pTable->readPending = 0;
  }
  if( offset<0 || offset>=pTable->data.dsize ) return "";
  return &pTable->data.dptr[offset];
}

/*
** Return the total number of bytes in either data or key.
*/
int sqliteDbbeKeyLength(DbbeTable *pTable){
  return pTable->key.dsize;
}
int sqliteDbbeDataLength(DbbeTable *pTable){
  if( pTable->readPending && pTable->pFile && pTable->pFile->dbf ){
    pTable->data = gdbm_fetch(pTable->pFile->dbf, pTable->key);
    pTable->readPending = 0;
  }
  return pTable->data.dsize;
}

/*
** Make is so that the next call to sqliteNextKey() finds the first
** key of the table.
*/
int sqliteDbbeRewind(DbbeTable *pTable){
  pTable->needRewind = 1;
  return 0;
}

/*
** Read the next key from the table.  Return 1 on success.  Return
** 0 if there are no more keys.
*/
int sqliteDbbeNextKey(DbbeTable *pTable){
  datum nextkey;
  int rc;
  if( pTable==0 || pTable->pFile==0 || pTable->pFile->dbf==0 ){
    pTable->readPending = 0;
    return 0;
  }
  if( pTable->needRewind ){
    nextkey = gdbm_firstkey(pTable->pFile->dbf);
    pTable->needRewind = 0;
  }else{
    nextkey = gdbm_nextkey(pTable->pFile->dbf, pTable->key);
  }
  datumClear(&pTable->key);
  datumClear(&pTable->data);
  pTable->key = nextkey;
  if( pTable->key.dptr ){
    pTable->readPending = 1;
    rc = 1;
  }else{
    pTable->needRewind = 1;
    pTable->readPending = 0;
    rc = 0;
  }
  return rc;
}

/*
** Get a new integer key.
*/
int sqliteDbbeNew(DbbeTable *pTable){
  int iKey;
  datum key;
  int go = 1;
  int i;
  struct rc4 *pRc4;

  if( pTable->pFile==0 || pTable->pFile->dbf==0 ) return 1;
  pRc4 = &pTable->pBe->rc4;
  while( go ){
    iKey = 0;
    for(i=0; i<4; i++){
      iKey = (iKey<<8) + rc4byte(pRc4);
    }
    key.dptr = (char*)&iKey;
    key.dsize = 4;
    go = gdbm_exists(pTable->pFile->dbf, key);
  }
  return iKey;
}   

/*
** Write an entry into the table.  Overwrite any prior entry with the
** same key.
*/
int sqliteDbbePut(DbbeTable *pTable, int nKey,char *pKey,int nData,char *pData){
  datum data, key;
  if( pTable->pFile==0 || pTable->pFile->dbf==0 ) return 0;
  data.dsize = nData;
  data.dptr = pData;
  key.dsize = nKey;
  key.dptr = pKey;
  gdbm_store(pTable->pFile->dbf, key, data, GDBM_REPLACE);
  datumClear(&pTable->key);
  datumClear(&pTable->data);
  return 1;
}

/*
** Remove an entry from a table, if the entry exists.
*/
int sqliteDbbeDelete(DbbeTable *pTable, int nKey, char *pKey){
  datum key;
  datumClear(&pTable->key);
  datumClear(&pTable->data);
  if( pTable->pFile==0 || pTable->pFile->dbf==0 ) return 0;
  key.dsize = nKey;
  key.dptr = pKey;
  gdbm_delete(pTable->pFile->dbf, key);
  return 1;
}

/*
** Open a temporary file.
*/
FILE *sqliteDbbeOpenTempFile(Dbbe *pBe){
  char *zFile;
  char zBuf[50];
  int i, j;
  int limit;

  for(i=0; i<pBe->nTemp; i++){
    if( pBe->apTemp[i]==0 ) break;
  }
  if( i>=pBe->nTemp ){
    pBe->nTemp++;
    pBe->apTemp = sqliteRealloc(pBe->apTemp, pBe->nTemp*sizeof(FILE*) );
  }
  if( pBe->apTemp==0 ) return 0;
  limit = 4;
  zFile = 0;
  do{
    randomName(&pBe->rc4, zBuf, "/_temp_file_");
    sqliteFree(zFile);
    sqliteSetString(&zFile, pBe->zDir, zBuf, 0);
  }while( access(zFile,0) && limit-- >= 0 );
  pBe->apTemp[i] = fopen(zFile, "w+");
  sqliteFree(zFile);
  return pBe->apTemp[i];
}

/*
** Close a temporary file opened using sqliteDbbeOpenTempFile()
*/
void sqliteDbbeCloseTempFile(Dbbe *pBe, FILE *f){
  int i;
  for(i=0; i<pBe->nTemp; i++){
    if( pBe->apTemp[i]==f ){
      char *zFile;
      char zBuf[30];
      sprintf(zBuf, "/_temp_%d~", i);
      zFile = 0;
      sqliteSetString(&zFile, pBe->zDir, zBuf, 0);
      unlink(zFile);
      sqliteFree(zFile);
      pBe->apTemp[i] = 0;
      break;
    }
  }
  fclose(f);
}
