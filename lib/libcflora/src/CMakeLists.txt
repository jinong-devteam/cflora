# Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
# 기존 파일을 살짝 수정함.

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (libcflora)

SET (LIBRARY_OUTPUT_PATH ../lib)

INCLUDE_DIRECTORIES (../include ../util)

SET (LIBSRCS cf_base.c cf_db_mysql.c cf_msgbuf.c cf_config.c)

ADD_LIBRARY (cflora STATIC ${LIBSRCS})
