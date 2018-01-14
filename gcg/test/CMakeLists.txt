ENABLE_TESTING ()

INCLUDE_DIRECTORIES (../include ../../lib/libcflora/include ../../lib/libtp3/include ../../lib/libgnode/include /usr/local/include)

SET (SRCS ../src/gcg_connection.cpp ../src/gcg_config.cpp ../src/gcg_server.cpp)

FIND_LIBRARY (UVLIB NAMES uv PATHS /usr/local/lib)
FIND_LIBRARY (CFLIB NAMES cflora PATHS ../../lib/libcflora/release/lib)
FIND_LIBRARY (TP3LIB NAMES tp3 PATHS ../../lib/libtp3/release/lib)

ADD_EXECUTABLE (test_device test_device.c ${SRCS})
TARGET_LINK_LIBRARIES (test_device ${TP3LIB})
TARGET_LINK_LIBRARIES (test_device ${UVLIB})
TARGET_LINK_LIBRARIES (test_device ${CFLIB} ${GLOG_LIBRARIES})

ADD_EXECUTABLE (test_conn test_conn.c ${SRCS})
TARGET_LINK_LIBRARIES (test_conn ${TP3LIB})
TARGET_LINK_LIBRARIES (test_conn ${UVLIB})
TARGET_LINK_LIBRARIES (test_conn ${CFLIB} ${GLOG_LIBRARIES})

ADD_EXECUTABLE (test_msgbuf test_msgbuf.c ${SRCS})
TARGET_LINK_LIBRARIES (test_msgbuf ${TP3LIB})
TARGET_LINK_LIBRARIES (test_msgbuf ${UVLIB})
TARGET_LINK_LIBRARIES (test_msgbuf ${CFLIB} ${GLOG_LIBRARIES})

ADD_EXECUTABLE (test_control test_control.c ${SRCS})
TARGET_LINK_LIBRARIES (test_control ${TP3LIB})
TARGET_LINK_LIBRARIES (test_control ${UVLIB})
TARGET_LINK_LIBRARIES (test_control ${CFLIB} ${GLOG_LIBRARIES})

ADD_TEST (unittest1 test_device)
ADD_TEST (unittest2 test_conn)
ADD_TEST (unittest3 test_msgbuf)
ADD_TEST (unittest4 test_control)

