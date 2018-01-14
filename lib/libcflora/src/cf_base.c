/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file cf_base.c
 * \brief 공통라이브러리 베이스 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <cf_base.h>

void 
cf_errormsg(const char *errfmt, ...) {
  va_list argptr;

  fflush(stdout);

  va_start(argptr, errfmt);
  vfprintf(stderr, errfmt, argptr);
  va_end(argptr);

  fflush(stderr);  // redundant
}

#ifdef __CF_MEM_DEBUG__

void *
cf_malloc (size_t size, char *file, int line) {
    void *ptr;

    cf_errormsg ("cfmem m %s:%d alloc ", file, line);
    ptr = malloc (size);
    cf_errormsg ("cfmem m (%p) size (%d)\n", ptr, (int)size);
    return ptr;
}

void *
cf_realloc (void *ptr, size_t size, char *file, int line) {
    void *nptr;

    cf_errormsg ("cfmem r %s:%d alloc ", file, line);
    nptr = realloc (ptr, size);
    cf_errormsg ("cfmem r (%p) size (%d)\n", nptr, (int)size);
    return nptr;
}

void
cf_free (void *ptr, char *file, int line) {
    cf_errormsg ("cfmem f %s:%d free (%p)\n", file, line, ptr);
    free (ptr);
}

#endif


