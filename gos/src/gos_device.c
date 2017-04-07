#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <uv.h>

#include <cf.h>
#include <tp3.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_control.h"
#include "gos_connection.h"
#include "gos_device.h"
#include "gos_rule.h"
#include "gos_server.h"
#include "gos_vsensor.h"

#ifdef _MSC_VER
#define strtok_r	strtok_s
#endif

static char *_str_status[_GOS_DEVSTAT_MAX] = {
	"activated",
	"installed",
	"detected",
	"suspended",
	"abnormal",
	"disconnected"
};

static char *_str_type[_GOS_DEVTYPE_MAX] = {
	"gos",
	"gcg",
	"snode",
	"anode",
	"sensor",
	"actuator",
	"cctv",
	"vsensor",
	"asensor",
	"unknown"
};

/*
static char *_str_ttaid[_GOS_DEVTYPE_MAX] = {
	"GOS_ID",
	"GCG_ID",
	"SNODE_ID",
	"ANODE_ID",
	"SGID",
	"AGID",
	"CCTV_ID",
	"VSENSOR_ID",
	"ASENSOR_ID",
	"UNKNOWN_ID"
};
*/

static char *_str_convert[_GOS_CVTTYPE_MAX] = {
	"none",
	"linear",
	"tablemap",
	"ratio",
	"oratio",
	"vsensor"
};

static char *_str_asensor[_GOS_ASENSOR_MAX] = {
	"limiter",
	"anglesensor"
};

int
gos_get_actuator_argument (int arg, int worktime) {
	return arg << _GOS_ACTARG_BIT | worktime;
}

void
gos_parse_actuator_argument (int rawvalue, int *arg, int *worktime) {
	*arg = (rawvalue & _GOS_ACTARG_ARGFILTER) >> _GOS_ACTARG_BIT;
	*worktime = rawvalue & _GOS_ACTARG_TIMEFILTER;
}

gos_devtype_t
gos_get_devicetype_from_string (char *str) {
	if (*str == 's') {
		if (strcmp (str, _str_type[GOS_DEV_SNODE]) == 0)
			return GOS_DEV_SNODE;
		if (strcmp (str, _str_type[GOS_DEV_SENSOR]) == 0)
			return GOS_DEV_SENSOR;

	} else if (*str == 'a') {
		if (strcmp (str, _str_type[GOS_DEV_ANODE]) == 0)
			return GOS_DEV_ANODE;
		if (strcmp (str, _str_type[GOS_DEV_ACTUATOR]) == 0)
			return GOS_DEV_ACTUATOR;
		if (strcmp (str, _str_type[GOS_DEV_ASENSOR]) == 0)
			return GOS_DEV_ASENSOR;

	} else if (*str == 'g') {
		if (strcmp (str, _str_type[GOS_DEV_GCG]) == 0)
			return GOS_DEV_GCG;
		if (strcmp (str, _str_type[GOS_DEV_GOS]) == 0)
			return GOS_DEV_GOS;

	} else {
		if (strcmp (str, _str_type[GOS_DEV_CCTV]) == 0)
			return GOS_DEV_CCTV;
		if (strcmp (str, _str_type[GOS_DEV_VSENSOR]) == 0)
			return GOS_DEV_VSENSOR;
	}

	return GOS_DEV_UNKNOWN;
}
	
gos_devstat_t
gos_get_devicestatus_from_string (char *str) {
	if (*str == 'a') {
		if (strcmp (str, _str_status[GOS_DEVST_ACTIVATED]) == 0)
			return GOS_DEVST_ACTIVATED;
	} else if (*str == 'd') {
		if (strcmp (str, _str_status[GOS_DEVST_DETECTED]) == 0)
			return GOS_DEVST_DETECTED;
		else if (strcmp (str, _str_status[GOS_DEVST_DISCONNECTED]) == 0) 
			return GOS_DEVST_DISCONNECTED;
	} else {
		if (strcmp (str, _str_status[GOS_DEVST_SUSPENDED]) == 0)
			return GOS_DEVST_SUSPENDED;
	}
	return GOS_DEVST_ABNORMAL;
}

gos_asen_t
gos_get_asensor_from_string (char *str) {
	if (strcmp (str, _str_asensor[GOS_ASENSOR_LIMITER]) == 0)
		return GOS_ASENSOR_LIMITER;
	return GOS_ASENSOR_ANGLE;
}

cf_ret_t
gos_load_devinfo (gos_devinfo_t *pdevinfo, cf_db_t *db) {
	char *query = "select id, devtype, status, gcg_id, node_id, dev_id, field_id, name from gos_devices as A left join gos_devicemap as B on A.id = B.device_id ";
	char **result;
	char *errmsg;
	int rows, columns, rc, i;
	gos_dev_t *pdev;


	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (get devices) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	pdevinfo->pdev = (gos_dev_t *) CF_MALLOC (sizeof (gos_dev_t) * rows);
	if (pdevinfo->pdev == NULL) {
		CF_ERR_LOG ("memory allocation for device infomation failed.");
		cf_db_free_table (result);
	}
	pdevinfo->size = rows;
	//pdevinfo->needsync = FALSE;

	for (i = 1; i <= rows; i++) {
		pdev = pdevinfo->pdev + i - 1;
		pdev->id = atoi (result[i * columns]);

		//pdev->type = atoi (result[i * columns + 1]);
		//pdev->status = atoi (result[i * columns + 2]);
		pdev->type = gos_get_devicetype_from_string (result[i * columns + 1]);
		pdev->status = gos_get_devicestatus_from_string (result[i * columns + 2]);
		pdev->ischanged = 0;
		pdev->isupdated = 0;

		if (result[i * columns + 3] == NULL) {
			pdev->gcgid = pdev->nodeid = pdev->deviceid = -1;
		} else {
			pdev->gcgid = atoi (result[i * columns + 3]);
			if (result[i * columns + 4] == NULL) {
				pdev->nodeid = pdev->deviceid = -1;
			} else {
				pdev->nodeid = atoi (result[i * columns + 4]);
				if (result[i * columns + 5] == NULL) {
					pdev->deviceid = -1;
				} else {
					pdev->deviceid = atoi (result[i * columns + 5]);
				}
			}
		}
		if (result[i * columns + 6] == NULL) { // field_id <== gos_devicemap by chj 2015/10/20
			pdev->field_id = -1 ;
		}
		else {
			pdev->field_id = atoi (result[i * columns + 6]);
		}
		if (result[i * columns + 7] == NULL) { // sensor ui name by chj 2015/10/20
			pdev->type_name[0] = '\0' ;
		}
		else {
			strncpy(pdev->type_name,result[i * columns + 7],sizeof(pdev->type_name)-1 ) ;
		}

		if (pdev->type == GOS_DEV_SENSOR || pdev->type == GOS_DEV_VSENSOR) {
			if (CF_ERR == gos_load_convertinfo (pdev, db)) {
				CF_ERR_LOG ("a sensor (%d) does not have a converting method.", pdev->id);
			}
		} else if (pdev->type == GOS_DEV_ACTUATOR) {
			if (CF_ERR == gos_load_driver (pdev, db)) {
				CF_ERR_LOG ("an actuator (%d) does not have an actuator sensor.", pdev->id);
			}

		}
	}

	cf_db_free_table (result);
	return CF_OK;
}

