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
** This file contains C code routines used for processing expressions
**
** $Id: expr.c,v 1.4 2000/06/03 19:19:41 drh Exp $
*/
#include "sqliteInt.h"

/*
** This routine walks an expression tree and resolves references to
** table fields.  Nodes of the form ID.ID or ID resolve into an
** index to the table in the table list and a field offset.  The opcode
** for such nodes is changed to TK_FIELD.  The iTable value is changed
** to the index of the referenced table in pTabList, and the iField value
** is changed to the index of the field of the referenced table.
**
** Unknown fields or tables provoke an error.  The function returns
** the number of errors seen and leaves an error message on pParse->zErrMsg.
*/
int sqliteExprResolveIds(Parse *pParse, IdList *pTabList, Expr *pExpr){
  if( pExpr==0 ) return 0;
  switch( pExpr->op ){
    /* A lone identifier */
    case TK_ID: {
      int cnt = 0;   /* Number of matches */
      int i;         /* Loop counter */
      char *z = 0;
      sqliteSetNString(&z, pExpr->token.z, pExpr->token.n, 0);
      for(i=0; i<pTabList->nId; i++){
        int j;
        Table *pTab = pTabList->a[i].pTab;
        if( pTab==0 ) continue;
        for(j=0; j<pTab->nCol; j++){
          if( sqliteStrICmp(pTab->aCol[j].zName, z)==0 ){
            cnt++;
            pExpr->iTable = i;
            pExpr->iField = j;
          }
        }
      }
      sqliteFree(z);
      if( cnt==0 ){
        sqliteSetNString(&pParse->zErrMsg, "no such field: ", -1,  
          pExpr->token.z, pExpr->token.n, 0);
        pParse->nErr++;
        return 1;
      }else if( cnt>1 ){
        sqliteSetNString(&pParse->zErrMsg, "ambiguous field name: ", -1,  
          pExpr->token.z, pExpr->token.n, 0);
        pParse->nErr++;
        return 1;
      }
      pExpr->op = TK_FIELD;
      break; 
    }
  
    /* A table name and field name:  ID.ID */
    case TK_DOT: {
      int cnt = 0;             /* Number of matches */
      int i;                   /* Loop counter */
      Expr *pLeft, *pRight;    /* Left and right subbranches of the expr */
      char *zLeft, *zRight;    /* Text of an identifier */

      pLeft = pExpr->pLeft;
      pRight = pExpr->pRight;
      assert( pLeft && pLeft->op==TK_ID );
      assert( pRight && pRight->op==TK_ID );
      zLeft = 0;
      sqliteSetNString(&zLeft, pLeft->token.z, pLeft->token.n, 0);
      zRight = 0;
      sqliteSetNString(&zRight, pRight->token.z, pRight->token.n, 0);
      for(i=0; i<pTabList->nId; i++){
        int j;
        char *zTab;
        Table *pTab = pTabList->a[i].pTab;
        if( pTab==0 ) continue;
        if( pTabList->a[i].zAlias ){
          zTab = pTabList->a[i].zAlias;
        }else{
          zTab = pTab->zName;
        }
        if( sqliteStrICmp(zTab, zLeft)!=0 ) continue;
        for(j=0; j<pTab->nCol; j++){
          if( sqliteStrICmp(pTab->aCol[j].zName, zRight)==0 ){
            cnt++;
            pExpr->iTable = i;
            pExpr->iField = j;
          }
        }
      }
      sqliteFree(zLeft);
      sqliteFree(zRight);
      if( cnt==0 ){
        sqliteSetNString(&pParse->zErrMsg, "no such field: ", -1,  
          pLeft->token.z, pLeft->token.n, ".", 1, 
          pRight->token.z, pRight->token.n, 0);
        pParse->nErr++;
        return 1;
      }else if( cnt>1 ){
        sqliteSetNString(&pParse->zErrMsg, "ambiguous field name: ", -1,  
          pLeft->token.z, pLeft->token.n, ".", 1,
          pRight->token.z, pRight->token.n, 0);
        pParse->nErr++;
        return 1;
      }
      sqliteExprDelete(pLeft);
      pExpr->pLeft = 0;
      sqliteExprDelete(pRight);
      pExpr->pRight = 0;
      pExpr->op = TK_FIELD;
      break;
    }

    /* For all else, just recursively walk the tree */
    default: {
      if( pExpr->pLeft 
            && sqliteExprResolveIds(pParse, pTabList, pExpr->pLeft) ){
        return 1;
      }
      if( pExpr->pRight 
            && sqliteExprResolveIds(pParse, pTabList, pExpr->pRight) ){
        return 1;
      }
      if( pExpr->pList ){
        int i;
        ExprList *pList = pExpr->pList;
        for(i=0; i<pList->nExpr; i++){
          if( sqliteExprResolveIds(pParse, pTabList, pList->a[i].pExpr) ){
            return 1;
          }
        }
      }
    }
  }
  return 0;
}






  
  
  



