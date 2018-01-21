/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file cf_db_mysql.c
 * \brief mysql 디비 관련 공통라이브러리 파일. 기존 코드를 수정했음.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_MYSQL

#include "cf_base.h"
#include "cf_db_mysql.h"

int
cf_db_timet( const char *datetime_string ) {
    struct tm stm;
    return (int)mktime(&stm);  // t is now your desired time_t
}

int
cf_db_timestring( char *buffer, int buffer_max , int timet ) {
    time_t cv_time = timet ; // localtime  변환 버퍼 할당 by chj
    struct tm *ptm = localtime(&cv_time);
    return strftime(buffer, buffer_max,"%Y-%m-%d %H:%M:%S", ptm);
}

int
cf_db_busy (void *handle, int ntry) {
    if (handle == NULL)
        cf_errormsg ("db busy [%d/3]\n", ntry);
    else
        cf_errormsg ("db busy [%d/3] : %s\n", ntry, (char *)handle);
    return 3 - ntry;
}

ret_t
cf_init_db (cf_db_t *pdb, char *dbstr) {
// ( HOST;DATABASE;USER;PASSWD;PORT; ) 
    char *conInfo ;
    char idx = 0 ;
    mysql_init(&pdb->db);
    pdb->dbstr = strdup (dbstr);

    conInfo = strtok(dbstr,";") ;
    while( conInfo != NULL ) 
    {
        switch ( idx ) {
            case 0 :
                pdb->pHost = conInfo ;
                break ;
            case 1 :
                pdb->pDatabase = conInfo  ;
                break ;
            case 2 :
                pdb->pUser = conInfo ;
                break ;
            case 3 :
                pdb->pPassword = conInfo ;
                break ;
            case 4 :
                pdb->pPort = conInfo ;
                break ;
        }
        idx ++ ;
        conInfo = strtok(NULL, ";");
    }    
    return OK;
}

void
cf_release_db (cf_db_t *pdb) {
    cf_db_close (pdb);
    CF_FREE (pdb->dbstr);
}

ret_t
cf_db_open (cf_db_t *pdb) {
// ( HOST;DATABASE;USER;PASSWD;PORT; ) 

    if (0 == mysql_real_connect ( &pdb->db, 
                pdb->pHost !=NULL?pdb->pHost :"" , // HOST
                pdb->pUser !=NULL?pdb->pUser :"" , // USER
                pdb->pPassword !=NULL?pdb->pPassword :"" , // PASSWD
                pdb->pDatabase !=NULL?pdb->pDatabase :"" , // DATABASE
                atoi(pdb->pPort !=NULL?pdb->pPort :"0") , // PORT
                NULL , 0)) {
        cf_errormsg ("DB open failed");
        return ERR;
    };
    return OK;
}

ret_t
cf_db_close (cf_db_t *pdb) {
    mysql_close(&pdb->db) ;
    return OK;
}

MYSQL_BIND    *g_pBind = NULL ;

int 
mysql_prepare (cf_db_t *db, const char *zSql, int nByte, cf_db_stmt **ppStmt, const char **pzTail) {
    int param_count;

    if (g_pBind) 
        CF_FREE(g_pBind);

    g_pBind = NULL ;
    *ppStmt = mysql_stmt_init( &db->db  );
    if (! *ppStmt) {
        cf_errormsg ("mysql_stmt_init(), out of memory");    
        cf_errormsg (" (%s :%d).\n", __FILE__, __LINE__);    
        return ERR;
    }

    if ( mysql_stmt_prepare(*ppStmt, zSql, nByte ) ) {
        cf_errormsg ( " mysql_stmt_prepare(), failed  : %s (%s)" ,mysql_stmt_error(*ppStmt), zSql );
        cf_errormsg (" (%s :%d).\n", __FILE__, __LINE__);    
        return ERR;
    }

    param_count= mysql_stmt_param_count(*ppStmt) ;
    if ( param_count ) {
        g_pBind = (MYSQL_BIND *)CF_MALLOC( sizeof(MYSQL_BIND)*param_count) ;
        memset(g_pBind, 0, sizeof(MYSQL_BIND)*param_count);
    }

    return OK;
}

int 
mysql_bind_int(cf_db_stmt* stmt, int idx, int value) {
    int param_count= mysql_stmt_param_count(stmt) ;

    if (g_pBind == NULL) 
        return OK;
    if (idx >= param_count) 
        return ERR; 

    if (g_pBind[idx].buffer)
        CF_FREE( g_pBind[idx].buffer );
    if (g_pBind[idx].length)
        CF_FREE( g_pBind[idx].length );

    g_pBind[idx].buffer = CF_MALLOC(sizeof(int));
    *(int*)g_pBind[idx].buffer = value;
    g_pBind[idx].buffer_type = MYSQL_TYPE_LONG;
    g_pBind[idx].is_null = 0;
    g_pBind[idx].length = 0;
    return OK;
}

