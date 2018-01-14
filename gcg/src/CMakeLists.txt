# Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
# 기존 파일을 살짝 수정함

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (gcg)

SET (EXECUTABLE_OUTPUT_PATH ../bin)
SET (LIBRARY_OUTPUT_PATH ../lib)

FIND_LIBRARY (UVLIB NAMES uv PATHS /usr/local/lib)
FIND_LIBRARY (CFLIB NAMES cflora PATHS ../../lib/libcflora/release/lib)
FIND_LIBRARY (TP3LIB NAMES tp3 PATHS ../../lib/libtp3/release/lib)

INCLUDE_DIRECTORIES (../include ../../lib/libcflora/include ../../lib/libtp3/include ../../lib/libgnode/include /usr/local/include)

SET (SRCS gcg_connection.cpp gcg_config.cpp gcg_server.cpp)

ADD_LIBRARY (gcgtest STATIC ${SRCS})

ADD_EXECUTABLE (gcg gcg.cpp ${SRCS})

TARGET_LINK_LIBRARIES (gcg ${TP3LIB})
TARGET_LINK_LIBRARIES (gcg ${UVLIB})
TARGET_LINK_LIBRARIES (gcg ${CFLIB})
TARGET_LINK_LIBRARIES (gcg ${GLOG_LIBRARIES})

