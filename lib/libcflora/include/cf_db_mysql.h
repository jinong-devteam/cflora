#ifdef USE_MYSQL
#ifndef _CFLORA_DATABASE_MYSQL_H_
#define _CFLORA_DATABASE_MYSQL_H_

#ifdef _MSC_VER
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif
#include <stdarg.h>
#include <string.h>

/** sqlite3 트랜젝션 시작. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_BEGIN_TRANSACTION(d)	\
	do {	\
		int rc;	\
		rc = mysql_query(&(d)->db, "START TRANSACTION"); \
		if ( rc != 0 ) { \
			cf_errorMsg("database transaction start failed.(%s)\n",mysql_error(&(d)->db) ); \
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 트랜젝션 종료. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_END_TRANSACTION(d)	\
	do {	\
		int rc;	\
		rc = mysql_query(&(d)->db, "COMMIT"); \
		if ( rc != 0 ) { \
			cf_errorMsg("database transaction end failed.(%s)\n",mysql_error(&(d)->db) ); \
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 트랜젝션 롤백. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_ROLLBACK(d)	\
	do {	\
		int rc;	\
		rc = mysql_query(&(d)->db, "ROLLBACK"); \
		if ( rc != 0 ) { \
			cf_errorMsg("database transaction rollback failed.(%s)\n",mysql_error(&(d)->db) ); \
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 쿼리 실행. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXECUTE(d,q)	\
	do {	\
		int rc;	\
		rc = mysql_query(&(d)->db, q); \
		if ( rc != 0 ) { \
			cf_errorMsg("database query execution failed : %s (%s).", mysql_error(&(d)->db), q);	\
			cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
			return CF_ERR;	\
		} \
	} while (0);

/** sqlite3 쿼리를 실행하고 첫번째 선택된 값을 정수형으로 받아옴. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXECUTE_GET_INT(d,q,ref)	\
	do {								\
		int rc;	\
		MYSQL_RES *res; \
		MYSQL_ROW row; \
		rc = mysql_query(&(d)->db, q); \
		if ( rc != 0 ) {  \
			cf_errorMsg("database query execution failed : %s (%s).", mysql_error(&(d)->db), q);	\
			cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
			return CF_ERR;	\
		} \
		res = mysql_store_result(&(d)->db); \
		if ( res ) { \
			row = mysql_fetch_row(res); \
			if ( row == 0 ) { \
				cf_errorMsg("database Fetch failed : %s (%s).", mysql_error(&(d)->db), q);	\
				cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
				return CF_ERR; \
			} \
			*(ref) = atoi (row[0]);		\
			mysql_free_result(res); \
		} \
		else { \
			cf_errorMsg("database store_result failed : %s (%s).", mysql_error(&(d)->db), q);	\
			cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
			return CF_ERR; \
		} \
	} while (0);

/** sqlite3 쿼리를 실행하고 첫번째 선택된 값을 문자열형으로 받아옴. 문자열의 길이는 충분하다고 가정. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXECUTE_GET_TEXT(d,q,str)	\
	do {								\
		int rc;	\
		MYSQL_RES *res; \
		MYSQL_ROW row; \
		rc = mysql_query(&(d)->db, q); \
		if ( rc != 0 ) {  \
			cf_errorMsg("database query execution failed : %s (%s).", mysql_error(&(d)->db), q);	\
			cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
			return CF_ERR;	\
		} \
		res = mysql_store_result(&(d)->db); \
		if ( res ) { \
			row = mysql_fetch_row(res); \
			if ( row == 0 ) { \
				cf_errorMsg("database Fetch failed : %s (%s).", mysql_error(&(d)->db), q);	\
				cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
				return CF_ERR; \
			} \
			strcpy (str, row[0]);		\
			mysql_free_result(res); \
		} \
		else { \
			cf_errorMsg("database store_result failed : %s (%s).", mysql_error(&(d)->db), q);	\
			cf_errorMsg(" (%s :%d).\n", __FILE__, __LINE__);	\
			return CF_ERR; \
		} \
	} while (0);

/**
 * database 에서 얻어진 datetime 문자열을 time_t로 변환하는 함수
 * @param datetime_string datetime 문자열
 * @return 변환된 time_t.
 * @see strptime,mktime
 */
int
cf_db_timet( const char *datetime_string ) ;


/**
 * time_t를 database로 전달할 datetime 문자열로 변환하는 함수
 * @param datetime_string datetime 문자열
 * @return 변환된 time_t.
 * @see strftime,localtime
 */
int
cf_db_timestring( char *buffer, int buffer_max , int timet ) ;

/**
 * sqlite3 데이터베이스 처리시 락을 획득할 수 없는 경우에 호출되는 함수.
 * 디폴트로 3회 재시도를 수행함.
 * @param handle 출력할 문자열
 * @param ntry 재시도 회수
 * @return 0 이면 재시도를 수행하지 않음.
 * @see sqlite3_busy_handler
 */