int 
mysql_bind_null(cf_db_stmt* stmt, int idx) {
    int param_count = mysql_stmt_param_count(stmt) ;

    if ( g_pBind == NULL ) 
        return OK;
    if ( idx >= param_count ) 
        return ERR; 

    if (g_pBind[idx].buffer) 
        CF_FREE( g_pBind[idx].buffer );
    if (g_pBind[idx].length) 
        CF_FREE( g_pBind[idx].length );

    g_pBind[idx].buffer_type = MYSQL_TYPE_NULL;
    g_pBind[idx].buffer = 0 ;
    g_pBind[idx].is_null = 0;
    g_pBind[idx].length = 0;
    return OK;
}

int 
mysql_bind_text(cf_db_stmt* stmt, int idx, const char* value, int length, void(* pFunc)(void*)) {
    int param_count= mysql_stmt_param_count(stmt);

    if ( g_pBind == NULL ) 
        return OK;
    if ( idx >= param_count ) 
        return ERR; 

    if ( g_pBind[idx].buffer ) 
        CF_FREE (g_pBind[idx].buffer);
    if ( g_pBind[idx].length ) 
        CF_FREE (g_pBind[idx].length);

    g_pBind[idx].length = CF_MALLOC( sizeof(unsigned long));
    g_pBind[idx].buffer = CF_MALLOC( length +1 );
    g_pBind[idx].buffer_type = MYSQL_TYPE_STRING;
    strncpy (g_pBind[idx].buffer, value, length);
    ((char*)g_pBind[idx].buffer)[length] = '\0';
    g_pBind[idx].is_null = 0;
    *(unsigned long*)g_pBind[idx].length = length;

    return OK;
}

int 
mysql_get_table(MYSQL *db, const char *zSql, char ***pazResult, int *pnRow, int *pnColumn, char **pzErrmsg) {
    int rc , row_idx, col  , max ;
    MYSQL_RES *res; 
    MYSQL_ROW row ; 
    MYSQL_FIELD *fields;

    rc = mysql_query(db, zSql); 
    *pnColumn = 0 ;
    *pnRow = 0 ;
    *pazResult = NULL ;
    *pzErrmsg = NULL ;

    if ( rc != 0 ) {  
        cf_errormsg ("database query mysql_get_table failed : %s (%s).", mysql_error(db), zSql);    
        cf_errormsg (" (%s :%d).\n", __FILE__, __LINE__);    
        return ERR;
    } 

    res = mysql_store_result(db); 
    *pnColumn = mysql_num_fields(res);
    *pnRow = mysql_num_rows(res) ;
    if ( res ) { 
        max = -9999 ;
        fields = mysql_fetch_fields(res);
        for( col = 0; col < *pnColumn ; col++)  {
            if ( max <  (int) fields[col].max_length ) max =  (int) fields[col].max_length ;
            if ( max < strlen(fields[col].name) ) max = strlen(fields[col].name) ;
        }
        max = max + 1 ;
        *pazResult = (char**)CF_MALLOC( sizeof(char*) * (*pnRow+1) *(*pnColumn) + 1*sizeof(char*)) ; //어레이 종료 확인 포인터
        for( row_idx = 0; row_idx < *pnRow+1 ; row_idx++)  {
            for( col = 0; col < *pnColumn ; col++)  {
                (*pazResult)[row_idx*(*pnColumn)+col] = (char*)CF_MALLOC( sizeof(char)*max ) ;
                (*pazResult)[row_idx*(*pnColumn)+col][0] = '\0' ;
            }
        }
        (*pazResult)[row_idx*(*pnColumn)]  = NULL ;
        row_idx = 0 ;
        for( col = 0; col < *pnColumn ; col++)  {
            strcpy((*pazResult)[row_idx*(*pnColumn)+col], fields[col].name)  ;
        }

        while ( (row = mysql_fetch_row(res) ) ) {
            row_idx ++  ;
            for( col = 0; col < *pnColumn ; col++)  {
                if ( row[col] == NULL ) {
                    (*pazResult)[row_idx*(*pnColumn)+col][0] = '\0' ;
                }
                else {
                    strcpy((*pazResult)[row_idx*(*pnColumn)+col], row[col])  ;
                }
            }
        }            
        mysql_free_result(res);
    } else {
        cf_errormsg ("mysql error : %s\n", mysql_error(db));
    }
    return OK;
}

int 
mysql_step(cf_db_stmt* stmt) {
    if (mysql_stmt_bind_param(stmt, g_pBind)) {
        cf_errormsg (" mysql_stmt_bind_param() failed ");
        cf_errormsg (" %s\n", mysql_stmt_error(stmt));
        return ERR;
    }
    mysql_stmt_execute(stmt) ;
    return OK;
}

//#define cf_db_exec(d,q,f,n,e) mysql_query(&(d)->db, q); *(e) = strdup(mysql_error(&(d)->db) );
int 
mysql_db_exec(MYSQL *db, const char *query,char **errMsg) {
    int rc = mysql_query(db, query);
    if ( errMsg != NULL ) {
        cf_errormsg ("mysql error : %s\n", mysql_error(db));
    }
    return rc ;
}

void 
mysql_free_table(char **result) {
    if ( result )  {
        int idx = 0  ;
        for ( idx = 0 ; ; idx ++ ) {
            if ( result[idx] == NULL ) break ;
            CF_FREE( result[idx]  ) ;
            result[idx] = NULL ;
        }
        CF_FREE( result ) ;
    }
}

#endif

