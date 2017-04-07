#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <uv.h>

#include <cf.h>
#include <tp3.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_connection.h"
#include "gos_device.h"
#include "gos_vsensor.h"

static char *_str_vsensor[_GOS_VSENS_MAX] = {
	"_gos_dnwhumidity",
	"_gos_dnwdewpoint",
	"_gos_dailyaccum"
};


double
_gos_get_relativehumidity_with_drynwetbulb (double dry, double wet) {
	double pressure = 101.3;
	double coeff = 0.00066 * (1.0 + 0.00115 * wet);
	double ew = exp ((16.78 * wet - 116.9) / (wet + 237.3));
	double ed = exp ((16.78 * dry - 116.9) / (dry + 237.3));

	return 100.0 * (ew - coeff * pressure * (dry - wet)) / ed;
}

cf_ret_t
_gos_get_dnwhumidity (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
	gos_dnw_config_t *pconfig = (gos_dnw_config_t *)config;
	gos_dev_t *pdry = gos_find_device (pdevinfo, pconfig->dry_device_id);
	gos_dev_t *pwet = gos_find_device (pdevinfo, pconfig->wet_device_id);
	CF_ERR_RETURN (pdry == NULL || pwet == NULL, "not found target physical sensors");

	*nvalue = _gos_get_relativehumidity_with_drynwetbulb (pdry->nvalue, pwet->nvalue);
	return CF_OK;
}

double
_gos_get_dewpoint_with_drynwetbulb (double dry, double wet) {
	double rhumidity = _gos_get_relativehumidity_with_drynwetbulb (dry, wet);
	double coeffa = 17.625, coeffb = 243.04;
	double numer = coeffb * (log (rhumidity/100) + ((coeffa * dry) / (dry + coeffb)));
	double denom = coeffa - log (rhumidity/100) - ((coeffa * dry) / (dry + coeffb));
	
	return numer / denom;
}

cf_ret_t
_gos_get_dnwdewpoint (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
	gos_dnw_config_t *pconfig = (gos_dnw_config_t *)config;
	gos_dev_t *pdry = gos_find_device (pdevinfo, pconfig->dry_device_id);
	gos_dev_t *pwet = gos_find_device (pdevinfo, pconfig->wet_device_id);
	CF_ERR_RETURN (pdry == NULL || pwet == NULL, "not found target physical sensors");

	*nvalue = _gos_get_dewpoint_with_drynwetbulb (pdry->nvalue, pwet->nvalue);
	return CF_OK;
}

