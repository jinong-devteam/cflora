#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cf_base.h>
#include <cf_config.h>

#include "test.h"

#define TESTINI	"cflora_server.ini"

cf_ret_t
test_ini () {
	char *item;
	int n;

	cf_ini_t *pini = cf_read_ini (TESTINI);
	ASSERT (pini != NULL, "to read config file failed");

	item = cf_get_configitem (pini, "GOS", "IP");
	ASSERT (strcmp(item, "127.0.0.1") == 0, "IP is not matched");

	item = cf_get_configitem (pini, "GCG", "PORT");
	ASSERT (strcmp(item, "5000") == 0, "Port is not matched");

	n = cf_get_configitem_int (pini, "GCG", "ID");
	ASSERT (n == 1002, "GCG ID is not matched");

	cf_release_ini (pini);

	return CF_OK;
}

int
main () {
	TEST (test_ini ());

	return 0;
}
