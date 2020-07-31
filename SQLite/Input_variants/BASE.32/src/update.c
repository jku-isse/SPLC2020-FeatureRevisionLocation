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
** This file contains C code routines that are called by the parser
** to handle UPDATE statements.
**
** $Id: update.c,v 1.1 2000/05/31 15:34:53 drh Exp $
*/
#include "sqliteInt.h"

/*
** Process an UPDATE statement.
*/
void sqliteUpdate(
  Parse *pParse,         /* The parser context */
  Token *pTableName,     /* The table in which we should change things */
  ExprList *pChanges,    /* Things to be changed */
  Expr *pWhere           /* The WHERE clause.  May be null */
){
  int i, j;              /* Loop counters */
  Table *pTab;           /* The table to be updated */
  IdList *pTabList = 0;  /* List containing only pTab */
  int end, addr;         /* A couple of addresses in the generated code */
  WhereInfo *pWInfo;     /* Information about the WHERE clause */
  Vdbe *v;               /* The virtual database engine */
  Index *pIdx;           /* For looping over indices */
  int nIdx;              /* Number of indices that need updating */
  Index **apIdx = 0;     /* An array of indices that need updating too */
  int *aXRef = 0;        /* aXRef[i] is the index in pChanges->a[] of the
                         ** an expression for the i-th field of the table.
                         ** aXRef[i]==-1 if the i-th field is not changed. */

  /* Locate the table which we want to update.  This table has to be
  ** put in an IdList structure because some of the subroutines will
  ** will be calling are designed to work with multiple tables and expect
  ** an IdList* parameter instead of just a Table* parameger.
  */
  pTabList = sqliteIdListAppend(0, pTableName);
  for(i=0; i<pTabList->nId; i++){
    pTabList->a[i].pTab = sqliteFindTable(pParse->db, pTabList->a[i].zName);
    if( pTabList->a[i].pTab==0 ){
      sqliteSetString(&pParse->zErrMsg, "no such table: ", 
         pTabList->a[i].zName, 0);
      pParse->nErr++;
      goto update_cleanup;
    }
    if( pTabList->a[i].pTab->readOnly ){
      sqliteSetString(&pParse->zErrMsg, "table ", pTabList->a[i].zName,
        " may not be modified", 0);
      pParse->nErr++;
      goto update_cleanup;
    }
  }
  pTab = pTabList->a[0].pTab;
  aXRef = sqliteMalloc( sizeof(int) * pTab->nCol );
  if( aXRef==0 ) goto update_cleanup;
  for(i=0; i<pTab->nCol; i++) aXRef[i] = -1;

  /* Resolve the field names in all the expressions in both the
  ** WHERE clause and in the new values.  Also find the field index
  ** for each field to be updated in the pChanges array.
  */
  if( pWhere ){
    if( sqliteExprResolveIds(pParse, pTabList, pWhere) ){
      goto update_cleanup;
    }
    if( sqliteExprCheck(pParse, pWhere, 0, 0) ){
      goto update_cleanup;
    }
  }
  for(i=0; i<pChanges->nExpr; i++){
    if( sqliteExprResolveIds(pParse, pTabList, pChanges->a[i].pExpr) ){
      goto update_cleanup;
    }
    if( sqliteExprCheck(pParse, pChanges->a[i].pExpr, 0, 0) ){
      goto update_cleanup;
    }
    for(j=0; j<pTab->nCol; j++){
      if( strcmp(pTab->azCol[j], pChanges->a[i].zName)==0 ){
        pChanges->a[i].idx = j;
        aXRef[j] = i;
        break;
      }
    }
    if( j>=pTab->nCol ){
      sqliteSetString(&pParse->zErrMsg, "no such field: ", 
         pChanges->a[i].zName, 0);
      pParse->nErr++;
      goto update_cleanup;
    }
  }

  /* Allocate memory for the array apIdx[] and fill it pointers to every
  ** index that needs to be updated.  Indices only need updating if their
  ** key includes one of the fields named in pChanges.
  */
  for(nIdx=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    for(i=0; i<pIdx->nField; i++){
      if( aXRef[pIdx->aiField[i]]>=0 ) break;
    }
    if( i<pIdx->nField ) nIdx++;
  }
  apIdx = sqliteMalloc( sizeof(Index*) * nIdx );
  if( apIdx==0 ) goto update_cleanup;
  for(nIdx=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    for(i=0; i<pIdx->nField; i++){
      if( aXRef[pIdx->aiField[i]]>=0 ) break;
    }
    if( i<pIdx->nField ) apIdx[nIdx++] = pIdx;
  }

  /* Begin generating code.
  */
  v = pParse->pVdbe;
  if( v==0 ){
    v = pParse->pVdbe = sqliteVdbeCreate(pParse->db->pBe);
  }
  if( v==0 ) goto update_cleanup;

  /* Begin the database scan
  */
  sqliteVdbeAddOp(v, 18, 0, 0, 0, 0);
  pWInfo = sqliteWhereBegin(pParse, pTabList, pWhere, 1);
  if( pWInfo==0 ) goto update_cleanup;

  /* Remember the index of every item to be updated.
  */
  sqliteVdbeAddOp(v, 19, 0, 0, 0, 0);

  /* End the database scan loop.
  */
  sqliteWhereEnd(pWInfo);

  /* Rewind the list of records that need to be updated and
  ** open every index that needs updating.
  */
  sqliteVdbeAddOp(v, 20, 0, 0, 0, 0);
  for(i=0; i<nIdx; i++){
    sqliteVdbeAddOp(v, 1, i+1, 0, apIdx[i]->zName, 0);
  }

  /* Loop over every record that needs updating.  We have to load
  ** the old data for each record to be updated because some fields
  ** might not change and we will need to copy the old value, therefore.
  ** Also, the old data is needed to delete the old index entires.
  */
  end = sqliteVdbeMakeLabel(v);
  addr = sqliteVdbeAddOp(v, 21, 0, end, 0, 0);
  sqliteVdbeAddOp(v, 47, 0, 0, 0, 0);
  sqliteVdbeAddOp(v, 3, 0, 0, 0, 0);

  /* Delete the old indices for the current record.
  */
  for(i=0; i<nIdx; i++){
    sqliteVdbeAddOp(v, 47, 0, 0, 0, 0);
    pIdx = apIdx[i];
    for(j=0; j<pIdx->nField; j++){
      sqliteVdbeAddOp(v, 8, 0, pIdx->aiField[j], 0, 0);
    }
    sqliteVdbeAddOp(v, 37, pIdx->nField, 0, 0, 0);
    sqliteVdbeAddOp(v, 17, i+1, 0, 0, 0);
  }

  /* Compute a completely new data for this record.  
  */
  for(i=0; i<pTab->nCol; i++){
    j = aXRef[i];
    if( j<0 ){
      sqliteVdbeAddOp(v, 8, 0, i, 0, 0);
    }else{
      sqliteExprCode(pParse, pChanges->a[j].pExpr);
    }
  }

  /* Insert new index entries that correspond to the new data
  */
  for(i=0; i<nIdx; i++){
    sqliteVdbeAddOp(v, 47, pTab->nCol, 0, 0, 0); /* The KEY */
    pIdx = apIdx[i];
    for(j=0; j<pIdx->nField; j++){
      sqliteVdbeAddOp(v, 47, j+pTab->nCol-pIdx->aiField[j], 0, 0, 0);
    }
    sqliteVdbeAddOp(v, 37, pIdx->nField, 0, 0, 0);
    sqliteVdbeAddOp(v, 16, i+1, 0, 0, 0);
  }

  /* Write the new data back into the database.
  */
  sqliteVdbeAddOp(v, 36, pTab->nCol, 0, 0, 0);
  sqliteVdbeAddOp(v, 5, 0, 0, 0, 0);

  /* Repeat the above with the next record to be updated, until
  ** all record selected by the WHERE clause have been updated.
  */
  sqliteVdbeAddOp(v, 38, 0, addr, 0, 0);
  sqliteVdbeAddOp(v, 22, 0, 0, 0, end);

update_cleanup:
  sqliteFree(apIdx);
  sqliteFree(aXRef);
  sqliteIdListDelete(pTabList);
  sqliteExprListDelete(pChanges);
  sqliteExprDelete(pWhere);
  return;
}