int
cf_db_busy (void *handle, int ntry); 

/** 디비 핸들러 */
typedef struct {
	MYSQL db;	///< sqlite 핸들러
	char *dbstr;	///< 디비 문자열
	char *pHost ;
	char *pDatabase ;
	char *pUser ;
	char *pPassword ;
	char *pPort ;
} cf_db_t;

/**
 * 디비 핸들러를 초기화 한다.
 * @param pdb 데이터베이스 핸들러
 * @param dbstr 디비 문자열( Server=myServerAddress;Database=myDataBase;Uid=myUsername;Pwd=myPassword; ) 
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_init_db (cf_db_t *pdb, char *dbstr);

/**
 * 디비 핸들러를 해제한다.
 * @param pdb 데이터베이스 핸들러
 */
void
cf_release_db (cf_db_t *pdb);

/**
 * 디비를 연다.
 * @param pdb 데이터베이스 핸들러
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_db_open (cf_db_t *pdb);

/**
 * 디비를 닫는다.
 * @param pdb 데이터베이스 핸들러
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_db_close (cf_db_t *pdb);

/**
 * 테이블 버퍼를 지운다
 * @param result row[row*col+col_idx]
 */
void 
free_table(char **result) ;

/**
 * 에러 메세지 출력
 * @param pdb 에러 메시지 포맷, 가변 아규먼트
 */
void 
cf_errorMsg(const char *errFmt, ...) ;


#define cf_db_stmt	MYSQL_STMT

int 
mysql_get_table(
  MYSQL *db,          /* An open database */
  const char *zSql,     /* SQL to be evaluated */
  char ***pazResult,    /* Results of the query */
  int *pnRow,           /* Number of result rows written here */
  int *pnColumn,        /* Number of result columns written here */
  char **pzErrmsg       /* Error msg written here */
  ) ;
int 
mysql_prepare(
  cf_db_t *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  cf_db_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
  ) ;
int mysql_bind_int(cf_db_stmt* stmt, int idx, int value) ;
int mysql_bind_null(cf_db_stmt* stmt, int idx) ;
int mysql_bind_text(cf_db_stmt* stmt,int idx ,const char* value,int length,void(* pFunc)(void*) ) ;
void mysql_free_table(char **result) ;
int mysql_db_exec(MYSQL *db, const char *query,char **errMsg) ;
int mysql_step(cf_db_stmt* stmt) ;

/* 이하는 랩핑 - 추후 다른 데이터베이스 사용시 쉬운 변경을 위한 트릭 */


#define cf_db_last_insert_rowid(d) mysql_insert_id(&(d)->db)
#define cf_db_errmsg(d) mysql_error(&(d)->db)
#define cf_db_exec(d,q,f,n,e)  mysql_db_exec(&(d)->db, q , e )
#define cf_db_busy_handler(d,f,m) 
#define cf_db_busy_timeout(d,tm) 
// mysql 인터페이스 함수
extern MYSQL_BIND	*g_pBind ;
#define cf_db_prepare_v2(d,q,l,st,t) mysql_prepare( d,q,l,st,t)
#define cf_db_step(st) mysql_step(st)
#define cf_db_bind_int(st,i,v) mysql_bind_int(st,i,v)
#define cf_db_bind_null(st,i) mysql_bind_null(st,i)
#define cf_db_bind_text(st,i,s,l,f) mysql_bind_text(st,i,s,l,f)

#define cf_db_get_table(d,q,v,r,c,e) mysql_get_table(&(d)->db, q, v, r, c, e)
#define cf_db_free_table(r) mysql_free_table(r)
#define cf_db_free(e)  \
	do { \
		if ( e ) CF_FREE(e) ; \
	} while (0) ;
// mysql 인터페이스 함수

	
#define cf_db_reset(st) \
	do { \
		int param_count= mysql_stmt_param_count(st) ; \
		if ( g_pBind ) memset(g_pBind, 0, sizeof(MYSQL_BIND)*param_count); \
		mysql_stmt_reset(st) ; \
	} while (0) ;
	
#define cf_db_finalize(st) \
	do { \
		if ( g_pBind ) { \
			int param_count= mysql_stmt_param_count(st) ; \
			for ( ; param_count ; param_count-- ) { \
				if ( g_pBind[param_count-1].buffer ) CF_FREE( g_pBind[param_count-1].buffer ); \
				if ( g_pBind[param_count-1].length ) CF_FREE( g_pBind[param_count-1].length ); \
			} \
			CF_FREE( g_pBind ) ; \
		} \
		g_pBind = NULL ; \
		mysql_stmt_free_result(st); \
		mysql_stmt_close(st); \
	} while(0) ;

#endif
#endif
