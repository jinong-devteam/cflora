# Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
# 기존 파일을 살짝 수정함

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (gos)

SET (EXECUTABLE_OUTPUT_PATH ../bin)
SET (LIBRARY_OUTPUT_PATH ../lib)

FIND_LIBRARY (CFLIB NAMES libcflora.a cflora PATHS ../../lib/libcflora/release/lib)
FIND_LIBRARY (TP3LIB NAMES libtp3_static.a tp3 PATHS ../../lib/libtp3/release/lib)
FIND_LIBRARY (UVLIB NAMES uv PATH /usr/local/lib)
FIND_LIBRARY (MYSQLLIB NAMES mysqlclient PATH /usr/lib/arm-linux-gnueabihf/)

INCLUDE_DIRECTORIES (../include ../../lib/libtp3/include ../../lib/libcflora/include /usr/local/include)

SET (SRCS gos_connection.cpp gos_rule.cpp gos_process.cpp gos_config.cpp gos_device.cpp gos_server.cpp gos_vsensor.cpp gos_time.cpp)

ADD_LIBRARY (gostest STATIC ${SRCS})

ADD_EXECUTABLE (gos gos.cpp ${SRCS})

TARGET_LINK_LIBRARIES (gos ${CFLIB})
TARGET_LINK_LIBRARIES (gos ${TP3LIB})
TARGET_LINK_LIBRARIES (gos ${UVLIB})
TARGET_LINK_LIBRARIES (gos ${MYSQLLIB})
TARGET_LINK_LIBRARIES (gos ${GLOG_LIBRARIES})
TARGET_LINK_LIBRARIES (gos dl)
TARGET_LINK_LIBRARIES (gos m)