/*
** Convert a function name into its integer identifier.  Return the
** identifier.  Return FN_Unknown if the function name is unknown.
*/
int sqliteFuncId(Token *pToken){
  static const struct {
     char *zName;
     int len;
     int id;
  } aFunc[] = {
     { "count",  5, 1 },
     { "min",    3, 2   },
     { "max",    3, 3   },
     { "sum",    3, 4   },
  };
  int i;
  for(i=0; i<(sizeof(aFunc)/sizeof(aFunc[0])); i++){
    if( aFunc[i].len==pToken->n 
     && sqliteStrNICmp(pToken->z, aFunc[i].zName, aFunc[i].len)==0 ){
       return aFunc[i].id;
    }
  }
  return 0;
}

/*
** Error check the functions in an expression.  Make sure all
** function names are recognized and all functions have the correct
** number of arguments.  Leave an error message in pParse->zErrMsg
** if anything is amiss.  Return the number of errors.
**
** if pIsAgg is not null and this expression is an aggregate function
** (like count(*) or max(value)) then write a 1 into *pIsAgg.
*/
int sqliteExprCheck(Parse *pParse, Expr *pExpr, int allowAgg, int *pIsAgg){
  int nErr = 0;
  if( pExpr==0 ) return 0;
  if( pIsAgg ) *pIsAgg = 0;
  switch( pExpr->op ){
    case TK_FUNCTION: {
      int id = sqliteFuncId(&pExpr->token);
      int n = pExpr->pList ? pExpr->pList->nExpr : 0;
      int no_such_func = 0;
      int too_many_args = 0;
      int too_few_args = 0;
      int is_agg = 0;
      int i;
      switch( id ){
        case 0: { 
          no_such_func = 1;
          break;
        }
        case 1: { 
          no_such_func = !allowAgg;
          too_many_args = n>1;
          is_agg = 1;
          break;
        }
        case 3:
        case 2: {
          too_few_args = allowAgg ? n<1 : n<2;
          is_agg = n==1;
          break;
        }
        case 4: {
          no_such_func = !allowAgg;
          too_many_args = n>1;
          too_few_args = n<1;
          is_agg = 1;
          break;
        }
        default: break;
      }
      if( no_such_func ){
        sqliteSetNString(&pParse->zErrMsg, "no such function: ", -1,
           pExpr->token.z, pExpr->token.n, 0);
        pParse->nErr++;
        nErr++;
      }else if( too_many_args ){
        sqliteSetNString(&pParse->zErrMsg, "too many arguments to function ",-1,
           pExpr->token.z, pExpr->token.n, "()", 2, 0);
        pParse->nErr++;
        nErr++;
      }else if( too_few_args ){
        sqliteSetNString(&pParse->zErrMsg, "too few arguments to function ",-1,
           pExpr->token.z, pExpr->token.n, "()", 2, 0);
        pParse->nErr++;
        nErr++;
      }
      if( is_agg && pIsAgg ) *pIsAgg = 1;
      for(i=0; nErr==0 && i<n; i++){
        nErr = sqliteExprCheck(pParse, pExpr->pList->a[i].pExpr, 0, 0);
      }
    }
    default: {
      if( pExpr->pLeft ){
        nErr = sqliteExprCheck(pParse, pExpr->pLeft, 0, 0);
      }
      if( nErr==0 && pExpr->pRight ){
        nErr = sqliteExprCheck(pParse, pExpr->pRight, 0, 0);
      }
      break;
    }
  }
  return nErr;
}

