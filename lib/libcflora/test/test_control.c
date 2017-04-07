
#include <stdio.h>
#include <stdlib.h>

#include <cf_base.h>
#include <cf_db.h>
#include <cf_control.h>

#include "test.h"

#define TESTDB	"cf_test_control.db"

cf_ret_t
test_initcontrol () {
	cf_db_t db;
	char *errmsg;

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	ASSERT (CF_OK == cf_db_exec (&db, "create table gos_environment (obstime datetime not null, device_id integer not null, svalue text, nvalue double);", NULL, 0, &errmsg), errmsg);

	ASSERT (CF_OK == cf_db_exec (&db, "insert into gos_environment (obstime, device_id, nvalue) values (datetime('now', 'localtime', '-300 seconds'), 1, 20.0)", NULL, 0, &errmsg), errmsg);
	ASSERT (CF_OK == cf_db_exec (&db, "insert into gos_environment (obstime, device_id, nvalue) values (datetime('now', 'localtime', '-100 seconds'), 1, 2.0)", NULL, 0, &errmsg), errmsg);
	ASSERT (CF_OK == cf_db_exec (&db, "insert into gos_environment (obstime, device_id, nvalue) values (datetime('now', 'localtime', '-100 seconds'), 2, 100.0)", NULL, 0, &errmsg), errmsg);
	ASSERT (CF_OK == cf_db_exec (&db, "insert into gos_environment (obstime, device_id, nvalue) values (datetime('now', 'localtime'), 2, 50.0)", NULL, 0, &errmsg), errmsg);

	ASSERT (CF_OK == cf_db_exec (&db, "create table gos_control (id integer not null primary key, exectime datetime not null, device_id integer not null, argument integer, ctrltype text not null, rule integer, updatetime datetime not null);", NULL, 0, &errmsg), errmsg);

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

	return CF_OK;
}

cf_ret_t
test_getenv () {
	cf_db_t db;
	int sid[2]  =  {1, 2};
	int rsid[2];
	double value[2];

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	ASSERT_OK (cf_get_environment (&db, 1, value, 500), "fail to get env");
	ASSERT (value[0] == 2.0, "value is not 2.0");

	ASSERT (1 == cf_get_environments (&db, 2, sid, rsid, value, 50), "fail to get envs");
	ASSERT (rsid[0] == 2, "id is not 2");
	ASSERT (rsid[1] == -1, "id is not -1");
	ASSERT (value[0] == 50.0, "value is not 50.0");

	ASSERT (2 == cf_get_environments (&db, 2, sid, rsid, value, 200), "fail to get envs");
	ASSERT (value[1] == 75.0, "value is not 75.0");

	ASSERT_OK (cf_db_close (&db), "fail to close");
	cf_release_db (&db);

	return CF_OK;
}

cf_ret_t
test_execcmd () {
	cf_db_t db;
	int aid[2] = {1, 2};
	int arg[2] = {1, -1};
	int wtime[2] = {10, 20};

	ASSERT_OK (cf_init_db (&db, TESTDB), "fail to init");
	ASSERT_OK (cf_db_open (&db), "fail to open");

	ASSERT_OK (cf_execute_command (&db, 1, 1, 50), "fail to execute command");
	ASSERT_OK (cf_execute_commands (&db, 2, aid, arg, wtime), "fail to execute commands");

	ASSERT_OK (cf_db_close (&db), "fail to close");
	return CF_OK;
}


int
main () {
	remove (TESTDB);
	TEST (test_initcontrol ());
	TEST (test_getenv());
	TEST (test_execcmd ());

	return 0;
}
