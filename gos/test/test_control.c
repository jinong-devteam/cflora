#include <stdio.h>
#include <stdlib.h>

#include <uv.h>

#include <tp3.h>
#include <cf.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_device.h"
#include "gos_connection.h"
#include "gos_process.h"
#include "gos_rule.h"
#include "gos_control.h"
#include "gos_server.h"

#include "test.h"

#define CONFIG	"../../conf/gos_server.ini"

cf_ret_t
test_readnsend () {
	return CF_OK;
}

cf_ret_t
test_rule () {
	/*
	gos_ruleset_t *prset = gos_get_ruleset ();
	gos_config_t *pconfig = gos_get_config ();

	gos_test_rules (prset, , nvalue);
	gos_evaluate_rules (prset, pconfig);
	gos_reset_rulecondition (prset);

	gos_test_rules (prset, , nvalue);
	gos_evaluate_rules (prset, pconfig);
	gos_reset_rulecondition (prset);

	gos_test_rules (prset, , nvalue);
	gos_evaluate_rules (prset, pconfig);
	gos_reset_rulecondition (prset);

	gos_test_rules (prset, , nvalue);
	gos_evaluate_rules (prset, pconfig);
	gos_reset_rulecondition (prset);
	*/

	return CF_OK;
}


int
main () {
	gos_initialize (CONFIG);
	TEST (test_readnsend ());
	gos_finalize ();
	return 0;
}