cf_ret_t
gos_init_devinfo (gos_devinfo_t *pdevinfo, gos_config_t *pconfig) {
	cf_db_t *db = &(pconfig->db);
	int rc;

	rc = gos_load_devinfo (pdevinfo, db);

	return rc;
}

int
gos_is_newdevice (gos_devinfo_t *pdevinfo, gos_dev_t *pdev) {
	int i;
	gos_dev_t *pd;

	for (i = 0; i < pdevinfo->size; i++) {
		pd = pdevinfo->pdev + i;
		if (pd->gcgid == pdev->gcgid && pd->nodeid == pdev->nodeid && pd->deviceid == pdev->deviceid)
			return FALSE;
	}
	return TRUE;
}

/*
cf_ret_t
gos_add_devices (gos_devinfo_t *pdevinfo, gos_config_t *pconfig, int size, gos_dev_t *pdev) {
	cf_db_t *db = &(pconfig->db);
	int rc;
	cf_db_stmt *stmt;
	char *tail;
	char *query = "INSERT INTO gos_devices (type, status, gcg_id, node_id, g_id) VALUES (?, ?, ?, ?, ?)";

	CF_ERR_RETURN (cf_db_open (pconfig->db, &db), "database open failed.");

	cf_db_busy_handler (db, cf_db_busy, "device information initialzation");
	cf_db_busy_timeout (db, 200);

	CF_BEGIN_TRANSACTION(db);

	rc = cf_db_prepare_v2 (db, query, strlen(query), &stmt, &tail);
	if (rc != CF_OK) {
		CF_ERR_LOG ("sql query preparation failed.");
		CF_ERR_LOG (cf_db_errmsg (db));
		cf_db_close (db);
		return CF_ERR;
	}

	for (i = 0; i < size; i++) {
		cf_db_bind_text (stmt, 1, _str_type[pdev[i].type], strlen(_str_type[pdev[i].type]), NULL);
		cf_db_bind_text (stmt, 2, _str_status[pdev[i].status], strlen(_str_status[pdev[i].status]), NULL);
		//cf_db_bind_int (stmt, 1, pdev[i].type);
		//cf_db_bind_int (stmt, 2, pdev[i].status);
		if (pdev[i].gcgid < 0) {
			cf_db_bind_null (stmt, 3);
			cf_db_bind_null (stmt, 4);
			cf_db_bind_null (stmt, 5);
		} else {
			cf_db_bind_int (stmt, 3, pdev[i].gcgid);
			if (pdev[i].nodeid < 0) {
				cf_db_bind_null (stmt, 4);
				cf_db_bind_null (stmt, 5);
			} else {
				cf_db_bind_int (stmt, 4, pdev[i].nodeid);
				if(pdev[i].deviceid < 0) {
					cf_db_bind_null (stmt, 5);
				} else {
					cf_db_bind_int (stmt, 5, pdev[i].deviceid);
				}
			}
		}
		cf_db_step (stmt);
		cf_db_reset (stmt);
	}
	cf_db_finalize (stmt);
	CF_END_TRANSACTION(db);

	gos_release_devinfo (pdevinfo);
	rc = gos_load_devinfo (pdevinfo, db);

	cf_db_close (db);

	return rc;
}
*/

void
gos_release_cvt_arg (gos_cvt_linear_arg_t *parg) {
	CF_FREE (parg->pseg);
}

void
gos_release_devinfo (gos_devinfo_t *pdevinfo) {
	int i; 
	gos_dev_t *pdev;
	for (i = 0; i < pdevinfo->size ; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->cvt == GOS_CVT_VSENSOR) { 
			gos_cvt_vsensor_arg_t *parg = (gos_cvt_vsensor_arg_t *)pdev->cvtarg;
			gos_release_vsensor_arg (parg);
			CF_FREE (parg);
		} else if (pdev->cvt != GOS_CVT_NONE) {
			gos_cvt_linear_arg_t *parg = (gos_cvt_linear_arg_t *)pdev->cvtarg;
			gos_release_cvt_arg (parg);
			CF_FREE (parg);
		}
	}
	CF_FREE (pdevinfo->pdev);
}

gos_dev_t *
gos_find_device (gos_devinfo_t *pdevinfo, int deviceid) {
	int i;
	gos_dev_t *pdev;

	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->id == deviceid) {
			return pdev;
		}
	}
	return NULL;
}

gos_dev_t *
gos_find_device_by_tta (gos_devinfo_t *pdevinfo, int gcgid, int nodeid, int deviceid) {
	int i;
	gos_dev_t *pdev;

	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->gcgid == gcgid && pdev->nodeid == nodeid && pdev->deviceid == deviceid) {
			return pdev;
		}
	}
	return NULL;
}

gos_dev_t *
gos_add_device (gos_devinfo_t *pdevinfo, gos_dev_t *pdev) {
	gos_dev_t *ptemp;

	ptemp = (gos_dev_t *) CF_REALLOC (pdevinfo->pdev, sizeof (gos_dev_t) * (pdevinfo->size + 1));
	if (ptemp == NULL) {
		CF_ERR_LOG ("memory allocation for device infomation failed.");
		return NULL;
	}

	pdev->ischanged = 0;
	pdev->isupdated = 0;
	pdev->nvalue = 0;
	pdev->rawvalue = 0;
	pdev->id = _GOS_NO_ID;
	pdev->status = GOS_DEVST_DETECTED;

	pdevinfo->pdev = ptemp;
	memcpy (pdevinfo->pdev + pdevinfo->size, pdev, sizeof(gos_dev_t));
	pdevinfo->size += 1;
	//pdevinfo->needsync = TRUE;
	return pdevinfo->pdev + pdevinfo->size - 1;
}

void
gos_set_device (gos_dev_t *pdev, gos_devtype_t type, int gcgid, int nodeid, int deviceid) {
	pdev->type = type;
	pdev->gcgid = gcgid;
	pdev->nodeid = nodeid;
	pdev->deviceid = deviceid;
}

gos_dev_t *
gos_get_device (gos_devinfo_t *pdevinfo, gos_devtype_t devtype, int gcgid, int nodeid, int deviceid) {
	gos_dev_t dev;
	gos_dev_t *pdev = gos_find_device_by_tta (pdevinfo, gcgid, nodeid, deviceid);
	
	if (pdev == NULL) {
		gos_set_device (&dev, devtype, gcgid, nodeid, deviceid);
		return gos_add_device (pdevinfo, &dev);
	}

	return pdev;
}

int
gos_get_last_deviceid (cf_db_t *db) {
	char query[200];
	char **results;
	char *errmsg;
	int rowid, rows, columns, rc, i;

	rowid = cf_db_last_insert_rowid (db);
	sprintf (query, "select id from gos_devices where rowid = %d", rowid);
	rc = cf_db_get_table (db, query, &results, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("getting device id failed. %s", errmsg);
		cf_db_free (errmsg);
		return _GOS_NO_ID;
	}
	i = atoi (results[1]);
	cf_db_free_table (results);
	return i;
}
	
