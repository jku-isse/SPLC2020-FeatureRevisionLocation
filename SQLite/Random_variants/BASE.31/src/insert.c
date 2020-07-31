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
** to handle INSERT statements.
**
** $Id: insert.c,v 1.1 2000/05/31 15:34:53 drh Exp $
*/
#include "sqliteInt.h"

/*
** This routine is call to handle SQL of the following form:
**
**    insert into TABLE (IDLIST) values(EXPRLIST)
**
** The parameters are the table name and the expression list.
*/
void sqliteInsert(
  Parse *pParse,        /* Parser context */
  Token *pTableName,    /* Name of table into which we are inserting */
  ExprList *pList,      /* List of values to be inserted */
  IdList *pField        /* Field name corresponding to pList.  Might be NULL */
){
  Table *pTab;
  char *zTab;
  int i, j;
  Vdbe *v;

  zTab = sqliteTableNameFromToken(pTableName);
  pTab = sqliteFindTable(pParse->db, zTab);
  sqliteFree(zTab);
  if( pTab==0 ){
    sqliteSetNString(&pParse->zErrMsg, "no such table: ", 0, 
        pTableName->z, pTableName->n, 0);
    pParse->nErr++;
    goto insert_cleanup;
  }
  if( pTab->readOnly ){
    sqliteSetString(&pParse->zErrMsg, "table ", pTab->zName,
        " may not be modified", 0);
    pParse->nErr++;
    goto insert_cleanup;
  }
  if( pField==0 && pList->nExpr!=pTab->nCol ){
    char zNum1[30];
    char zNum2[30];
    sprintf(zNum1,"%d", pList->nExpr);
    sprintf(zNum2,"%d", pTab->nCol);
    sqliteSetString(&pParse->zErrMsg, "table ", pTab->zName,
       " has ", zNum2, " columns but ",
       zNum1, " values were supplied", 0);
    pParse->nErr++;
    goto insert_cleanup;
  }
  if( pField!=0 && pList->nExpr!=pField->nId ){
    char zNum1[30];
    char zNum2[30];
    sprintf(zNum1,"%d", pList->nExpr);
    sprintf(zNum2,"%d", pField->nId);
    sqliteSetString(&pParse->zErrMsg, zNum1, " values for ",
       zNum2, " columns", 0);
    pParse->nErr++;
    goto insert_cleanup;
  }
  if( pField ){
    for(i=0; i<pField->nId; i++){
      pField->a[i].idx = -1;
    }
    for(i=0; i<pField->nId; i++){
      for(j=0; j<pTab->nCol; j++){
        if( sqliteStrICmp(pField->a[i].zName, pTab->azCol[j])==0 ){
          pField->a[i].idx = j;
          break;
        }
      }
      if( j>=pTab->nCol ){
        sqliteSetString(&pParse->zErrMsg, "table ", pTab->zName,
           " has no column named ", pField->a[i].zName, 0);
        pParse->nErr++;
        goto insert_cleanup;
      }
    }
  }
  v = pParse->pVdbe = sqliteVdbeCreate(pParse->db->pBe);
  if( v ){
    Index *pIdx;
    sqliteVdbeAddOp(v, 1, 0, 0, pTab->zName, 0);
    sqliteVdbeAddOp(v, 4, 0, 0, 0, 0);
    if( pTab->pIndex ){
      sqliteVdbeAddOp(v, 47, 0, 0, 0, 0);
    }
    for(i=0; i<pTab->nCol; i++){
      if( pField==0 ){
        j = i;
      }else{
        for(j=0; j<pField->nId; j++){
          if( pField->a[j].idx==i ) break;
        }
      }
      if( pField && j>=pField->nId ){
        sqliteVdbeAddOp(v, 45, 0, 0, "", 0);
      }else{
        sqliteExprCode(pParse, pList->a[j].pExpr);
      }
    }
    sqliteVdbeAddOp(v, 36, pTab->nCol, 0, 0, 0);
    sqliteVdbeAddOp(v, 5, 0, 0, 0, 0);
    sqliteVdbeAddOp(v, 2, 0, 0, 0, 0);
    for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
      if( pIdx->pNext ){
        sqliteVdbeAddOp(v, 47, 0, 0, 0, 0);
      }
      sqliteVdbeAddOp(v, 1, 0, 0, pIdx->zName, 0);
      for(i=0; i<pIdx->nField; i++){
        int idx = pIdx->aiField[i];
        if( pField==0 ){
          j = idx;
        }else{
          for(j=0; j<pField->nId; j++){
            if( pField->a[j].idx==idx ) break;
          }
        }
        if( pField && j>=pField->nId ){
          sqliteVdbeAddOp(v, 45, 0, 0, "", 0);
        }else{
          sqliteExprCode(pParse, pList->a[j].pExpr);
        }
      }
      sqliteVdbeAddOp(v, 37, pIdx->nField, 0, 0, 0);
      sqliteVdbeAddOp(v, 16, 0, 0, 0, 0);
      sqliteVdbeAddOp(v, 2, 0, 0, 0, 0);
    }
  }

insert_cleanup:
  sqliteExprListDelete(pList);
  sqliteIdListDelete(pField);
}
