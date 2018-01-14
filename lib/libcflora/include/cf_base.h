/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file cf_base.h
 * \brief 공통라이브러리 베이스 파일. 기존 코드를 수정했음.
 */

#ifndef _CFLORA_BASE_H_
#define _CFLORA_BASE_H_

/** 커먼 출력값 정의 */
typedef enum {
	OK = 0,		///< 성공
	ERR = 1		///< 에러
} ret_t;

/**
 * 에러 메세지 출력
 * @param errfmt 에러 메시지 포맷
 * @param ... 가변 인자
 */
void 
cf_errormsg(const char *errfmt, ...);

/** 향후 메모리 누수 테스트를 위한 장치 */
#ifdef __CF_MEM_DEBUG__
    void *
    cf_malloc (size_t size, char *file, int line);
    void *
    cf_realloc (void *ptr, size_t size, char *file, int line);
    void
    cf_free (void *ptr, char *file, int line);

	#define CF_MALLOC(x) cf_malloc(x,__FILE__,__LINE__)
	#define CF_REALLOC(x,y) cf_realloc(x,y,__FILE__,__LINE__) ; 
	#define CF_FREE(x) cf_free(x,__FILE__,__LINE__)
#else
	#define CF_MALLOC(x) malloc(x)
	#define CF_REALLOC(x,y) realloc(x,y)
	#define CF_FREE(x) free(x)
#endif
	
#endif