cf_ret_t
gos_sync_devices (gos_devinfo_t *pdevinfo, gos_config_t *pconfig) {
	cf_db_t *db = &(pconfig->db);
	int i, rc;
	gos_dev_t *pdev;
	cf_db_stmt *stmt;
	const char *tail;
	char *query = "INSERT INTO gos_devices (type, status, gcg_id, node_id, g_id) VALUES (?, ?, ?, ?, ?)";

	CF_ERR_RETURN (cf_db_open (db), "database open failed.");

#ifdef USE_SQLITE3
	cf_db_busy_handler (db, cf_db_busy, "device information initialzation");
	cf_db_busy_timeout (db, BUSY_TIMEOUT);
	CF_EXECUTE (db, "PRAGMA journal_mode = WAL");
#endif

	CF_BEGIN_TRANSACTION(db);

	rc = cf_db_prepare_v2 (db, query, strlen(query), &stmt, &tail);
	if (rc != CF_OK) {
		CF_ERR_LOG ("sql query preparation failed. %s", cf_db_errmsg (db));
		cf_db_close (db);
		return CF_ERR;
	}

	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->id != _GOS_NO_ID) {
			continue;
		}
		cf_db_bind_text (stmt, 1, _str_type[pdev[i].type], strlen(_str_type[pdev[i].type]), NULL);
		cf_db_bind_text (stmt, 2, _str_status[pdev[i].status], strlen(_str_status[pdev[i].status]), NULL);
		//cf_db_bind_int (stmt, 1, pdev[i].type);
		//cf_db_bind_int (stmt, 2, pdev[i].status);
		if (pdev[i].gcgid < 0) {
			cf_db_bind_null (stmt, 3);
			cf_db_bind_null (stmt, 4);
			cf_db_bind_null (stmt, 5);
		} else {
			cf_db_bind_int (stmt, 3, pdev[i].gcgid);
			if (pdev[i].nodeid < 0) {
				cf_db_bind_null (stmt, 4);
				cf_db_bind_null (stmt, 5);
			} else {
				cf_db_bind_int (stmt, 4, pdev[i].nodeid);
				if(pdev[i].deviceid < 0) {
					cf_db_bind_null (stmt, 5);
				} else {
					cf_db_bind_int (stmt, 5, pdev[i].deviceid);
				}
			}
		}
		rc = cf_db_step (stmt);
		if (rc != CF_OK) {
			CF_ERR_LOG ("sql query execution failed. %s", cf_db_errmsg (db));
			CF_ROLLBACK (db);
			cf_db_close (db);

			CF_ERR_LOG ("relase device information & refresh it.");
			gos_release_devinfo (pdevinfo);
			rc = gos_load_devinfo (pdevinfo, db);

			return rc;
		}
		pdev->id = gos_get_last_deviceid (db);
		cf_db_reset (stmt);
	}
	cf_db_finalize (stmt);

	CF_END_TRANSACTION(db);

	cf_db_close (db);

	//pdevinfo->needsync = FALSE;

	return CF_OK;
}

cf_ret_t
gos_update_env (gos_dev_t *pdev, double nvalue, int rawvalue) {
	pdev->nvalue = nvalue;
	pdev->rawvalue = rawvalue;

	return CF_OK;
}

cf_ret_t
gos_update_stat (gos_dev_t *pdev, int status) {
	if (pdev == NULL) {
		return CF_ERR;
	}

	if (pdev->type == GOS_DEV_SENSOR 
		|| pdev->type == GOS_DEV_ACTUATOR 
		|| pdev->type == GOS_DEV_ASENSOR) {

		if (pdev->status == GOS_DEVST_SUSPENDED) { 
			// 사용자에 의해서 중지된 상태에서는 모든 상태변환을 무시
			status = GOS_DEVST_SUSPENDED;

		} else if (pdev->status == GOS_DEVST_INSTALLED) { 
			// 설치된 상태에서는 감지된 상태로만 전환가능
			if (status == TP3_DEVST_NORMAL)
				status = GOS_DEVST_DETECTED;

		} else if (pdev->status != GOS_DEVST_DETECTED) { 
			// 중지, 설치, 감지 상태가 아니라면 에러발생시 ABNORMAL 상태가 됨
			if (status == TP3_DEVST_ERROR)
				status = GOS_DEVST_ABNORMAL;
			else 
				status = GOS_DEVST_ACTIVATED;
		}

	} else {
		if (status != TP3_NODEST_NORMAL)
			status = GOS_DEVST_ABNORMAL;
		else
			status = GOS_DEVST_ACTIVATED;
	} 

	pdev->status = status;
	pdev->ischanged = 1;
	return CF_OK;
}

cf_ret_t
gos_update_sensor_env (gos_dev_t *pdev, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char date_now[20] ; // YYYY-MM-DD HH:MM:ss

	cf_db_timestring (date_now, sizeof(date_now), (int)time(NULL));
	sprintf (query, "UPDATE gos_environment_current "
			"set obstime = '%s', nvalue=%f, rawvalue=%d where device_id = %d", 
			date_now, pdev->nvalue, pdev->rawvalue, pdev->id);
	CF_EXECUTE (db, query);

	sprintf (query, "INSERT INTO gos_environment "
			"(obstime, device_id, nvalue, rawvalue) "
			"VALUES ('%s', %d, %f, %d)", 
			date_now, pdev->id, pdev->nvalue, pdev->rawvalue);
	CF_EXECUTE (db, query);

	return CF_OK;
}

cf_ret_t
gos_update_actuator_env (gos_dev_t *pdev, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	int arg, tm;
	char date_now[20] ; // YYYY-MM-DD HH:MM:ss

	cf_db_timestring (date_now, sizeof(date_now), (int)time(NULL));
	if (pdev->type == GOS_DEV_NACTUATOR) {

		sprintf (query, "UPDATE gos_device_status "
			"set updatetime = '%s', rawvalue = %d where device_id = %d", 
			date_now, pdev->rawvalue, pdev->id);
	} else {
		gos_parse_actuator_argument (pdev->rawvalue, &arg, &tm);

		sprintf (query, "UPDATE gos_device_status "
			"set updatetime = '%s', rawvalue = %d, argument = %d, "
			"worktime = %d where device_id = %d", 
			date_now, pdev->rawvalue, pdev->id, arg, tm);
	}
	CF_EXECUTE (db, query);

	return CF_OK;
}

cf_ret_t
gos_vsensor_env (gos_devinfo_t *pdevinfo) {
	int i ;
	gos_dev_t *pdev;

	/* Virtual Sensor Observation Setting & Test Rules */
	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->type == GOS_DEV_VSENSOR) {
			gos_cvt_vsensor_arg_t *parg = (gos_cvt_vsensor_arg_t *)(pdev->cvtarg);
			if (CF_OK == (parg->_get_value) (pdevinfo, parg->config, &(pdev->nvalue))) {
				CF_VERBOSE (CF_VERBOSE_MID, "virtual Sensor [%d] : [%f].", pdev->id, pdev->nvalue);

				pdev->rawvalue = 0;

				gos_test_rules (gos_get_ruleset (), pdev->id, pdev->nvalue);
			}
		}
	}
	return CF_OK;
}