/*
** Generate code into the current Vdbe to evaluate the given
** expression and leave the result on the stack.
*/
void sqliteExprCode(Parse *pParse, Expr *pExpr){
  Vdbe *v = pParse->pVdbe;
  int op;
  switch( pExpr->op ){
    case TK_PLUS:     op = 49;      break;
    case TK_MINUS:    op = 51; break;
    case TK_STAR:     op = 52; break;
    case TK_SLASH:    op = 53;   break;
    case TK_AND:      op = 67;      break;
    case TK_OR:       op = 68;       break;
    case TK_LT:       op = 60;       break;
    case TK_LE:       op = 61;       break;
    case TK_GT:       op = 62;       break;
    case TK_GE:       op = 63;       break;
    case TK_NE:       op = 59;       break;
    case TK_EQ:       op = 58;       break;
    case TK_LIKE:     op = 56;     break;
    case TK_GLOB:     op = 57;     break;
    case TK_ISNULL:   op = 64;   break;
    case TK_NOTNULL:  op = 65;  break;
    case TK_NOT:      op = 69;      break;
    case TK_UMINUS:   op = 66; break;
    default: break;
  }
  switch( pExpr->op ){
    case TK_FIELD: {
      sqliteVdbeAddOp(v, 8, pExpr->iTable, pExpr->iField, 0, 0);
      break;
    }
    case TK_INTEGER: {
      int i = atoi(pExpr->token.z);
      sqliteVdbeAddOp(v, 44, i, 0, 0, 0);
      break;
    }
    case TK_FLOAT: {
      int addr = sqliteVdbeAddOp(v, 45, 0, 0, 0, 0);
      sqliteVdbeChangeP3(v, addr, pExpr->token.z, pExpr->token.n);
      break;
    }
    case TK_STRING: {
      int addr = sqliteVdbeAddOp(v, 45, 0, 0, 0, 0);
      sqliteVdbeChangeP3(v, addr, pExpr->token.z, pExpr->token.n);
      sqliteVdbeDequoteP3(v, addr);
      break;
    }
    case TK_NULL: {
      sqliteVdbeAddOp(v, 45, 0, 0, "", 0);
      break;
    }
    case TK_AND:
    case TK_OR:
    case TK_PLUS:
    case TK_STAR:
    case TK_MINUS:
    case TK_SLASH: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteExprCode(pParse, pExpr->pRight);
      sqliteVdbeAddOp(v, op, 0, 0, 0, 0);
      break;
    }
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
    case TK_NE:
    case TK_EQ: 
    case TK_LIKE: 
    case TK_GLOB: {
      int dest;
      sqliteVdbeAddOp(v, 44, 1, 0, 0, 0);
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteExprCode(pParse, pExpr->pRight);
      dest = sqliteVdbeCurrentAddr(v) + 2;
      sqliteVdbeAddOp(v, op, 0, dest, 0, 0);
      sqliteVdbeAddOp(v, 50, -1, 0, 0, 0);
      break;
    }
    case TK_NOT:
    case TK_UMINUS: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteVdbeAddOp(v, op, 0, 0, 0, 0);
      break;
    }
    case TK_ISNULL:
    case TK_NOTNULL: {
      int dest;
      sqliteVdbeAddOp(v, 44, 1, 0, 0, 0);
      sqliteExprCode(pParse, pExpr->pLeft);
      dest = sqliteVdbeCurrentAddr(v) + 2;
      sqliteVdbeAddOp(v, op, 0, dest, 0, 0);
      sqliteVdbeAddOp(v, 50, -1, 0, 0, 0);
      break;
    }
    case TK_FUNCTION: {
      int id = sqliteFuncId(&pExpr->token);
      int op;
      int i;
      ExprList *pList = pExpr->pList;
      op = id==2 ? 54 : 55;
      for(i=0; i<pList->nExpr; i++){
        sqliteExprCode(pParse, pList->a[i].pExpr);
        if( i>0 ){
          sqliteVdbeAddOp(v, op, 0, 0, 0, 0);
        }
      }
      break;
    }
  }
  return;
}

