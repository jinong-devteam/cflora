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
	strptime(datetime_string, "%Y-%m-%d %H:%M:%S", &stm);
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
		fprintf(stderr, "db busy [%d/3]\n", ntry);
	else
		fprintf(stderr, "db busy [%d/3] : %s\n", ntry, (char *)handle);
	return 3 - ntry;
}

cf_ret_t
cf_init_db (cf_db_t *pdb, char *dbstr) {
// ( HOST;DATABASE;USER;PASSWD;PORT; ) 
	char *conInfo ;
	char *con_text[5] , idx = 0 ;
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
	return CF_OK;
}

void
cf_release_db (cf_db_t *pdb) {
	cf_db_close (pdb);
	CF_FREE (pdb->dbstr);
}

cf_ret_t
cf_db_open (cf_db_t *pdb) {
// ( HOST;DATABASE;USER;PASSWD;PORT; ) 

	CF_ERR_RETURN (0 == mysql_real_connect ( &pdb->db, 
			pdb->pHost !=NULL?pdb->pHost :"" , // HOST
			pdb->pUser !=NULL?pdb->pUser :"" , // USER
			pdb->pPassword !=NULL?pdb->pPassword :"" , // PASSWD
			pdb->pDatabase !=NULL?pdb->pDatabase :"" , // DATABASE
			atoi(pdb->pPort !=NULL?pdb->pPort :"0") , // PORT
			NULL , 0
			) , "DB open failed");
	return CF_OK;
}

cf_ret_t
cf_db_close (cf_db_t *pdb) {
	mysql_close(&pdb->db) ;
	return CF_OK;
}

MYSQL_BIND	*g_pBind = NULL ;

int 
mysql_prepare(
  cf_db_t *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  cf_db_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
  ) {
	  int param_count ;
	 if ( g_pBind ) CF_FREE( g_pBind ) ;
	 g_pBind = NULL ;
	*ppStmt = mysql_stmt_init( &db->db  );
	if (! *ppStmt)
	{
		cf_errorMsg("mysql_stmt_init(), out of memory");	
		cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	
		return CF_ERR;
	}
	if ( mysql_stmt_prepare(*ppStmt, zSql, nByte ) )
	{
		cf_errorMsg( " mysql_stmt_prepare(), failed  : %s (%s)" ,mysql_stmt_error(*ppStmt), zSql );
		cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	
		return CF_ERR;
	}
	param_count= mysql_stmt_param_count(*ppStmt) ;
	if ( param_count ) {
		g_pBind = (MYSQL_BIND *)CF_MALLOC( sizeof(MYSQL_BIND)*param_count) ;
		memset(g_pBind, 0, sizeof(MYSQL_BIND)*param_count);
	}

	return CF_OK;
}

int mysql_bind_int(cf_db_stmt* stmt, int idx, int value)
{
	int param_count= mysql_stmt_param_count(stmt) ;
	if ( g_pBind == NULL ) return CF_OK;
	if ( idx >= param_count ) 
		return CF_ERR; \
	if ( g_pBind[idx].buffer ) CF_FREE( g_pBind[idx].buffer ) ;
	if ( g_pBind[idx].length ) CF_FREE( g_pBind[idx].length ) ;
	g_pBind[idx].buffer = CF_MALLOC( sizeof(int) ) ;
	*(int*)g_pBind[idx].buffer= value ;
	g_pBind[idx].buffer_type= MYSQL_TYPE_LONG;
	g_pBind[idx].is_null= 0;
	g_pBind[idx].length= 0;
	return CF_OK;
}

int mysql_bind_null(cf_db_stmt* stmt, int idx)
{
	int param_count= mysql_stmt_param_count(stmt) ;
	if ( g_pBind == NULL ) return CF_OK;
	if ( idx >= param_count ) 
		return CF_ERR; \

	if ( g_pBind[idx].buffer ) CF_FREE( g_pBind[idx].buffer ) ;
	if ( g_pBind[idx].length ) CF_FREE( g_pBind[idx].length ) ;
	g_pBind[idx].buffer_type= MYSQL_TYPE_NULL;
	g_pBind[idx].buffer= 0 ;
	g_pBind[idx].is_null= 0;
	g_pBind[idx].length= 0;
	return CF_OK;
}
int mysql_bind_text(cf_db_stmt* stmt,int idx ,const char* value,int length,void(* pFunc)(void*) )
{
	int param_count= mysql_stmt_param_count(stmt) ;
	if ( g_pBind == NULL ) return CF_OK;
	if ( idx >= param_count ) 
		return CF_ERR; \

	if ( g_pBind[idx].buffer ) CF_FREE( g_pBind[idx].buffer ) ;
	if ( g_pBind[idx].length ) CF_FREE( g_pBind[idx].length ) ;
	g_pBind[idx].length = CF_MALLOC( sizeof(unsigned long) ) ;
	g_pBind[idx].buffer = CF_MALLOC( length +1 ) ;
	g_pBind[idx].buffer_type= MYSQL_TYPE_STRING;
	strncpy( g_pBind[idx].buffer , value , length) ;
	((char*)g_pBind[idx].buffer)[length] = '\0' ;
	g_pBind[idx].is_null= 0 ;
	*(unsigned long*)g_pBind[idx].length= length;
	return CF_OK;
}

  
int 
mysql_get_table(
  MYSQL *db,          /* An open database */
  const char *zSql,     /* SQL to be evaluated */
  char ***pazResult,    /* Results of the query */
  int *pnRow,           /* Number of result rows written here */
  int *pnColumn,        /* Number of result columns written here */
  char **pzErrmsg       /* Error msg written here */
  ) {
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
		cf_errorMsg("database query mysql_get_table failed : %s (%s).", mysql_error(db), zSql);	
		cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	
		return CF_ERR;
	} 
	res = mysql_store_result(db); 
	*pnColumn = mysql_num_fields(res);
	*pnRow = mysql_num_rows(res) ;
	if ( res ) { 
// 최대 컬럼 사이즈 계산
		max = -9999 ;
		fields = mysql_fetch_fields(res);
		for( col = 0; col < *pnColumn ; col++)  {
			if ( max <  (int) fields[col].max_length ) max =  (int) fields[col].max_length ;
			if ( max < strlen(fields[col].name) ) max = strlen(fields[col].name) ;
		}
		max = max + 1 ;
// 메모리 할당
		*pazResult = (char**)CF_MALLOC( sizeof(char*) * (*pnRow+1) *(*pnColumn) + 1*sizeof(char*)) ; //어레이 종료 확인 포인터
		for( row_idx = 0; row_idx < *pnRow+1 ; row_idx++)  {
			for( col = 0; col < *pnColumn ; col++)  {
				(*pazResult)[row_idx*(*pnColumn)+col] = (char*)CF_MALLOC( sizeof(char)*max ) ;
				(*pazResult)[row_idx*(*pnColumn)+col][0] = '\0' ;
			}
		}
		(*pazResult)[row_idx*(*pnColumn)]  = NULL ;
// 컬럼 이름 저장
		row_idx = 0 ;
		for( col = 0; col < *pnColumn ; col++)  {
			strcpy((*pazResult)[row_idx*(*pnColumn)+col], fields[col].name)  ;
		}

// 데이터 저장			
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
	} 
	else {
	}
	*pzErrmsg = strdup(mysql_error(db)) ;
	return CF_OK;
}