cf_ret_t
gos_write_env (gos_devinfo_t *pdevinfo, cf_db_t *db){
	int i ;
	gos_dev_t *pdev;

	gos_vsensor_env (pdevinfo);

	CF_BEGIN_TRANSACTION(db);
	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->type == GOS_DEV_SENSOR 
			|| pdev->type == GOS_DEV_VSENSOR
			|| pdev->type == GOS_DEV_ASENSOR) {

			gos_update_sensor_env (pdev, db);

		} else if (pdev->type == GOS_DEV_ACTUATOR) {

			gos_update_actuator_env (pdev, db);
		}
	}
	CF_END_TRANSACTION(db);

	return CF_OK;
}

cf_ret_t
gos_vsensor_stat (gos_devinfo_t *pdevinfo) {
	int i;
	gos_dev_t *pdev;

	/* Virtual Sensor Status*/
	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->type == GOS_DEV_VSENSOR && pdev->status != GOS_DEVST_SUSPENDED) {
			gos_cvt_vsensor_arg_t *parg = (gos_cvt_vsensor_arg_t *)(pdev->cvtarg);
			if (CF_OK == (parg->_get_stat) (pdev, pdevinfo, parg->config, &(pdev->status))) {
				pdev->ischanged = 1;
			}
		}
	}

	return CF_OK;
}

cf_ret_t
gos_write_stat (gos_devinfo_t *pdevinfo, cf_db_t *db){
	int i;
	char query[200];
	gos_dev_t *pdev;

	gos_vsensor_stat (pdevinfo);

	CF_BEGIN_TRANSACTION(db);
	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->ischanged == 0)
			continue;

		sprintf (query, "UPDATE gos_devices "
			"set status = '%s' where id = %d;"
			, _str_status[pdev->status], pdev->id);

		CF_EXECUTE (db, query);
		pdev->ischanged = 0;
	}
	CF_END_TRANSACTION(db);
	return CF_OK;
}

cf_ret_t
gos_write_devinfo (gos_devinfo_t *pdevinfo, gos_config_t *pconfig) {
	cf_db_t *db = &(pconfig->db);

	gos_write_stat (pdevinfo, db);
	gos_write_env (pdevinfo, db);

	return CF_OK;
}

typedef double 
(*_gos_convert_func) (gos_cvt_segment_t *pseg, double offset, int value);

double
gos_convert_linear (gos_cvt_segment_t *pseg, double offset, int value) {
	return offset + pseg->intercept + pseg->slope * (value - pseg->from);
}

double
gos_convert_tablemap (gos_cvt_segment_t *pseg, double offset, int value) {
	return offset + pseg->intercept;
}

double
gos_convert_ratio (gos_cvt_segment_t *pseg, double offset, int value) {
	return offset + pseg->slope * value + pseg->intercept;
}

double
gos_convert_oratio (gos_cvt_segment_t *pseg, double offset, int value) {
	if (value == 0)
		return 0;
	return offset + pseg->slope / value + pseg->intercept;
}

static _gos_convert_func _cvt_funcs[_GOS_CVTTYPE_MAX] = {
	NULL,
	gos_convert_linear, 
	gos_convert_tablemap, 
	gos_convert_ratio, 
	gos_convert_oratio, 
	NULL
};

cf_ret_t
gos_convert_env (gos_dev_t *pdev, int raw, double *nvalue) {
	int i;
	gos_cvt_linear_arg_t *parg;
	gos_cvt_segment_t *pseg;

	if (pdev->cvt == GOS_CVT_NONE) {
		*nvalue = 0;
		return CF_OK;
	}

	CF_ASSERT (pdev->cvt != GOS_CVT_VSENSOR, "GOS_CVT_VSENSOR is not for realtime converting");

	parg = (gos_cvt_linear_arg_t *)pdev->cvtarg;
	for (i = 0; i < parg->nseg; i++) {
		pseg = parg->pseg + i;
		if (pseg->from <= raw && raw <= pseg->to) {
			CF_ASSERT (_cvt_funcs[pdev->cvt] != NULL, "no converting functions");
			*nvalue = _cvt_funcs[pdev->cvt] (pseg, parg->offset, raw);
			return CF_OK;
		}
	}
	CF_ERR_LOG ("sensor value converting failed.");
	return CF_ERR;
}

#define _DELIM_KV	':'
#define _DELIM_ROW	","

void
gos_get_linear_cvt_arg_token (char *token, double *raw, double *val) {
	char *ptr;
	for (ptr = token; *ptr != _DELIM_KV; ptr++) {
		CF_ASSERT (*ptr != '\0', 
			"The converting configuration is not well-formed. There is no ':' character.");
	}
	*ptr = '\0';
	ptr++;

	*raw = atof (token);
	*val = atof (ptr);
}

/* 0:0,5:100 */
cf_ret_t
gos_generate_linear_cvt_arg (gos_cvt_linear_arg_t *parg, char *config) {
	int n;
	double from, to, s, e;
	char *ptr, *token;
	gos_cvt_segment_t *pseg;

	CF_VERBOSE (CF_VERBOSE_HIGH, "parse linear converting configure [%s]", config);
	
	for (n = 0, ptr = config; *ptr != '\0'; ptr++) {
		if (*ptr == ',')
			n++;
	}

	pseg = (gos_cvt_segment_t *) CF_MALLOC (sizeof (gos_cvt_segment_t) * n);
	CF_ERR_RETURN (pseg == NULL, "memory allocation for cvt segment failed");

	parg->pseg = pseg;
	parg->nseg = n;

	n = 0;
	token = strtok_r (config, _DELIM_ROW, &ptr);
	CF_ASSERT (token != NULL, "wrong configuration");
	gos_get_linear_cvt_arg_token (token, &from, &s);

	token = strtok_r(ptr, _DELIM_ROW, &ptr);
	CF_ASSERT (token != NULL, "wrong configuration");

	while (token != NULL) {
		gos_get_linear_cvt_arg_token (token, &to, &e);
		pseg[n].from = from;
		pseg[n].to = to;
		pseg[n].slope = (e - s) / (to - from);
		pseg[n].intercept = s;
		CF_VERBOSE (CF_VERBOSE_MID, "conv token [%f][%f][%f][%f]", 
					from, to, (e - s) / (to - from), s);
		from = to;
		s = e;
		token = strtok_r(ptr, _DELIM_ROW, &ptr);
		//CF_ASSERT (token != NULL, "wrong configuration");
		//gos_get_linear_cvt_arg_token (token, &to, &e);
		n++;
	} 

	return CF_OK;
}

