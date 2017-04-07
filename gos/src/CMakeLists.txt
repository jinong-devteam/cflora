CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (gos)

SET (EXECUTABLE_OUTPUT_PATH ../bin)
SET (LIBRARY_OUTPUT_PATH ../lib)

FIND_LIBRARY (CFLIB NAMES libcflora.a cflora PATHS ../../libcflora/release/lib)
FIND_LIBRARY (TP3LIB NAMES libtp3_static.a tp3 PATHS ../../libtp3/release/lib)
FIND_LIBRARY (UVLIB NAMES uv PATH /usr/local/lib)
FIND_LIBRARY (MYSQLLIB NAMES mysqlclient PATH /usr/lib/arm-linux-gnueabihf/)

INCLUDE_DIRECTORIES (../include ../../libtp3/include ../../libcflora/include /usr/local/include)

SET (SRCS gos_connection.c gos_rule.c gos_control.c gos_process.c gos_config.c gos_device.c gos_server.c gos_vsensor.c)

ADD_LIBRARY (gostest STATIC ${SRCS})

ADD_EXECUTABLE (gos gos.c ${SRCS})

TARGET_LINK_LIBRARIES (gos ${CFLIB})
TARGET_LINK_LIBRARIES (gos ${TP3LIB})
TARGET_LINK_LIBRARIES (gos ${UVLIB})
TARGET_LINK_LIBRARIES (gos ${MYSQLLIB})
TARGET_LINK_LIBRARIES (gos dl)
TARGET_LINK_LIBRARIES (gos m)

