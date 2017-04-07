
#include <stdio.h>
#include <stdlib.h>

#include <cf_base.h>
#include <cf_db.h>

#include "test.h"

#define TESTDB	"cf_test.db"

cf_ret_t
test_opennclose () {
	cf_db_t db;

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");
	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

	return CF_OK;
}

cf_ret_t
test_execute () {
	cf_db_t db;
	char *errmsg;

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	ASSERT (CF_OK == cf_db_exec (&db, "create table temp (id integer, value double)", NULL, 0, &errmsg), errmsg);
	ASSERT (CF_OK == cf_db_exec (&db, "insert into temp (id, value) values (1, 2.0)", NULL, 0, &errmsg), errmsg);
	ASSERT (CF_OK == cf_db_exec (&db, "insert into temp (id, value) values (3, 4.0)", NULL, 0, &errmsg), errmsg);

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

	return CF_OK;
}

cf_ret_t
test_read () {
	cf_db_t db;
	int rows, columns, i, j;   
	char *errmsg; 
	char **results;

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	ASSERT (CF_OK == cf_db_get_table (&db, "select id, value from temp", &results, &rows, &columns, &errmsg), errmsg);
	for (i = 1; i <= rows; i++) {
		for (j = 0; j < columns; j++) {
			ASSERT ((i * rows + j + 1 - rows) == atof (results[i*rows + j]), "values are not matched");
		}
	}
	cf_db_free_table (results);	

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

	return CF_OK;
}

int 
main () {
	remove (TESTDB);
	TEST (test_opennclose ());
	TEST (test_execute());
	TEST (test_read());
	return 0;
}