/* 0:0,3:10,5:0  0~3은 0, 3~5는 10, 100은 무시*/
cf_ret_t
gos_generate_tablemap_cvt_arg (gos_cvt_linear_arg_t *parg, char *config) {
	int n;
	double from, to, s, e;
	char *ptr, *token;
	gos_cvt_segment_t *pseg;
	
	for (n = 0, ptr = config; *ptr != '\0'; ptr++) {
		if (*ptr == ',')
			n++;
	}

	pseg = (gos_cvt_segment_t *) CF_MALLOC (sizeof (gos_cvt_segment_t) * n);
	CF_ERR_RETURN (pseg == NULL, "memory allocation for cvt segment failed");

	parg->pseg = pseg;
	parg->nseg = n;

	n = 0;
	token = strtok_r (config, _DELIM_ROW, &ptr);
	CF_ASSERT (token != NULL, "wrong configuration");
	gos_get_linear_cvt_arg_token (token, &from, &s);

	token = strtok_r(ptr, _DELIM_ROW, &ptr);
	CF_ASSERT (token != NULL, "wrong configuration");

	while (token != NULL) {
		gos_get_linear_cvt_arg_token (token, &to, &e);
		pseg[n].from = from;
		pseg[n].to = to;
		pseg[n].slope = 0;	// 무시
		pseg[n].intercept = s;
		from = to;
		s = e;
		token = strtok_r(ptr, _DELIM_ROW, &ptr);
		n++;
	} 

	return CF_OK;
}

/* 3:4 - slope:intercept */
cf_ret_t
gos_generate_ratio_cvt_arg (gos_cvt_linear_arg_t *parg, char *config) {
	double s, i;
	gos_cvt_segment_t *pseg;
	
	pseg = (gos_cvt_segment_t *) CF_MALLOC (sizeof (gos_cvt_segment_t));
	CF_ERR_RETURN (pseg == NULL, "memory allocation for cvt segment failed");

	parg->pseg = pseg;
	parg->nseg = 1;

	gos_get_linear_cvt_arg_token (config, &s, &i);

	pseg->from = 0;
	pseg->to = 5000;
	pseg->slope = s;
	pseg->intercept = i;

	return CF_OK;
}

void *
gos_generate_cvt_arg (gos_dev_t *pdev, char *config, double offset) {
	gos_cvt_linear_arg_t *parg;

	parg = (gos_cvt_linear_arg_t *) CF_MALLOC (sizeof (gos_cvt_linear_arg_t));
	CF_EXP_RETURN (parg == NULL, NULL, "memory allocation for linear converting argument failed");

	parg->offset = offset;

	CF_VERBOSE (CF_VERBOSE_MID, "load conv map [%d] [%s]", pdev->id, config);

	switch (pdev->cvt) {
		case GOS_CVT_LINEAR:
			gos_generate_linear_cvt_arg (parg, config);
			break;
		case GOS_CVT_TABLEMAP:
			gos_generate_tablemap_cvt_arg (parg, config);
			break;
		case GOS_CVT_RATIO:
		case GOS_CVT_ORATIO:
			gos_generate_ratio_cvt_arg (parg, config);
			break;
		default:
			CF_ERR_LOG ("GOS_CVT_NONE does not use argument & GOS_CVT_VSENSOR use diffrent argument.");
			return NULL;
	}

	parg->offset = offset;

	return parg;
}

cf_ret_t
gos_load_driver (gos_dev_t *pdev, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char **result;
	char *errmsg;
	int rows, columns, rc, i;

	sprintf(query, "select channel, name from gos_device_portmap where opt = 'asensor' and device_id = %d", pdev->id);

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (get device convertmap) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	CF_ASSERT (_GOS_ASENSOR_MAX >= rows, "max number of actuator sensor is 2");
	((pdev->driver).waitingcmd).stat = GOS_CMD_NONE;
	((pdev->driver).currentcmd).stat = GOS_CMD_NONE;

	for (i = 1; i <= _GOS_ASENSOR_MAX; i++) {
		if (i <= rows) {
			(pdev->driver).asensorid[i-1] = atoi (result[i * columns]);
			(pdev->driver).type[i-1] = gos_get_asensor_from_string (result[i * columns + 1]); 
			(pdev->driver).used_asensor = 1;
		} else {
			(pdev->driver).asensorid[i-1] = _GOS_NOT_ASENSOR;
			(pdev->driver).type[i-1] = GOS_ASENSOR_LIMITER;
		}
	}

	cf_db_free_table (result);

	return CF_OK;
}

cf_ret_t
gos_load_convertinfo (gos_dev_t *pdev, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char **result;
	char *errmsg;
	int rows, columns, rc;
	gos_cvt_t cvt;

	sprintf(query, "select ctype, configuration, offset from gos_device_convertmap where device_id = %d", pdev->id);

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (get device convertmap) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	// 결과 값이 없을 경우
	if ( rows <= 0 ) {
		cf_db_free_table (result);
		return CF_ERR;
	}

	for (cvt = GOS_CVT_NONE; cvt < _GOS_CVTTYPE_MAX; cvt++) {
		if (strcmp (_str_convert[cvt], result[3]) == 0) {
			pdev->cvt = cvt;
			if (cvt == GOS_CVT_VSENSOR) {
				pdev->cvtarg = gos_generate_vsensor_arg (pdev, result[4], atof(result[5]), db);
			} else if (cvt != GOS_CVT_NONE) {
				pdev->cvtarg = gos_generate_cvt_arg (pdev, result[4], atof(result[5]));
			} else {
				pdev->cvtarg = NULL;
			}
		}
	}

	cf_db_free_table (result);

	return CF_OK;
}

/*
static char *cmdstatstr[GOS_CMD_STATMAX] = {
	"no command",
	"ignored because of a wrong command",
	"ignored by a new command",
	"ignored by auto control policy",
	"waiting",
	"sent",
	"working",
	"stopping by actuator sensor",
	"stopping by a stop command",
	"finished normally",
	"finished by actuator sensor",
	"finished by a stop command"
};
*/

cf_ret_t
gos_send_real_command (gos_cmd_t *pcmd, gos_conninfo_t *pconninfo, gos_dev_t *pdev) {
	tp3_msg_t req;
	tp3_frame_t frame;
	cf_ret_t rc;
	int gid;
	gos_conn_t *pconn;

	gid = tp3_getgid (pdev->nodeid, pdev->deviceid);

	pconn = gos_find_connection (pconninfo, pdev->gcgid);
	if (pconn == NULL) {
		CF_VERBOSE (CF_VERBOSE_LOW, "Invalid gcgid [%d]", pdev->gcgid);
		return CF_ERR;
	}

	CF_ERR_RETURN (TP3_SC_NOERROR != 
			tp3_actcontrolmsg (&req, pdev->gcgid, 1, &gid, &(pcmd->ctrlarg)),
			"TTA actuator control message generation failed.");
	CF_ERR_RETURN (TP3_SC_NOERROR != tp3_generateframe (&req, &frame),
			"TTA actuator control message frame generation failed.");

	rc = gos_send_message (pconn->handle, &frame);

	CF_VERBOSE (CF_VERBOSE_LOW, "A command [seq : %d] "
		"(id : %d, deviceid: %d, arg : %d, time : %d, scnt: %d, actval: %d) is sent.", 
		req.header.msgseq, pcmd->id, pcmd->deviceid, 
		pcmd->arg, pcmd->tm, pcmd->workcnt, pcmd->ctrlarg);

	pcmd->senttm = (int)time(NULL);
	pcmd->workcnt += 1;

	tp3_releaseframe (&frame);
	tp3_releasemsg (&req);

	return rc;
}

