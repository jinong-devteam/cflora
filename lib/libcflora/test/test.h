/**
 * \file test.h
 * \brief libcflora 를 테스트하기 위해 사용되는 헤더파일
 */
#ifndef _CF_TEST_H_
#define _CF_TEST_H_

#include <stdlib.h>
#include <string.h>

/* Die with fatal error. */
#define FATAL(msg)                                        		\
	do {                                                    	\
		fprintf(stderr,                                       	\
			"Fatal error in %s on line %d: %s\n",         		\
			__FILE__,                                     		\
			__LINE__,                                     		\
			msg);                                         		\
		fflush(stderr);                                       	\
		abort();                                              	\
	} while (0)

/* No error Assertion Test */
#define ASSERT_OK(expr,msg)											\
	do {                                                     	\
		cf_ret_t __stat;  \
		if (CF_OK != (__stat = expr)) {                                          \
			fprintf(stderr,                                     \
				"Assertion failed in %s on line %d: %s\n\t%s\n",   	\
				__FILE__,                                     	\
				__LINE__,                                     	\
				#expr, msg);                                       	\
			fflush(stderr);                                       	\
			abort();                                            \
		}                                                       \
	} while (0)

/* Assertion Test */
#define ASSERT(expr,msg)											\
	do {                                                     	\
		if (!(expr)) {                                          \
			fprintf(stderr,                                     \
				"Assertion failed in %s on line %d: %s\n\t%s\n",    	\
				__FILE__,                                     	\
				__LINE__,                                     	\
				#expr, msg);                                       	\
			fflush(stderr);                                       	\
			abort();                                            \
		}                                                       \
	} while (0)

/* Test Run */
#define TEST(expr)											\
	do {                                                     	\
		if (CF_OK == (expr)) {                                          \
			fprintf(stderr,                                     \
				"%s test pass in %s on line %d\n",    	\
				#expr,                                       	\
				__FILE__,                                     	\
				__LINE__);                                     	\
			fflush(stderr);                                       	\
		}                                                       \
	} while (0)

#endif //_CF_TEST_H_
