ENABLE_TESTING ()

INCLUDE_DIRECTORIES (../include ../../libtp3/include /usr/local/include ../../libcflora/include)

FIND_LIBRARY (GOSLIB NAMES gostest PATHS ../lib)
FIND_LIBRARY (TP3LIB NAMES tp3 PATHS ../../libtp3/lib)
FIND_LIBRARY (CFLIB NAMES cflora PATHS ../../libcflora/lib)
FIND_LIBRARY (UVLIB NAMES uv PATH /usr/local/lib)

ADD_EXECUTABLE (test_device test_device.c)
TARGET_LINK_LIBRARIES (test_device ${GOSLIB})
TARGET_LINK_LIBRARIES (test_device ${TP3LIB})
TARGET_LINK_LIBRARIES (test_device ${UVLIB})
TARGET_LINK_LIBRARIES (test_device ${SQLITELIB})

ADD_EXECUTABLE (test_conn test_conn.c)
TARGET_LINK_LIBRARIES (test_conn ${GOSLIB})
TARGET_LINK_LIBRARIES (test_conn ${TP3LIB})
TARGET_LINK_LIBRARIES (test_conn ${UVLIB})
TARGET_LINK_LIBRARIES (test_conn ${SQLITELIB})

ADD_EXECUTABLE (test_msgbuf test_msgbuf.c)
TARGET_LINK_LIBRARIES (test_msgbuf ${GOSLIB})
TARGET_LINK_LIBRARIES (test_msgbuf ${TP3LIB})
TARGET_LINK_LIBRARIES (test_msgbuf ${UVLIB})
TARGET_LINK_LIBRARIES (test_msgbuf ${SQLITELIB})

ADD_EXECUTABLE (test_control test_control.c)
TARGET_LINK_LIBRARIES (test_control ${GOSLIB})
TARGET_LINK_LIBRARIES (test_control ${TP3LIB})
TARGET_LINK_LIBRARIES (test_control ${UVLIB})
TARGET_LINK_LIBRARIES (test_control ${CFLIB})
TARGET_LINK_LIBRARIES (test_control ${SQLITELIB})
TARGET_LINK_LIBRARIES (test_control m)

ADD_TEST (unittest1 test_device)
ADD_TEST (unittest2 test_conn)
ADD_TEST (unittest3 test_msgbuf)
ADD_TEST (unittest4 test_control)

