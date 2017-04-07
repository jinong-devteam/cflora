#ifndef _CFLORA_BASE_H_
#define _CFLORA_BASE_H_

#include <time.h>

#ifdef _MSC_VER
char * strptime(const char *s, const char *format, struct tm *tm) ;
#endif

#ifndef TRUE
typedef enum {
	FALSE = 0,
	TRUE = 1
} cf_bool_t;
#endif

/** 보통의 CF 함수를 위한 출력값 정의 */
typedef enum {
	CF_OK = 0,		///< 성공
	CF_ERR = 1		///< 에러
} cf_ret_t;

/** CF Verbose 표시. */
typedef enum {
	CF_VERBOSE_NO = 0,
	CF_VERBOSE_LOW = 1,
	CF_VERBOSE_MID = 2,
	CF_VERBOSE_HIGH = 3,
} cf_verbose_t;

#define CF_VERBOSE(level,...)	\
	do {	\
		if (level <= cf_get_verbose ()) {	\
			char date_time[20]  ; \
			time_t cv_time = time(NULL) ; \
			struct tm *ptm = localtime(&cv_time); \
			strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
			fprintf (stderr, "[%s] ",date_time);	\
			fprintf (stderr, __VA_ARGS__);	\
			fprintf (stderr, " (%s :%d).\n", __FILE__, __LINE__);	\
			fflush (stderr);\
		}	\
	} while (0);

void
cf_set_verbose (cf_verbose_t level);

cf_verbose_t
cf_get_verbose ();

/** CF 로그 표시. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_ERR_LOG(...)	\
	do {                        \
		char date_time[20]  ; \
		time_t cv_time = time(NULL) ; \
		struct tm *ptm = localtime(&cv_time); \
		strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
		fprintf (stderr, "[%s] ",date_time);	\
		fprintf (stderr, "Error (");\
		fprintf (stderr, __VA_ARGS__);	\
		fprintf (stderr, ") occured in %s on line %d.\n", __FILE__, __LINE__); \
		fflush (stderr);\
	} while (0);

/** CF 에러 표시 void 리턴. expr이 참이면 msg를 출력하고 void 리턴. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXP_VRETURN(expr,msg)  \
	do {                        \
		if ((expr)) {           \
			char date_time[20]  ; \
			time_t cv_time = time(NULL) ; \
			struct tm *ptm = localtime(&cv_time); \
			strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
			fprintf (stderr, "[%s] ",date_time);	\
			fprintf (stderr, "Error (%s) occured in %s on line %d.\n",      \
					msg, __FILE__, __LINE__);              \
			fflush (stderr);\
			return ;                 \
		}                               \
	} while (0);

/** CF 에러 표시 리턴. expr이 참이면 msg를 출력하고 CF_ERR 리턴. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_ERR_RETURN(expr,...)  \
	do {                        \
		if (CF_ERR == (expr)) {           \
			char date_time[20]  ; \
			time_t cv_time = time(NULL) ; \
			struct tm *ptm = localtime(&cv_time); \
			strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
			fprintf (stderr, "[%s] ",date_time);	\
			fprintf (stderr, "Error (");\
			fprintf (stderr, __VA_ARGS__);	\
			fprintf (stderr, ") occured in %s on line %d.\n", __FILE__, __LINE__); \
			fflush (stderr);\
			return CF_ERR;                 \
		}                               \
	} while (0);

/** void 리턴. expr이 참이면 msg를 출력하고 void 리턴. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_ERR_VOIDRETURN(expr,...)  \
	do {                        \
		if (CF_ERR == (expr)) {           \
			char date_time[20]  ; \
			time_t cv_time = time(NULL) ; \
			struct tm *ptm = localtime(&cv_time); \
			strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
			fprintf (stderr, "[%s] ",date_time);	\
			fprintf (stderr, "Error (");\
			fprintf (stderr, __VA_ARGS__);	\
			fprintf (stderr, ") occured in %s on line %d.\n", __FILE__, __LINE__); \
			fflush (stderr);\
			return ;                 \
		}                               \
	} while (0);

/** CF 에러 표시 ret 리턴. expr이 참이면 msg를 출력하고 ret 리턴. 추후에 fprintf 를 적절한 로깅함수로 변경할 예정 */
#define CF_EXP_RETURN(expr,ret,...)  \
	do {                        \
		if ((expr)) {           \
			char date_time[20]  ; \
			time_t cv_time = time(NULL) ; \
			struct tm *ptm = localtime(&cv_time); \
			strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
			fprintf (stderr, "[%s] ",date_time);	\
			fprintf (stderr, "Error (");\
			fprintf (stderr, __VA_ARGS__);	\
			fprintf (stderr, ") occured in %s on line %d.\n", __FILE__, __LINE__); \
			fflush (stderr);\
			return ret;                 \
		}                               \
	} while (0);

/** Assertion Test */
#define CF_ASSERT(expr,msg)											\
	do {                                                     	\
		if (!(expr)) {                                          \
			char date_time[20]  ; \
			time_t cv_time = time(NULL) ; \
			struct tm *ptm = localtime(&cv_time); \
			strftime(date_time, sizeof(date_time),"%Y-%m-%d %H:%M:%S", ptm); \
			fprintf (stderr, "[%s] ",date_time);	\
			fprintf(stderr,                                     \
				"Assertion failed in %s on line %d: %s\n\t%s\n",    	\
				__FILE__,                                     	\
				__LINE__,                                     	\
				#expr, msg);                                       	\
			fflush(stderr);                                       	\
			abort();                                            \
		}                                                       \
	} while (0)

/** 향후 메모리 누수 테스트를 위한 장치 */
#ifdef __TP3_MEM_DEBUG__
	#define CF_MALLOC(x) mem_malloc(x,__FILE__,__LINE__)
	#define CF_REALLOC(x,y) mem_realloc(x,y,__FILE__,__LINE__) ; 
	#define CF_FREE(x) mem_free(x,__FILE__,__LINE__)
#else
	// 메모리 0 초기화 => 초기화 안된 것이 문제를 일으킬 수 있음 by chj
	#define CF_MALLOC(x) calloc(1,x) 
//	#define CF_MALLOC(x) malloc(x)
	#define CF_REALLOC(x,y) realloc(x,y)
	#define CF_FREE(x) free(x)
#endif
	
#endif