int mysql_step(cf_db_stmt* stmt)
{
	if (mysql_stmt_bind_param(stmt, g_pBind)) {
		cf_errorMsg(" mysql_stmt_bind_param() failed ");
		cf_errorMsg(" %s\n", mysql_stmt_error(stmt));
		return CF_ERR;
	}
	mysql_stmt_execute(stmt) ;
	return CF_OK;
}

//#define cf_db_exec(d,q,f,n,e) mysql_query(&(d)->db, q); *(e) = strdup(mysql_error(&(d)->db) );
int mysql_db_exec(MYSQL *db, const char *query,char **errMsg)
{
	int rc = mysql_query(db, query);
	if ( errMsg != NULL ) {
		*errMsg = strdup( mysql_error(db) ) ;
	}
	return rc ;
}

void mysql_free_table(char **result)
{
	if ( result )  {
		int idx = 0  ;
		for ( idx = 0 ; ; idx ++ ) {
			if ( result[idx] == NULL ) break ;
			CF_FREE( result[idx]  ) ;
			result[idx] = NULL ;
		}
		CF_FREE( result ) ;
	}
		//CF_MALLOC
}

void 
cf_errorMsg(const char *errFmt, ...)
{
  va_list argptr;

  fflush(stdout);

  va_start(argptr, errFmt);
  vfprintf(stderr, errFmt, argptr);
  va_end(argptr);

  fflush(stderr);  // redundant
  }

 /*
 cf_ret_t include_test()
 {
	  MYSQL *db ;          //An open database 
	  char ***pazResult ;    // Results of the query 
	  char **pzErrmsg ;      // Error msg written here 

  cf_db_stmt *pStmt ;
	cf_db_prepare_v2(db,"select",6,&pStmt,NULL) ;
	cf_db_bind_int(pStmt,1,1) ;
	cf_db_bind_null(pStmt,1) ;
	cf_db_bind_text(pStmt,1,"aaaa",4,NULL) ;
	cf_db_step(pStmt) ;
	cf_db_reset(pStmt) ;
	cf_db_finalize(pStmt) ;
	cf_db_free_table(*pazResult) ;
	cf_db_free(pzErrmsg) ;

	cf_db_exec(db,"select",NULL,NULL,pzErrmsg) ;
	char leng[50] ;
	int val ;
	cf_db_t *pdb ;
	CF_EXECUTE_GET_TEXT(pdb,"select",leng) ;
	CF_EXECUTE_GET_INT(pdb,"select",&val) ;
	CF_EXECUTE(pdb,"select") ;
	CF_ROLLBACK(pdb) ;
	CF_END_TRANSACTION(pdb) ;
	CF_BEGIN_TRANSACTION(pdb) ;
}

*/

#if 0
void main()
{
	cf_db_t db ;
	char leng[50] ;
	MYSQL con ;
	cf_init_mysql_db( &db , "127.0.0.1;gos;root;0419;" ) ;
	
	cf_db_mysql_open( &db ) ;
	CF_EXECUTE(&db , "insert into gos_farm (name, address, postcode, telephone, owner) values ('이지팜농원', '경기도 안양시 인덕원', '123-456', '031-880-4615', '박흔동')" ) ;

	CF_EXECUTE_GET_TEXT( &db , "select * from gos_farm" , leng ) ;
	printf("%s\n", leng ) ;
	
	cf_db_mysql_close( &db ) ;
#if 0	
    mysql_init(&con);
    if ( mysql_real_connect (
        &con,                 /* pointer to connection handler */
        NULL,       /* host to connect to */
        "root",       /* user name */
        "0419",          /* password */
        "gos",          /* database to use */
        0,          /* port (use default) */
        NULL,   /* socket (use default) */
        0)                                 /* flags (none) */
    ) {
        printf("Connection success\n");
    } else {
        fprintf(stderr, "Failed to connect to databases: Error: %s\n",
            mysql_error(&con));
    }

    mysql_close(&con);	
#endif
	
}
#endif

#endif