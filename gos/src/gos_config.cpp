/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_config.cpp
 * \brief GOS 설정관련 소스 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_time.h"

ret_t
gos_default_config (gos_config_t *pconfig) {
    pconfig->port = GOS_DEFAULT_PORT;
    pconfig->timer = GOS_DEFAULT_TIMER;
    pconfig->nwrite = GOS_DEFAULT_NWRITE;
    pconfig->nupdate = GOS_DEFAULT_NUPDATE;
    pconfig->nrule = GOS_DEFAULT_NRULE;

    pconfig->gosid = GOS_DEFAULT_GOSID;
    pconfig->numofgcg = GOS_DEFAULT_NUMOFGCG;

    pconfig->gcgids = (int *)CF_MALLOC(sizeof(int) * GOS_DEFAULT_NUMOFGCG);
    if (pconfig->gcgids == NULL) {
        LOG(ERROR) << "configuration memory allocation failed.";
        return ERR;
    }

    (pconfig->gcgids)[0] = GOS_DEFAULT_GCGID;

    if (OK != cf_init_db (&(pconfig->db), GOS_DEFAULT_DB)) {
        CF_FREE (pconfig->gcgids);
        LOG(ERROR) << "configuration memory allocation failed.";
        return ERR;
    }

    return OK;
}

ret_t
gos_read_config (gos_config_t *pconfig, char *conffile) {
    cf_ini_t *pini;
    char *item;
    int i;
    char tbuf[50];

    if (conffile == NULL) {
        return gos_default_config (pconfig);
    }

    pini = cf_read_ini (conffile);
    if (pini == NULL) {
        LOG(ERROR) << "failed to read configuration file.";
        return ERR;
    }
    
    item = cf_get_configitem (pini, CF_INI_DB_SECTION, CF_INI_DB_LOCATION);
    if (OK != cf_init_db (&(pconfig->db), item)) {
        LOG(ERROR) << "configuration memory allocation failed.";
        return ERR;
    }

    pconfig->port = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_PORT);
    pconfig->gosid = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_ID);

    pconfig->numofgcg = cf_get_configitem_int (pini, 
                                        CF_INI_GOS_SECTION, CF_INI_GOS_GCGNUM);
    pconfig->gcgids = (int *)CF_MALLOC(sizeof(int) * (pconfig->numofgcg));
    if (pconfig->gcgids == NULL) {
        cf_release_db (&(pconfig->db));
        LOG(ERROR) << "configuration memory allocation failed.";
        return ERR;
    }

    for (i = 0; i < pconfig->numofgcg; i++) {
        sprintf (tbuf, "%s-%d", CF_INI_GCG_SECTION, i + 1);
        (pconfig->gcgids)[i] = cf_get_configitem_int (pini, tbuf, CF_INI_GCG_ID);
    }
    
    pconfig->timer = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_TIMER);
    pconfig->nwrite = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_WRITE);
    pconfig->nupdate = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_UPDATE);
    pconfig->nrule = cf_get_configitem_int (pini, CF_INI_GOS_SECTION, CF_INI_GOS_RULE);
    pconfig->longitude = cf_get_configitem_double (pini, CF_INI_GOS_SECTION, CF_INI_GOS_LONGITUDE);
    pconfig->latitude = cf_get_configitem_double (pini, CF_INI_GOS_SECTION, CF_INI_GOS_LATITUDE);

    gos_inittime (pconfig->longitude, pconfig->latitude);

    return OK;
}

void
gos_release_config (gos_config_t *pconfig) {
    CF_FREE (pconfig->gcgids);
    cf_release_db (&(pconfig->db));
}