/*
** Generate code for a boolean expression such that a jump is made
** to the label "dest" if the expression is true but execution
** continues straight thru if the expression is false.
*/
void sqliteExprIfTrue(Parse *pParse, Expr *pExpr, int dest){
  Vdbe *v = pParse->pVdbe;
  int op = 0;
  switch( pExpr->op ){
    case TK_LT:       op = 60;       break;
    case TK_LE:       op = 61;       break;
    case TK_GT:       op = 62;       break;
    case TK_GE:       op = 63;       break;
    case TK_NE:       op = 59;       break;
    case TK_EQ:       op = 58;       break;
    case TK_LIKE:     op = 56;     break;
    case TK_GLOB:     op = 57;     break;
    case TK_ISNULL:   op = 64;   break;
    case TK_NOTNULL:  op = 65;  break;
    default:  break;
  }
  switch( pExpr->op ){
    case TK_AND: {
      int d2 = sqliteVdbeMakeLabel(v);
      sqliteExprIfFalse(pParse, pExpr->pLeft, d2);
      sqliteExprIfTrue(pParse, pExpr->pRight, dest);
      sqliteVdbeResolveLabel(v, d2);
      break;
    }
    case TK_OR: {
      sqliteExprIfTrue(pParse, pExpr->pLeft, dest);
      sqliteExprIfTrue(pParse, pExpr->pRight, dest);
      break;
    }
    case TK_NOT: {
      sqliteExprIfFalse(pParse, pExpr->pLeft, dest);
      break;
    }
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
    case TK_NE:
    case TK_EQ:
    case TK_LIKE:
    case TK_GLOB: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteExprCode(pParse, pExpr->pRight);
      sqliteVdbeAddOp(v, op, 0, dest, 0, 0);
      break;
    }
    case TK_ISNULL:
    case TK_NOTNULL: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteVdbeAddOp(v, op, 0, dest, 0, 0);
      break;
    }
    default: {
      sqliteExprCode(pParse, pExpr);
      sqliteVdbeAddOp(v, 39, 0, dest, 0, 0);
      break;
    }
  }
}

/*
** Generate code for boolean expression such that a jump is made
** to the label "dest" if the expression is false but execution
** continues straight thru if the expression is true.
*/
void sqliteExprIfFalse(Parse *pParse, Expr *pExpr, int dest){
  Vdbe *v = pParse->pVdbe;
  int op = 0;
  switch( pExpr->op ){
    case TK_LT:       op = 63;       break;
    case TK_LE:       op = 62;       break;
    case TK_GT:       op = 61;       break;
    case TK_GE:       op = 60;       break;
    case TK_NE:       op = 58;       break;
    case TK_EQ:       op = 59;       break;
    case TK_LIKE:     op = 56;     break;
    case TK_GLOB:     op = 57;     break;
    case TK_ISNULL:   op = 65;  break;
    case TK_NOTNULL:  op = 64;   break;
    default:  break;
  }
  switch( pExpr->op ){
    case TK_AND: {
      sqliteExprIfFalse(pParse, pExpr->pLeft, dest);
      sqliteExprIfFalse(pParse, pExpr->pRight, dest);
      break;
    }
    case TK_OR: {
      int d2 = sqliteVdbeMakeLabel(v);
      sqliteExprIfTrue(pParse, pExpr->pLeft, d2);
      sqliteExprIfFalse(pParse, pExpr->pRight, dest);
      sqliteVdbeResolveLabel(v, d2);
      break;
    }
    case TK_NOT: {
      sqliteExprIfTrue(pParse, pExpr->pLeft, dest);
      break;
    }
    case TK_LT:
    case TK_LE:
    case TK_GT:
    case TK_GE:
    case TK_NE:
    case TK_EQ: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteExprCode(pParse, pExpr->pRight);
      sqliteVdbeAddOp(v, op, 0, dest, 0, 0);
      break;
    }
    case TK_LIKE:
    case TK_GLOB: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteExprCode(pParse, pExpr->pRight);
      sqliteVdbeAddOp(v, op, 1, dest, 0, 0);
      break;
    }
    case TK_ISNULL:
    case TK_NOTNULL: {
      sqliteExprCode(pParse, pExpr->pLeft);
      sqliteVdbeAddOp(v, op, 0, dest, 0, 0);
      break;
    }
    default: {
      sqliteExprCode(pParse, pExpr);
      sqliteVdbeAddOp(v, 69, 0, 0, 0, 0);
      sqliteVdbeAddOp(v, 39, 0, dest, 0, 0);
      break;
    }
  }
}