void
gos_set_command (gos_cmd_t *pcmd, int id, int exectm, int deviceid, int argument, int worktime, char *ctrltype, int ctrlid, int ctrlarg) {
	pcmd->id = id;
	pcmd->exectm = exectm;
	pcmd->deviceid = deviceid;
	pcmd->ctrlid = ctrlid;
	pcmd->resentignore = 0;
	strncpy (pcmd->ctrltype, ctrltype, _GOS_BUF_LEN);
	if (ctrlarg < 0) {
		pcmd->arg = argument;
		pcmd->tm = worktime;
		pcmd->ctrlarg = gos_get_actuator_argument (argument, worktime);
	} else  {
        gos_parse_actuator_argument (ctrlarg, &(pcmd->arg), &(pcmd->tm));
		pcmd->ctrlarg = ctrlarg;
	}

	pcmd->workcnt = 0;
	pcmd->stopcnt = 0;
	pcmd->senttm = 0;
	pcmd->stat = GOS_CMD_NONE;
}

cf_ret_t
gos_insert_control_history (gos_cmd_t *pcmd, cf_db_t *db) {
	int rc;
	char *errmsg;
	char query[_GOS_BBUF_LEN];
	char exectime[20], senttime[20], finishtime[20]; // YYYY-MM-DD HH:MM:ss

	cf_db_timestring (exectime, sizeof(exectime), pcmd->exectm);
	cf_db_timestring (finishtime, sizeof(finishtime), (int)time(NULL));

	if (pcmd->senttm == 0) {
		sprintf (query, "insert into gos_control_history "
			"(id, exectime, device_id, argument, ctrltype, ctrlid, ctrlarg, "
			"worktime, sentcnt, stopcnt, finishtime, status) "
			"values (%d, '%s', %d, %d, '%s', %d, %d, %d, %d, %d, '%s', %d)",
			pcmd->id, exectime, pcmd->deviceid, pcmd->arg, pcmd->ctrltype, pcmd->ctrlid, pcmd->ctrlarg, 
			pcmd->tm, pcmd->workcnt, pcmd->stopcnt, finishtime, pcmd->stat);
	} else {
		cf_db_timestring (senttime, sizeof(senttime), pcmd->senttm);

		sprintf (query, "insert into gos_control_history "
			"(id, exectime, device_id, argument, ctrltype, ctrlid, ctrlarg, "
			"worktime, sentcnt, stopcnt, finishtime, status, senttime) "
			"values (%d, '%s', %d, %d, '%s', %d, %d, %d, %d, %d, '%s', %d, '%s')",
			pcmd->id, exectime, pcmd->deviceid, pcmd->arg, pcmd->ctrltype, pcmd->ctrlid, pcmd->ctrlarg, 
			pcmd->tm, pcmd->workcnt, pcmd->stopcnt, finishtime, pcmd->stat, senttime);
	}

	rc = cf_db_exec(db, query, NULL, 0, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("control history error : %s, %s", query, errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}

	return CF_OK;
}

cf_ret_t
gos_update_control (gos_cmd_t *pcmd, cf_db_t *db) {
	int rc;
	char *errmsg;
	char query[_GOS_BBUF_LEN];
	char senttime[20]; // YYYY-MM-DD HH:MM:ss

	cf_db_timestring (senttime, sizeof(senttime), pcmd->senttm);

	sprintf (query, "update gos_control "
		"set sentcnt = %d, stopcnt = %d, senttime = '%s', status = %d "
		"where id = %d",
		pcmd->workcnt, pcmd->stopcnt, senttime, pcmd->stat, pcmd->id);

	rc = cf_db_exec(db, query, NULL, 0, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("control update error : %s, %s", query, errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}

	return CF_OK;
}

cf_ret_t
gos_delete_control (gos_cmd_t *pcmd, cf_db_t *db) {
	char query[_GOS_BBUF_LEN];
	char *errmsg;
	int rc;

	sprintf (query, "delete from gos_control where id = %d", pcmd->id);
	rc = cf_db_exec(db, query, NULL, 0, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("control delete error : %s, %s", query, errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}
	return CF_OK;
}

int
gos_is_command_finished (gos_cmd_t *pcmd) {
    if (pcmd->stat >= GOS_CMD_FINISHED_NORMALLY) {
        return 1;
    } else {
        return 0;
    }
}

int
gos_is_command_ignored (gos_cmd_t *pcmd) {
    if (pcmd->stat == GOS_CMD_IGNORED_BY_NEWCOMMAND
		|| pcmd->stat == GOS_CMD_IGNORED_BY_AUTOCONTROL
		|| pcmd->stat == GOS_CMD_IGNORED_WRONGCOMMAND) {
        return 1;
    } else {
        return 0;
    }
}

cf_ret_t
gos_update_command_status (gos_cmd_t *pcmd, gos_cmdstat_t stat, cf_db_t *db) {
	pcmd->stat = stat;
	if (gos_update_control (pcmd, db) == CF_OK) {
		// 명령이 종료된 경우에는 업데이트후 완전 종료함.
        if (gos_is_command_finished (pcmd) || gos_is_command_ignored (pcmd)) {

			CF_VERBOSE (CF_VERBOSE_MID, "A command (%d) of an actuator (%d) finished with status (%d).", pcmd->id, pcmd->deviceid, pcmd->stat);

			gos_insert_control_history (pcmd, db);
			gos_delete_control (pcmd, db);
			pcmd->stat = GOS_CMD_NONE;
		}
		return CF_OK;
	} else {
		return CF_ERR;
	} 
}

gos_cmdstat_t
gos_get_command_status (gos_cmd_t *pcmd) {
	return pcmd->stat;
}

cf_ret_t
gos_load_command_from_result (gos_cmd_t *pcmd, char **result, int idx) {
	gos_set_command (pcmd, 
		atoi (result[idx]),			//id
		cf_db_timet (result[idx + 1]),  // execution time 
		atoi (result[idx + 2]),		// device_id
		atoi (result[idx + 3]),		// argument
		atoi (result[idx + 4]),		// working time
		result[idx + 5],			// ctrltype
		atoi (result[idx + 6]),		// ctrlid
		atoi (result[idx + 7]));	// ctrlarg
	return CF_OK;
}

int
gos_is_auto_command (gos_cmd_t *pcmd) {
	return (pcmd->ctrltype)[0] == 'a';
}

int
gos_is_stop_command (gos_cmd_t *pcmd) {
	return pcmd->tm == 0;
}

gos_cmd_t *
gos_update_waiting_command (gos_dev_t *pdev, gos_cmd_t *pcmd, cf_db_t *db) {
	gos_cmd_t *pwaiting = &((pdev->driver).waitingcmd);

	if (pwaiting->stat != GOS_CMD_NONE) {
		gos_update_command_status (pwaiting, GOS_CMD_IGNORED_BY_NEWCOMMAND, db);
	}
	memcpy (pwaiting, pcmd, sizeof(gos_cmd_t));
	gos_update_command_status (pwaiting, GOS_CMD_WAITING, db);

	return pwaiting;
}

int
gos_is_wrong_command (gos_cmd_t *pcmd) {
    if (pcmd->tm < 0 || pcmd->tm > _GOS_ACTARG_TIMEFILTER)
        return 1;
    // 모터형과 스위치형이 구분이 안되서 완벽한 상태는 아님.
    if (pcmd->arg != _GOS_MOTOR_ARG_OPEN && pcmd->arg != _GOS_MOTOR_ARG_CLOSE)
        return 1;
    return 0;
}

cf_ret_t
gos_read_commands (gos_devinfo_t *pdevinfo, cf_db_t *db) {
	char **result;
	char *errmsg;
	int rows, columns, rc, i;
	int autocontrolmode, pass_deviceid;
	char query[_GOS_BBUF_LEN];
	gos_dev_t *pdev;
	gos_cmd_t cmd;
	gos_cmd_t *pwaiting;

	// select commands from gos_control table
	sprintf (query, "select id, exectime, device_id, argument, worktime, "
			"ctrltype, ctrlid, ctrlarg from gos_control where status = 0 "
			"order by device_id asc, id asc");

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (read command) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	pdev = NULL;
	pass_deviceid = -1;
	autocontrolmode = gos_check_auto_control (gos_get_server (), gos_get_config ());

	CF_VERBOSE (CF_VERBOSE_HIGH, "There are %d commands in the gos_control table.", rows);
	for (i = 1; i <= rows; i++) {
		gos_load_command_from_result (&cmd, result, i * columns);

        // 잘못된 명령이라면 무시한다.
        if (gos_is_wrong_command (&cmd)) {
			gos_update_command_status (&cmd, GOS_CMD_IGNORED_WRONGCOMMAND, db);
            continue;
        }

		// 정지명령이 있다면 해당 장비에 대한 이후의 명령을 패스한다.
		if (cmd.deviceid == pass_deviceid)	
			continue;

		if (autocontrolmode && !(gos_is_auto_command (&cmd))) {
			CF_VERBOSE (CF_VERBOSE_LOW, "During the automatic operation, any manual command was ignored.");
			gos_update_command_status (&cmd, GOS_CMD_IGNORED_BY_AUTOCONTROL, db);

		} else {
			// 현재 디바이스 체크
			if (pdev == NULL || pdev->id != cmd.deviceid)
				pdev = gos_find_device (pdevinfo, cmd.deviceid);

			if (pdev == NULL) {
				CF_ERR_LOG ("There is no device[%d] for a command[%d].", cmd.deviceid, cmd.id);
				continue;
			}

			// 정지명령이 대기중인지 확인
			pwaiting = &((pdev->driver).waitingcmd);
			if (pwaiting->stat == GOS_CMD_WAITING && gos_is_stop_command (pwaiting)) {	
				pass_deviceid = cmd.deviceid;
			} else  {
				gos_update_waiting_command (pdev, &cmd, db); 
			}
		}
	}

	cf_db_free_table (result);

	return CF_OK;
}

cf_ret_t
gos_send_command (gos_dev_t *pdev, gos_conninfo_t *pconninfo, cf_db_t *db) {
	gos_cmd_t *pcmd, *pnewcmd;
	gos_cmd_t tmpcmd;

	pcmd = &((pdev->driver).currentcmd);
	if (pcmd->stat == GOS_CMD_NONE) {	// 현재 진행중인 명령이 없고,
		// check waiting
		pnewcmd = &((pdev->driver).waitingcmd);
		if (pnewcmd->stat == GOS_CMD_WAITING) { // 대기중인 명령이 있다면
			// move waiting to current
			memcpy (pcmd, pnewcmd, sizeof (gos_cmd_t));
			pnewcmd->stat = GOS_CMD_NONE;
			// send it
			gos_send_real_command (pcmd, pconninfo, pdev);
			CF_VERBOSE (CF_VERBOSE_HIGH, "A waiting command[%d] was sent [%d] times.", pcmd->id, pcmd->workcnt);

			gos_update_command_status (pcmd, GOS_CMD_SENT, db);
		}

	} else if (pcmd->stat == GOS_CMD_STOPPING_BY_ASENSOR) { 
		// asensor 에 의해서 중지가 필요한 경우
		memcpy (&tmpcmd, pcmd, sizeof (gos_cmd_t));
		tmpcmd.tm = 0;	// make stop command
		(pcmd->stopcnt)++;
		gos_send_real_command (&tmpcmd, pconninfo, pdev);
		CF_VERBOSE (CF_VERBOSE_HIGH, "A limiter stop command[%d] was sent [%d] times.", pcmd->id, pcmd->workcnt);

	} else if (pcmd->stat == GOS_CMD_STOPPING_BY_STOPCOMMAND) {	
		// 정지 명령에 의해서 중지가 필요한 경우
		pnewcmd = &((pdev->driver).waitingcmd);
		CF_ASSERT (gos_is_stop_command (pnewcmd), "The waiting command should be a stop command.");
		if (pnewcmd->arg != pcmd->arg) {
			CF_VERBOSE (CF_VERBOSE_LOW, "The argument (%d) of a stop command (%d) is not matched "
					"with the argument (%d) of the working command (%d). "
					"So it changed automatically."
					, pnewcmd->arg, pnewcmd->id, pcmd->arg, pcmd->id);
			pnewcmd->arg = pcmd->arg;
		}
		gos_send_real_command (pnewcmd, pconninfo, pdev);
		(pcmd->stopcnt)++;
		CF_VERBOSE (CF_VERBOSE_HIGH, "A stop command[%d] was sent [%d] times.", pnewcmd->id, pnewcmd->workcnt);

	} else if (pcmd->stat == GOS_CMD_SENT && !gos_is_stop_command (pcmd)) {	
		// 구동 명령을 전송하였으나 반응이 없는 경우
		(pcmd->resentignore)++;
		if ((pcmd->resentignore % _GOS_RESENT_IGNORE_COUNT) == 0) {
			CF_ERR_LOG ("Working command was sent but the actuator [%d] does not working. "
						"It waited %d times and it would be sent again.", pdev->id, pcmd->resentignore);
			gos_send_real_command (pcmd, pconninfo, pdev);
			CF_VERBOSE (CF_VERBOSE_HIGH, "A not working command[%d] was sent [%d] times.", 
						pcmd->id, pcmd->workcnt);
		}
	}

	return CF_OK;
}

cf_ret_t
gos_limitstop_actuator (gos_dev_t *pdev, cf_db_t *db) {
	gos_cmd_t *pcmd;

	pcmd = &((pdev->driver).currentcmd);

	if (pcmd->stat == GOS_CMD_WORKING) {
		gos_update_command_status (pcmd, GOS_CMD_STOPPING_BY_ASENSOR, db);
	} else if (pcmd->stat == GOS_CMD_NONE) {
		CF_VERBOSE (CF_VERBOSE_MID, "An actuator [%d] is not working, so no need to stop by limiter.", pdev->id);
	} else {
		CF_ERR_LOG ("There is no working command to stop but the limiter wants to stop the actuator [%d].", pdev->id);
	}

	return CF_OK;
}


cf_ret_t
gos_control_actuator_with_asensor (gos_dev_t *pdev, gos_devinfo_t *pdevinfo, cf_db_t *db) {
	gos_dev_t *pasen;
	int j;

	if ((pdev->driver).used_asensor) {
		for (j = 0; j < _GOS_MAX_ASENSOR; j++) {
			if ((pdev->driver).asensorid[j] != _GOS_NOT_ASENSOR) {
				pasen = gos_find_device (pdevinfo, (pdev->driver).asensorid[j]);
				if (pasen == NULL) {
					CF_ERR_LOG ("can't find actuator sensor [%d].", (pdev->driver).asensorid[j]);
					return CF_ERR;
				}

				if (pasen->rawvalue != GOS_SIGNAL_OFF 
						&& (pdev->driver).type[j] == GOS_ASENSOR_LIMITER) {
					// 리미터가 터치된 상태라면 기존 명령 멈춤 필요
					CF_VERBOSE (CF_VERBOSE_MID, "A actuator [%d] would be stopped by the limiter [%d].", pdev->id, pasen->id);
					gos_limitstop_actuator (pdev, db);
				}
			} else {
				break;
			}
		}
	}
	return CF_OK;
}

cf_ret_t
gos_check_actuator_working_status (gos_dev_t *pdev, cf_db_t *db) {
	int arg, tm;
	gos_cmd_t *pcmd, *pnewcmd;

	pcmd = &((pdev->driver).currentcmd);
	pnewcmd = &((pdev->driver).waitingcmd);

	gos_parse_actuator_argument (pdev->rawvalue, &arg, &tm);

	CF_VERBOSE (CF_VERBOSE_HIGH, "The status of an actuator [%d] is (arg : %d, tm :%d).", pdev->id, arg, tm);
	CF_VERBOSE (CF_VERBOSE_HIGH, "The status of commands [%d, %d] are (%d, %d).", pcmd->id, pnewcmd->id, pcmd->stat, pnewcmd->stat);

	if (tm > 0) {	// still working
		CF_ASSERT (pcmd->stat != GOS_CMD_NONE, "A working actuator should have a command.");
		CF_ASSERT (pcmd->arg == arg || pcmd->arg == 0, "Argument of a working actuator should be same with argument of a working command.");

		if (pcmd->stat == GOS_CMD_SENT) {		// 명령 전송이후 첫번째 확인
			gos_update_command_status (pcmd, GOS_CMD_WORKING, db);
		}

		if (pnewcmd->stat == GOS_CMD_WAITING && gos_is_stop_command (pnewcmd)) {		// 다음 제어명령이 정지명령이라면 정지명령 전송
			gos_update_command_status (pcmd, GOS_CMD_STOPPING_BY_STOPCOMMAND, db);
		}

	} else {	// stopped
		if (pcmd->stat == GOS_CMD_WORKING) {
			gos_update_command_status (pcmd, GOS_CMD_FINISHED_NORMALLY, db);

		} else if (pcmd->stat == GOS_CMD_STOPPING_BY_ASENSOR) {
			gos_update_command_status (pcmd, GOS_CMD_FINISHED_BY_ASENSOR, db);

		} else if (pcmd->stat == GOS_CMD_STOPPING_BY_STOPCOMMAND) {
			gos_update_command_status (pcmd, GOS_CMD_FINISHED_BY_STOPCOMMAND, db);

			if (gos_is_stop_command (pnewcmd)) { // 대기중인 명령이 정지명령이 맞다면 같이 종료함.
				gos_update_command_status (pnewcmd, GOS_CMD_FINISHED_NORMALLY, db);
			}

		} else if (pcmd->stat == GOS_CMD_SENT) {	// 작동이 시작되지 않았고, 전송은 된 상태
			if (pnewcmd->stat == GOS_CMD_WAITING && gos_is_stop_command (pnewcmd)) {		// 다음 제어명령이 정지명령이라면 정지명령 전송
				gos_update_command_status (pcmd, GOS_CMD_STOPPING_BY_STOPCOMMAND, db);
			}

		} else if (pcmd->stat == GOS_CMD_SENT && gos_is_stop_command (pcmd)) {	// 작동이 시작되지 않았고 중지명령이라면, 그냥 정상 종료로 처리함 
			gos_update_command_status (pcmd, GOS_CMD_FINISHED_NORMALLY, db);

		} // else 다른 경우에는 작동 시작 전이고, 작동 시작전이라면 시간은 당연히 0
	}
	return CF_OK;
}

cf_ret_t
gos_select_and_send_command (gos_devinfo_t *pdevinfo, gos_conninfo_t *pconninfo, cf_db_t *db) {
	gos_dev_t *pdev;
	int i;

	for (i = 0; i < pdevinfo->size; i++) {
		pdev = pdevinfo->pdev + i;
		if (pdev->type == GOS_DEV_ACTUATOR) {
			CF_VERBOSE (CF_VERBOSE_HIGH, "A actuator [%d] would be tested for control.", pdev->id);

			// actuator 동작 상태/완료 체크
			gos_check_actuator_working_status (pdev, db);

			// actuator sensor 에 의한 동작 체크
			gos_control_actuator_with_asensor (pdev, pdevinfo, db);

			// 필요하다면 명령 전송 
			gos_send_command (pdev, pconninfo, db);
		}
	}

	return CF_OK;
}

cf_ret_t
gos_control (gos_devinfo_t *pdevinfo, gos_config_t *pconfig, gos_conninfo_t *pconninfo) {
	gos_read_commands (pdevinfo, &(pconfig->db));
	gos_select_and_send_command (pdevinfo, pconninfo, &(pconfig->db));

	return CF_OK;
}

cf_ret_t
gos_reset_sequence_of_control (cf_db_t *db) {
	char query[200];
	char **results;
	char *errmsg;
	int rows, columns, rc, maxid;

	rc = cf_db_get_table (db, "select max(id) from (select max(id) as id from gos_control c "
							"union all select max(id) as id from gos_control_history h) t", 
							&results, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("getting max control id failed. %s", errmsg);
		cf_db_free (errmsg);
		return _GOS_NO_ID;
	}
	CF_VERBOSE (CF_VERBOSE_MID, "Last control id is [%s].", results[1]);
	maxid = atoi (results[1]) + 1;
	cf_db_free_table (results);

	sprintf (query, "alter table gos_control auto_increment = %d", maxid);
	rc = cf_db_exec(db, query, NULL, 0, &errmsg); 
	if (rc != CF_OK) {
		CF_ERR_LOG ("gos_control sequence setting failed :%s", errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}
	CF_VERBOSE (CF_VERBOSE_MID, "gos_control sequence is reset [%d].", maxid);
	return CF_OK;
}
	
cf_ret_t
gos_init_control (gos_config_t *pconfig) {
	int rc;
	char *errmsg;

	if (CF_ERR == gos_reset_sequence_of_control (&(pconfig->db)))
		return CF_ERR;

	rc = cf_db_exec(&(pconfig->db), 
		"insert into gos_control_history "
		"(select * from gos_control where status > 0 )", NULL, 0, &errmsg);

	if (rc != CF_OK) {
		CF_ERR_LOG ("move previous controls into history failed :%s", errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}

	rc = cf_db_exec(&(pconfig->db), 
		"delete from gos_control where status != 0", NULL, 0, &errmsg);

	if (rc != CF_OK) {
		CF_ERR_LOG ("delete previous controls failed :%s", errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}

	return CF_OK;
}
