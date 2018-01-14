/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gcg_config.cpp
 * \brief GCG 설정관련 코드. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cf.h>

#include "gcg_base.h"
#include "gcg_config.h"

ret_t
gcg_default_config (gcg_config_t *pconfig) {
	pconfig->gosport = GCG_DEFAULT_GOS_PORT;
	pconfig->gcgport = GCG_DEFAULT_GCG_PORT;

	pconfig->gosid = GCG_DEFAULT_GOSID;
	pconfig->gcgid = GCG_DEFAULT_GCGID;

	strcpy (pconfig->gosip, GCG_DEFAULT_GOS_IP);
	strcpy (pconfig->gcgip, GCG_DEFAULT_GCG_IP);

	pconfig->timer = GCG_DEFAULT_TIMER;

	return OK;
}

ret_t
gcg_read_config (gcg_config_t *pconfig, char *conffile) {
	cf_ini_t *pini;
	char section[_GCG_BUF_LEN];

	if (conffile == NULL) {
		return gcg_default_config (pconfig);
	}

	sprintf (section, "%s-%d", CF_INI_GCG_SECTION, pconfig->gcgid);

	pini = cf_read_ini (conffile);
	ERR_RETURN (pini == NULL, "failed to read configuration file.");

	pconfig->gosport = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_PORT);
	pconfig->gcgport = cf_get_configitem_int (pini, section, CF_INI_GCG_PORT);
	pconfig->gosid = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_ID);
	//pconfig->gcgid = cf_get_configitem_int (pini, CF_INI_GCG_SECTION, CF_INI_GCG_ID);

	//pconfig->snodecnt = cf_get_configitem_int (pini, section, CF_INI_GCG_SNODECOUNT);
	//pconfig->anodecnt = cf_get_configitem_int (pini, section, CF_INI_GCG_ANODECOUNT);

	strcpy (pconfig->gosip, cf_get_configitem (pini, CF_INI_GOS_SECTION, CF_INI_GOS_IP));
	strcpy (pconfig->gcgip, cf_get_configitem (pini, section, CF_INI_GCG_IP));

	pconfig->timer = cf_get_configitem_int (pini, section, CF_INI_GCG_TIMER);

	return OK;
}

void
gcg_release_config (gcg_config_t *pconfig) {
	;
}
