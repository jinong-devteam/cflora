/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file cf_config.c
 * \brief 설정파일 관련 공통라이브러리 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <cf_base.h>
#include <cf_config.h>

#define _CF_BUF_LEN    256

cf_ini_t *
cf_read_ini (char *conffile) {
    FILE *fp = fopen (conffile, "r");
    cf_ini_t *pheader, *pini;
    char sect[_CF_INI_SECTION_LEN];
    char line[_CF_BUF_LEN];
    char *pc, *sc;

    if (fp == NULL) {
        cf_errormsg ("config file open failed");
        return NULL;
    }

    pheader = NULL;
    while (fgets (line, _CF_BUF_LEN, fp) != NULL) {
        if (line[0] == '[') { //section
            for (sc = sect, pc = line + 1; *pc != '\0'; pc++) {
                if (*pc == ']') {
                    *sc = '\0';
                    break;
                }
                *sc = *pc;
                sc++;
            }
        } else if (isalpha(line[0])) {    // not empty
            pc = strtok (line, "=");
            sc = strtok (NULL, "\n");
            pini = (cf_ini_t *) CF_MALLOC (sizeof(cf_ini_t));
            if (pini == NULL) {
                cf_release_ini (pheader);
                pheader = NULL;
                break;
            }
            strcpy (pini->section, sect);
            strcpy (pini->name, pc);
            strcpy (pini->value, sc);
            cf_errormsg ("configuration [%s:%s]=[%s]", sect, pc, sc);
            pini->next = (void *)pheader;
            pheader = pini;
        }
    }
    fclose (fp);

    return pheader;
}

char *
cf_get_configitem (cf_ini_t *pini, const char *section, const char *name) {
    while (pini != NULL) {
        if (strcmp (pini->section, section) == 0
            && strcmp (pini->name, name) == 0) {
                return pini->value;
        }
        pini = (cf_ini_t *)(pini->next);
    }
    cf_errormsg ("no configuration item - %s:%s", section, name);
    return NULL;
}

int
cf_get_configitem_int (cf_ini_t *pini, const char *section, const char *name) {
    char *item = cf_get_configitem (pini, section, name);
    if (item == NULL) {
        cf_errormsg ("no configuration item to convert to integer value");
        return 0;
    }
    return atoi (item);
}

double
cf_get_configitem_double (cf_ini_t *pini, const char *section, const char *name) {
    char *item = cf_get_configitem (pini, section, name);
    if (item == NULL) {
        cf_errormsg ("no configuration item to convert to integer value");
        return 0;
    }
    return atof (item);
}

void
cf_release_ini (cf_ini_t *pini) {
    cf_ini_t *ptmp;

    while (pini != NULL) {
        ptmp = (cf_ini_t *)(pini->next);
        CF_FREE (pini);
        pini = ptmp;
    }
}
