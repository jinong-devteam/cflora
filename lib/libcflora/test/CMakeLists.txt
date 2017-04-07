
ENABLE_TESTING ()

INCLUDE_DIRECTORIES (../include ../control)

FIND_LIBRARY (CFLIB NAMES cflora PATHS ../lib)
FIND_LIBRARY (SQLITELIB NAMES sqlite3 PATH /usr/local/lib)

ADD_EXECUTABLE (test_msgbuf test_msgbuf.c)
TARGET_LINK_LIBRARIES (test_msgbuf ${CFLIB})

ADD_EXECUTABLE (test_db test_db.c)
TARGET_LINK_LIBRARIES (test_db ${CFLIB})
TARGET_LINK_LIBRARIES (test_db ${SQLITELIB})

ADD_EXECUTABLE (test_control test_control.c)
TARGET_LINK_LIBRARIES (test_control ${CFLIB})
TARGET_LINK_LIBRARIES (test_control ${SQLITELIB})

ADD_EXECUTABLE (test_config test_config.c)
TARGET_LINK_LIBRARIES (test_config ${CFLIB})
TARGET_LINK_LIBRARIES (test_config ${SQLITELIB})


ADD_TEST (unittest1 test_msgbuf)
ADD_TEST (unittest2 test_db)
ADD_TEST (unittest3 test_control)
ADD_TEST (unittest4 test_config)