cf_ret_t
_gos_get_dailyaccumulation (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    time_t current = time(NULL);
    struct tm *ptm = localtime (&current);
    gos_dayacc_config_t *pconfig = (gos_dayacc_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    time_t gap;
	CF_ERR_RETURN (pdev == NULL, "not found target physical sensors");

    if (pconfig->today != ptm->tm_mday) {
        pconfig->previous = 0;
        pconfig->today = ptm->tm_mday;
    }

    gap = current - pconfig->last;
    pconfig->last = current;
    if (gap > 0) {
	    *nvalue = pconfig->previous + pdev->nvalue * gap / 3600;
        pconfig->previous = *nvalue;
    } else  {
        *nvalue = pconfig->previous;
    }
	return CF_OK;
}

cf_ret_t
_gos_get_dailyaccstat (gos_dev_t *pself, gos_devinfo_t *pdevinfo, void *config, gos_devstat_t *stat) {
    gos_dayacc_config_t *pconfig = (gos_dayacc_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
	CF_ERR_RETURN (pdev == NULL, "not found target physical sensors");

	if (pself->status == GOS_DEVST_INSTALLED) {
		if (pdev->status == GOS_DEVST_DETECTED 
				|| pdev->status == GOS_DEVST_ACTIVATED) {

			pself->status = GOS_DEVST_DETECTED;
		}
	} else if (pself->status != GOS_DEVST_DETECTED) {
		if (pdev->status == GOS_DEVST_ABNORMAL) {
			pself->status = GOS_DEVST_ABNORMAL;
		} else if (pdev->status == GOS_DEVST_DISCONNECTED) {
			pself->status = GOS_DEVST_DISCONNECTED;
		} else {
			pself->status = GOS_DEVST_ACTIVATED;
        }
    }
	return CF_OK;
}

cf_ret_t
_gos_get_dnwstat (gos_dev_t *pself, gos_devinfo_t *pdevinfo, void *config, gos_devstat_t *stat) {
	gos_dnw_config_t *pconfig = (gos_dnw_config_t *)config;
	gos_dev_t *pdry = gos_find_device (pdevinfo, pconfig->dry_device_id);
	gos_dev_t *pwet = gos_find_device (pdevinfo, pconfig->wet_device_id);
	CF_ERR_RETURN (pdry == NULL || pwet == NULL, "not found target physical sensors");

	if (pself->status == GOS_DEVST_INSTALLED) {
		if ((pdry->status == GOS_DEVST_DETECTED 
				|| pdry->status == GOS_DEVST_ACTIVATED)
			&& (pwet->status == GOS_DEVST_DETECTED 
				|| pwet->status == GOS_DEVST_ACTIVATED)) {

			pself->status = GOS_DEVST_DETECTED;
		}
	} else if (pself->status != GOS_DEVST_DETECTED) {
		if (pdry->status == GOS_DEVST_ABNORMAL 
			|| pwet->status == GOS_DEVST_ABNORMAL) {

			pself->status = GOS_DEVST_ABNORMAL;
		} else if (pdry->status == GOS_DEVST_DISCONNECTED 
				|| pwet->status == GOS_DEVST_DISCONNECTED) {

			pself->status = GOS_DEVST_DISCONNECTED;
		} else {

			pself->status = GOS_DEVST_ACTIVATED;
		}
	}
	return CF_OK;
}

void
_gos_release_dnwhumidity (void *config) {
	CF_FREE (config);
}

void
_gos_release_dnwdewpoint (void *config) {
	CF_FREE (config);
}

void
_gos_release_dailyacc (void *config) {
	CF_FREE (config);
}

cf_ret_t
_gos_load_dailyacc (gos_cvt_vsensor_arg_t *parg, int devid, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char **result;
	char *errmsg;
	int rows, columns, rc, i;
	gos_dayacc_config_t *pconfig;

	parg->_get_value = _gos_get_dailyaccumulation;
	parg->_get_stat = _gos_get_dailyaccstat;
	parg->_release_config = _gos_release_dailyacc;
	pconfig = (gos_dayacc_config_t *)CF_MALLOC (sizeof(gos_dayacc_config_t));
	CF_ERR_RETURN (pconfig == NULL, 
			"memory allocation for daily accumulation sensor argument failed");

	sprintf(query, "select name, channel from gos_device_portmap "
				"where device_id = %d and ptype = 'sensor'", devid);

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (for getting devicemap) failed. %s", errmsg);
		cf_db_free(errmsg);
		CF_FREE (pconfig);
		return CF_ERR;
	}

	for (i = 1; i <= rows; i++) {
		if (strcmp (result[i * columns], _DAY_ACC) == 0) {
			pconfig->device_id = atoi (result[i * columns + 1]);
		}
	}
	pconfig->previous = 0;
	pconfig->last = time(NULL);
	pconfig->today = 32;
	CF_VERBOSE (CF_VERBOSE_LOW, "virtual Sensor [%d] config : sensor [%d].", 
				devid, pconfig->device_id);
	parg->config = (void *)pconfig;
	cf_db_free_table (result);
	return CF_OK;
}

cf_ret_t
_gos_load_dnwhumidity (gos_cvt_vsensor_arg_t *parg, int devid, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char **result;
	char *errmsg;
	int rows, columns, rc, i;
	gos_dnw_config_t *pconfig;

	parg->_get_value = _gos_get_dnwhumidity;
	parg->_get_stat = _gos_get_dnwstat;
	parg->_release_config = _gos_release_dnwhumidity;
	pconfig = (gos_dnw_config_t *)CF_MALLOC (sizeof(gos_dnw_config_t));
	CF_ERR_RETURN (pconfig == NULL, 
			"memory allocation for dnw humidity sensor argument failed");

	sprintf(query, "select name, channel from gos_device_portmap "
				"where device_id = %d and ptype = 'sensor'", devid);

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (for getting devicemap) failed. %s", errmsg);
		cf_db_free(errmsg);
		CF_FREE (pconfig);
		return CF_ERR;
	}

	for (i = 1; i <= rows; i++) {
		if (strcmp (result[i * columns], _DNW_DRY) == 0) {
			pconfig->dry_device_id = atoi (result[i * columns + 1]);
		} else if (strcmp (result[i * columns], _DNW_WET) == 0) {
			pconfig->wet_device_id = atoi (result[i * columns + 1]);
		}
	}
	CF_VERBOSE (CF_VERBOSE_LOW, "virtual Sensor [%d] config : dry [%d], wet [%d].", 
				devid, pconfig->dry_device_id, pconfig->wet_device_id); 
	parg->config = (void *)pconfig;
	cf_db_free_table (result);
	return CF_OK;
}

cf_ret_t
_gos_load_dnwdewpoint (gos_cvt_vsensor_arg_t *parg, int devid, cf_db_t *db) {
	cf_ret_t ret = _gos_load_dnwhumidity (parg, devid, db);
	if (ret == CF_OK) {
		parg->_get_value = _gos_get_dnwdewpoint;
		parg->_get_stat = _gos_get_dnwstat;
		parg->_release_config = _gos_release_dnwdewpoint;
	}
	return ret;
}

/** 가상 센서의 설정을 로드하기 위한 함수 포인터 */
typedef cf_ret_t (*_gos_load_vsensor_config_func) (gos_cvt_vsensor_arg_t *parg, int devid, cf_db_t *db);

static _gos_load_vsensor_config_func _load_funcs[_GOS_VSENS_MAX] = {
	_gos_load_dnwhumidity,
	_gos_load_dnwdewpoint,
	_gos_load_dailyacc
};

void *
gos_generate_vsensor_arg (gos_dev_t *pdev, char *configstr, double offset, cf_db_t *db) {
	gos_cvt_vsensor_arg_t  *parg;
	int i;

	for (i = 0; i < _GOS_VSENS_MAX; i++) {
		if (strcmp (_str_vsensor[i], configstr) == 0) {
			parg = (gos_cvt_vsensor_arg_t  *)CF_MALLOC (sizeof (gos_cvt_vsensor_arg_t));
			CF_EXP_RETURN (parg == NULL, 
					NULL, "memory allocatioin for virtual sensor argument failed");
			CF_EXP_RETURN (CF_ERR == _load_funcs[i] (parg, pdev->id, db), 
					NULL, "to load virtual sensor config failed.");
			parg->offset = offset;
			return parg;
		}
	}
	return NULL;
}

void
gos_release_vsensor_arg (gos_cvt_vsensor_arg_t *parg) {
	if (parg->_release_config)
		(parg->_release_config)(parg->config);
}

