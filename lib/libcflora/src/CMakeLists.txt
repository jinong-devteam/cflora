CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (libcflora)

SET (LIBRARY_OUTPUT_PATH ../lib)

INCLUDE_DIRECTORIES (../include ../util)

SET (LIBSRCS cf_base.c cf_db_mysql.c cf_msgbuf.c cf_control.c cf_config.c)

ADD_LIBRARY (cflora STATIC ${LIBSRCS})
