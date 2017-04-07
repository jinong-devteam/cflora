#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gos_base.h"
#include "gos_config.h"

cf_ret_t
gos_default_config (gos_config_t *pconfig) {
	pconfig->port = GOS_DEFAULT_PORT;
	pconfig->timer = GOS_DEFAULT_TIMER;
	pconfig->nwrite = GOS_DEFAULT_NWRITE;
	pconfig->nrule = GOS_DEFAULT_NRULE;

	pconfig->gosid = GOS_DEFAULT_GOSID;
	pconfig->numofgcg = GOS_DEFAULT_NUMOFGCG;

	pconfig->gcgids = (int *)CF_MALLOC(sizeof(int) * GOS_DEFAULT_NUMOFGCG);
	CF_ERR_RETURN (pconfig->gcgids == NULL, "configuration memory allocation failed.");
	(pconfig->gcgids)[0] = GOS_DEFAULT_GCGID;

	if (CF_OK != cf_init_db (&(pconfig->db), GOS_DEFAULT_DB)) {
		CF_FREE (pconfig->gcgids);
		CF_ERR_LOG ("configuration memory allocation failed.");
		return CF_ERR;
	}

	return CF_OK;
}

cf_ret_t
gos_read_config (gos_config_t *pconfig, char *conffile) {
	cf_ini_t *pini;
	char *item;

	if (conffile == NULL) {
		return gos_default_config (pconfig);
	}

	pini = cf_read_ini (conffile);
	CF_ERR_RETURN (pini == NULL, "failed to read configuration file.");
	
	item = cf_get_configitem (pini, CF_INI_DB_SECTION, CF_INI_DB_LOCATION);
	if (CF_OK != cf_init_db (&(pconfig->db), item)) {
		CF_ERR_LOG ("configuration memory allocation failed.");
		return CF_ERR;
	}

	pconfig->port = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_PORT);
	pconfig->gosid = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_ID);

	pconfig->numofgcg = GOS_DEFAULT_NUMOFGCG;
	pconfig->gcgids = (int *)CF_MALLOC(sizeof(int) * GOS_DEFAULT_NUMOFGCG);
	if (pconfig->gcgids == NULL) {
		cf_release_db (&(pconfig->db));
		CF_ERR_LOG ("configuration memory allocation failed.");
		return CF_ERR;
	}

	(pconfig->gcgids)[0] = cf_get_configitem_int (pini, CF_INI_GCG_SECTION, CF_INI_GCG_ID);
	
	pconfig->timer = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_TIMER);
	pconfig->nwrite = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_WRITE);
	pconfig->nrule = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_RULE);

	return CF_OK;
}

void
gos_release_config (gos_config_t *pconfig) {
	CF_FREE (pconfig->gcgids);
	cf_release_db (&(pconfig->db));
}
