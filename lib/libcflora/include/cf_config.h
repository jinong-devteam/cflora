#ifndef _CFLORA_CONFIG_H_
#define _CFLORA_CONFIG_H_


#define CF_INI_GOS_SECTION	"GOS"
#define CF_INI_GOS_IP 		"IP"
#define CF_INI_GOS_PORT		"PORT"
#define CF_INI_GOS_ID		"ID"
#define CF_INI_GOS_TIMER	"TIMER"
#define CF_INI_GOS_WRITE	"WRITE"
#define CF_INI_GOS_RULE		"RULE"

#define CF_INI_GCG_SECTION	"GCG"
#define CF_INI_GCG_IP 		"IP"
#define CF_INI_GCG_PORT		"PORT"
#define CF_INI_GCG_ID		"ID"
#define CF_INI_GCG_TIMER	"TIMER"
#define CF_INI_GCG_SNODECOUNT	"SNODECOUNT"
#define CF_INI_GCG_ANODECOUNT	"ANODECOUNT"

#define CF_INI_DB_SECTION	"DATABASE"
#define CF_INI_DB_LOCATION	"LOCATION"


#define _CF_INI_SECTION_LEN	50
#define _CF_INI_NAME_LEN	50
#define _CF_INI_VALUE_LEN	50

typedef struct {
	char section[_CF_INI_SECTION_LEN];
	char name[_CF_INI_NAME_LEN];
	char value[_CF_INI_VALUE_LEN];
	void *next;
} cf_ini_t;

cf_ini_t *
cf_read_ini (char *conffile);

char *
cf_get_configitem (cf_ini_t *pini, const char *section, const char *name);

int
cf_get_configitem_int (cf_ini_t *pini, const char *section, const char *name);

void
cf_release_ini (cf_ini_t *pini);

#endif
