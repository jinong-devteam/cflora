/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_vsensor.cpp
 * \brief GOS 가상센서관련 소스 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <uv.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <cf.h>
#include <tp3.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_connection.h"
#include "gos_device.h"
#include "gos_vsensor.h"
#include "gos_time.h"

#define _GOS_VS_FILENAME    "filename"
#define _GOS_VS_DEVICE      "device"
#define _GOS_VS_SENSOR      "sensor"
#define _GOS_VS_ACTUATOR    "actuator"
#define _GOS_VS_MOTOR       "motor"
#define _GOS_VS_DNW_DRY     "dry bulb"
#define _GOS_VS_DNW_WET     "wet bulb"
#define _GOS_VS_WINDDIR     "wind direction"
#define _GOS_VS_FIRSTOP     "first operand"
#define _GOS_VS_SECONDOP    "second operand"
#define _GOS_VS_LONGITUDE   "longitude"
#define _GOS_VS_LATITUDE    "latitude"

typedef enum {
    GOS_VSENS_DNWHUM = 1,
    GOS_VSENS_DNWDEW = 2,
    GOS_VSENS_DAILYACC = 3,
    GOS_VSENS_MOVAVG = 4,
    GOS_VSENS_ACTRTIME = 5,
    GOS_VSENS_MOTPOS = 6,
    GOS_VSENS_WINDSIDE = 7,
    GOS_VSENS_OPVSEN = 8,
    GOS_VSENS_CURRENT = 9,
    GOS_VSENS_LASTTIME = 10, 
    GOS_VSENS_FILE = 11,
    GOS_VSENS_SHAREDMEM = 12,
    GOS_VSENS_SUNRISESEC = 13,
    GOS_VSENS_SUNSETSEC = 14,
    GOS_VSENS_AVGSEN = 15,
    GOS_VSENS_TEMPCOMP = 16,
} gos_vsenstype_t;

#define _GOS_VSENS_MAX      16

static char *_str_vsensor[_GOS_VSENS_MAX] = {
    (char *)"_gos_dnwhumidity",
    (char *)"_gos_dnwdewpoint",
    (char *)"_gos_dailyaccum",
    (char *)"_gos_moveavg",
    (char *)"_gos_actrtime",
    (char *)"_gos_motpos",
    (char *)"_gos_windside",
    (char *)"_gos_opvsen",
    (char *)"_gos_current",
    (char *)"_gos_lasttime",
    (char *)"_gos_file",
    (char *)"_gos_sharedmem",
    (char *)"_gos_sunrise",
    (char *)"_gos_sunset",
    (char *)"_gos_avgvsen",
    (char *)"_gos_tempcomp",
};

double
_gos_get_relativehumidity_with_drynwetbulb (double dry, double wet) {
    double pressure = 101.3;
    double coeff = 0.00066 * (1.0 + 0.00115 * wet);
    double ew = exp ((16.78 * wet - 116.9) / (wet + 237.3));
    double ed = exp ((16.78 * dry - 116.9) / (dry + 237.3));

    return 100.0 * (ew - coeff * pressure * (dry - wet)) / ed;
}

ret_t
_gos_get_dnwhumidity (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_dnw_config_t *pconfig = (gos_dnw_config_t *)config;
    gos_dev_t *pdry = gos_find_device (pdevinfo, pconfig->dry_device_id);
    gos_dev_t *pwet = gos_find_device (pdevinfo, pconfig->wet_device_id);
    if (pdry == NULL || pwet == NULL) {
        LOG(ERROR) << "not found target physical sensors for a humidity virtual sensor";
        return ERR;
    }

    *nvalue = _gos_get_relativehumidity_with_drynwetbulb (pdry->nvalue, pwet->nvalue);
    if (*nvalue > 100) {
    	LOG(INFO) << "Calculated humidity is too great." << *nvalue;
        *nvalue = 100;
    } else if (*nvalue < 0) {
    	LOG(INFO) << "Calculated humidity is too small." << *nvalue;
        *nvalue = 0;
    }
    return OK;
}

double
_gos_get_dewpoint_with_drynwetbulb (double dry, double wet) {
    double rhumidity = _gos_get_relativehumidity_with_drynwetbulb (dry, wet);
    double coeffa = 17.625, coeffb = 243.04;
    double numer = coeffb * (log (rhumidity/100) + ((coeffa * dry) / (dry + coeffb)));
    double denom = coeffa - log (rhumidity/100) - ((coeffa * dry) / (dry + coeffb));
    
    return numer / denom;
}

ret_t
_gos_get_dnwdewpoint (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_dnw_config_t *pconfig = (gos_dnw_config_t *)config;
    gos_dev_t *pdry = gos_find_device (pdevinfo, pconfig->dry_device_id);
    gos_dev_t *pwet = gos_find_device (pdevinfo, pconfig->wet_device_id);
    if (pdry == NULL || pwet == NULL) {
        LOG(ERROR) << "not found target physical sensors for a dewpoint virtual sensor.";
        return ERR;
    }

    *nvalue = _gos_get_dewpoint_with_drynwetbulb (pdry->nvalue, pwet->nvalue);
    return OK;
}

ret_t
_gos_get_dailyaccumulation (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    time_t current = time(NULL);
    struct tm *ptm = localtime (&current);
    gos_dayacc_config_t *pconfig = (gos_dayacc_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    time_t gap;
    if (pdev == NULL) {
        LOG(ERROR) << "not found target physical sensor [" << pconfig->device_id 
            << "] for a daily accumulation virtual sensor.";
        return ERR;
    }

    if (pconfig->today != ptm->tm_mday) {
    	LOG(INFO) << "Daily Accum : day changes [" << pconfig->today << "] today [" << ptm->tm_mday << "].";
        pconfig->previous = 0;
        pconfig->today = ptm->tm_mday;
        (pdev->range).skip = 1;		// 날이 바뀌었으니 센서속성값 체크를 하지 않음.
    }

    gap = current - pconfig->last;
    LOG(INFO) << pconfig->device_id << " Daily Accum : gap [" << gap << "] prev [" 
        << pconfig->previous << "] cur [" << pdev->nvalue << "] ratio [" << pconfig->ratio << "].";
    if (gap > 0) {
        *nvalue = pconfig->previous + pdev->nvalue * gap * pconfig->ratio;
        pconfig->previous = *nvalue;
    	pconfig->last = current;
    } else  {
        *nvalue = pconfig->previous;
    }
    //LOG(INFO) << "Daily Accum value " << *nvalue;
    return OK;
}

ret_t
_gos_get_movingaverage (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_movavg_config_t *pconfig = (gos_movavg_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    if (pdev == NULL) {
        LOG(ERROR) << "not found target physical sensors for a moving average virtual sensor.";
        return ERR;
    }

    if (pconfig->isinit) {
        pconfig->previous = pdev->nvalue;
        pconfig->isinit = 0;
    }

    *nvalue = ((pconfig->previous) * (pconfig->number - 1) + pdev->nvalue) 
                / pconfig->number;
    pconfig->previous = *nvalue;

    return OK;
}

ret_t
_gos_get_avgvsen (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_avgvsen_config_t *pconfig = (gos_avgvsen_config_t *)config;
    int i, n;
    double temp = 0;

    for (i = 0, n = 0; i < pconfig->cnt; i++, n++) {
        gos_dev_t *pdev = gos_find_device (pdevinfo, (pconfig->devids)[i]);
        if (pdev == NULL) {
            LOG(ERROR) << "not found target physical sensor [" 
                << (pconfig->devids)[i] << "] for an average virtual sensor.";
            n--;
        }
        temp += pdev->nvalue;
    }

    if (n > 0)
        *nvalue = temp / n;
    else
        *nvalue = 0;

    return OK;
}

ret_t
_gos_get_windside (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_windside_config_t *pconfig = (gos_windside_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    if (pdev == NULL) {
        LOG(ERROR) << "not found target physical sensors for a windside virtual sensor";
        return ERR;
    }

    if (pconfig->angle < 180) {
        if (pconfig->angle > pdev->nvalue 
            || pconfig->angle + 180 < pdev->nvalue)
            *nvalue = 0;
        else
            *nvalue = 1;
    } else {
        if (pconfig->angle > pdev->nvalue 
            || pconfig->angle - 180 < pdev->nvalue)
            *nvalue = 1;
        else
            *nvalue = 0;
    }

    return OK;
}

ret_t
_gos_get_actuatortime (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_vsensor_config_t *pconfig = (gos_vsensor_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    if (pdev == NULL) {
        LOG(ERROR) << "not found target physical sensors for a actuator working time virtual sensor.";
        return ERR;
    }
    *nvalue = gos_get_remaining_time (pdev);
    return OK;
}

ret_t
_gos_get_motorposition (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_vsensor_config_t *pconfig = (gos_vsensor_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    if (pdev == NULL) {
        LOG(ERROR) << "not found target physical sensors for a motor position virtual sensor.";
        return ERR;
    }
    *nvalue = gos_get_driver_position (pdev);
    LOG(INFO) << "Temporal Motor[" << pdev->id << "] Position[" << *nvalue << "] was set.";
    return OK;
}

ret_t
_gos_get_lasttime (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_vsensor_config_t *pconfig = (gos_vsensor_config_t *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, pconfig->device_id);
    if (pdev == NULL) {
        LOG(ERROR) << "not found target device for a last time virtual sensor.";
        return ERR;
    }
    *nvalue = (double)(time(NULL) - pdev->lastchanged);
    return OK;
}

ret_t
_gos_get_opvsensor (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_opvsen_config_t *pconfig = (gos_opvsen_config_t *)config;
    gos_dev_t *pfdev = gos_find_device (pdevinfo, pconfig->fop_device_id);
    gos_dev_t *psdev = gos_find_device (pdevinfo, pconfig->sop_device_id);

    if (pfdev == NULL || psdev == NULL) {
        LOG(ERROR) << "not found target physical sensors for an operation virtual sensor.";
        return ERR;
    }


    switch (pconfig->optype) {
        case OP_PLUS:
            *nvalue = pfdev->nvalue + psdev->nvalue;
             break;
        case OP_MINUS:
            *nvalue = pfdev->nvalue - psdev->nvalue;
             break;
        case OP_ABSMINUS:
            *nvalue = fabs(pfdev->nvalue - psdev->nvalue);
             break;
        case OP_MULTIPLY:
            *nvalue = pfdev->nvalue * psdev->nvalue;
             break;
        case OP_DIVIDE:
            if (psdev->nvalue == 0) {
                *nvalue = 0;
            } else {
                *nvalue = pfdev->nvalue / psdev->nvalue;
            }
             break;
        case OP_MODULUS:
            if (psdev->nvalue == 0) {
                *nvalue = 0;
            } else {
                *nvalue = pfdev->nvalue / psdev->nvalue;
            }
             break;
        default:
             break;
    }

    LOG(INFO) << "opv sensor two value : " << pfdev->nvalue << " (?) " << psdev->nvalue << " = " << *nvalue;

    return OK;
}

ret_t
_gos_get_current (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    *nvalue = (double) gos_gettime_currentsec ();
    return OK;
}

ret_t
_gos_get_sunrisesec (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    *nvalue = (double) gos_gettime_sunrisesec ();
    return OK;
}

ret_t
_gos_get_sunsetsec (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    *nvalue = (double) gos_gettime_sunsetsec ();
    return OK;
}

ret_t
_gos_get_filevalue (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    char buf[_GOS_BUF_LEN];
    gos_filevsen_config_t *pconfig = (gos_filevsen_config_t *)config;

fileagain:
    if (fgets (buf, _GOS_BUF_LEN, pconfig->fp) != NULL) {
        *nvalue = atof (buf);
        return OK;
    } else {
        fclose (pconfig->fp);
        pconfig->fp = fopen (pconfig->fname, "r");
        if (pconfig->fp == NULL) {
            *nvalue = 0;
            return OK;
        } else {
            goto fileagain;
        }
    }
}

ret_t
_gos_get_sharedmem (gos_devinfo_t *pdevinfo, void *config, double *nvalue) {
    gos_shmvsen_config_t *pconfig = (gos_shmvsen_config_t *)config;

shmagain:
    if (pconfig->shm != (void *)-1) {
        memcpy (nvalue, pconfig->shm, sizeof (double));
        return OK;
    } else {
        if ((pconfig->shmid = shmget(pconfig->shmkey, sizeof(double), 0666)) < 0) {
            *nvalue = 0;
            return OK;
        }
        pconfig->shm = shmat(pconfig->shmid, NULL, 0);
        goto shmagain;
    }
}

ret_t
_gos_get_statofnodevice (gos_dev_t *pself, gos_devinfo_t *pdevinfo, void *config, gos_devstat_t *stat) {
    pself->status = GOS_DEVST_ACTIVATED;
    return OK;
}

ret_t
_gos_get_statofonedevice (gos_dev_t *pself, gos_devinfo_t *pdevinfo, void *config, gos_devstat_t *stat) {
    int *device_id = (int *)config;
    gos_dev_t *pdev = gos_find_device (pdevinfo, device_id[0]);
    if (pdev == NULL) {
        LOG(ERROR) << "Not found target physical sensors[" << device_id[0] 
            << "] for a virtual sensor[" << pself->id << "]";
        return ERR;
    }

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
    return OK;
}

ret_t
_gos_get_statoftwodevices (gos_dev_t *pself, gos_devinfo_t *pdevinfo, void *config, gos_devstat_t *stat) {
    gos_vsensor_twodev_config_t *pconfig = (gos_vsensor_twodev_config_t *)config;
    gos_dev_t *pf = gos_find_device (pdevinfo, pconfig->first_id);
    gos_dev_t *ps = gos_find_device (pdevinfo, pconfig->second_id);

    //LOG(INFO) << "stat two device id  : " << pconfig->first_id << "," << pconfig->second_id;
    if (pf == NULL || ps == NULL) {
        LOG(ERROR) << "not found target physical sensors for two arguments virtual sensor status";
        return ERR;
    }

    if (pself->status == GOS_DEVST_INSTALLED) {
        if ((pf->status == GOS_DEVST_DETECTED 
                || pf->status == GOS_DEVST_ACTIVATED)
            && (ps->status == GOS_DEVST_DETECTED 
                || ps->status == GOS_DEVST_ACTIVATED)) {

            pself->status = GOS_DEVST_DETECTED;
        }
    } else if (pself->status != GOS_DEVST_DETECTED) {
        if (pf->status == GOS_DEVST_ABNORMAL 
            || ps->status == GOS_DEVST_ABNORMAL) {

            pself->status = GOS_DEVST_ABNORMAL;
        } else if (pf->status == GOS_DEVST_DISCONNECTED 
                || ps->status == GOS_DEVST_DISCONNECTED) {

            pself->status = GOS_DEVST_DISCONNECTED;
        } else {

            pself->status = GOS_DEVST_ACTIVATED;
        }
    }
    return OK;
}

void
_gos_release_common (void *config) {
    CF_FREE (config);
}

void
_gos_release_file (void *config) {
    gos_filevsen_config_t *pconfig = (gos_filevsen_config_t *)config;
    if (pconfig->fp != NULL)
        fclose (pconfig->fp);
    _gos_release_common (config);
}

void
_gos_release_sharedmem (void *config) {
    gos_shmvsen_config_t *pconfig = (gos_shmvsen_config_t *)config;
    if (pconfig->shm != (void *) -1)
        shmdt (pconfig->shm);
    _gos_release_common (config);
}

ret_t
_gos_load_dailyacc (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_dayacc_config_t *pconfig;
    struct tm *ptm;

    parg->_get_value = _gos_get_dailyaccumulation;
    parg->_get_stat = _gos_get_statofonedevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_dayacc_config_t *)CF_MALLOC (sizeof(gos_dayacc_config_t));
    if (pconfig == NULL) { 
        LOG(ERROR) << "memory allocation for daily accumulation sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_SENSOR) == 0) {
            pconfig->device_id = atoi (result[i * columns + 1]);
            pconfig->ratio = atof (result[i * columns + 2]);
            pconfig->previous = atof (result[i * columns + 4]);
            pconfig->last = time(NULL);
        }
    }
    ptm = localtime (&(pconfig->last));
    pconfig->today = ptm->tm_mday;
    LOG(INFO) << "Daily accumulation sensor " << pconfig->device_id << pconfig->previous;
    parg->config = (void *)pconfig;
    
    return OK;
}

ret_t
_gos_load_movingaverage (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_movavg_config_t *pconfig;

    parg->_get_value = _gos_get_movingaverage;
    parg->_get_stat = _gos_get_statofonedevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_movavg_config_t *)MALLOC (sizeof(gos_movavg_config_t));
    if (pconfig == NULL) { 
        LOG(ERROR) << "memory allocation for moving average sensor argument failed";
        return ERR;
    }

    /* name, channel, opt, ptype */
    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_SENSOR) == 0) {
            pconfig->device_id = atoi (result[i * columns + 1]);
            pconfig->number = atoi (result[i * columns + 2]);
        }
    }
    pconfig->previous = 0;
    pconfig->isinit = 1;
    LOG(INFO) << "moving average sensor [" << pconfig->device_id << "] number [" << pconfig->number << "].";
    parg->config = (void *)pconfig;

    return OK;
}

ret_t
_gos_load_windside (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_windside_config_t *pconfig;

    parg->_get_value = _gos_get_windside;
    parg->_get_stat = _gos_get_statofonedevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_windside_config_t *)MALLOC (sizeof(gos_windside_config_t));
    if (pconfig == NULL) { 
        LOG(ERROR) << "memory allocation for windside sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_WINDDIR) == 0) {
            pconfig->device_id = atoi (result[i * columns + 1]);
            pconfig->angle = atoi (result[i * columns + 2]);
        }
    }

    LOG(INFO) << "Windside sensor [" << pconfig->device_id << "] angle [" << pconfig->angle << "]."; 
    parg->config = (void *)pconfig;

    return OK;
}

ret_t
_gos_load_lasttime (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_vsensor_config_t *pconfig;

    parg->_get_value = _gos_get_lasttime;
    parg->_get_stat = _gos_get_statofonedevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_vsensor_config_t *)MALLOC (sizeof(gos_vsensor_config_t));
    if (pconfig == NULL) { 
        LOG(ERROR) << "memory allocation for lasttime sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_DEVICE) == 0) {
            pconfig->device_id = atoi (result[i * columns + 1]);
        }
    }
    LOG(INFO) << "lasttime device " << pconfig->device_id;
    parg->config = (void *)pconfig;

    return OK;
}

ret_t
_gos_load_actuatortime (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_vsensor_config_t *pconfig;

    parg->_get_value = _gos_get_actuatortime;
    parg->_get_stat = _gos_get_statofonedevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_vsensor_config_t *)MALLOC (sizeof(gos_vsensor_config_t));
    if (pconfig == NULL) { 
        LOG(ERROR) << "memory allocation for actuator time sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_ACTUATOR) == 0) {
            pconfig->device_id = atoi (result[i * columns + 1]);
        }
    }
    LOG(INFO) << "actuator time device " << pconfig->device_id;
    parg->config = (void *)pconfig;

    return OK;
}

ret_t
_gos_load_motorposition (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_vsensor_config_t *pconfig;

    parg->_get_value = _gos_get_motorposition;
    parg->_get_stat = _gos_get_statofonedevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_vsensor_config_t *)MALLOC (sizeof(gos_vsensor_config_t));
    if (pconfig == NULL) { 
        LOG(ERROR) << "memory allocation for motor position sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_MOTOR) == 0) {
            pconfig->device_id = atoi (result[i * columns + 1]);
        }
    }
    LOG(INFO) << "motor position " << pconfig->device_id;
    parg->config = (void *)pconfig;

    return OK;
}

ret_t
_gos_load_opvsensor (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_opvsen_config_t *pconfig;

    parg->_get_value = _gos_get_opvsensor;
    parg->_get_stat = _gos_get_statoftwodevices;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_opvsen_config_t *)CF_MALLOC (sizeof(gos_opvsen_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for operation virtual sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_FIRSTOP) == 0) {
            pconfig->fop_device_id = atoi (result[i * columns + 1]);
            pconfig->optype = (gos_op_t) atoi (result[i * columns + 2]);
        } else if (strcmp (result[i * columns], _GOS_VS_SECONDOP) == 0) {
            pconfig->sop_device_id = atoi (result[i * columns + 1]);
        }
    }
    LOG(INFO) << "operation first operand[" << pconfig->fop_device_id << "], second operand[" << pconfig->sop_device_id << "], operator[" << pconfig->optype << "]."; 
    parg->config = (void *)pconfig;
    
    return OK;
}

ret_t
_gos_load_avgvsen (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_avgvsen_config_t *pconfig;

    parg->_get_value = _gos_get_avgvsen;
    parg->_get_stat = _gos_get_statofnodevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_avgvsen_config_t *)CF_MALLOC (sizeof(gos_avgvsen_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for average virtual sensor argument failed";
        return ERR;
    }

    pconfig->cnt = 0;
    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_DEVICE) == 0) {
            (pconfig->devids)[pconfig->cnt]= atoi (result[i * columns + 1]);
            (pconfig->cnt)++;
            LOG(INFO) << "avgvsen " << pconfig->cnt << "th device_id[" 
                << (pconfig->devids)[pconfig->cnt - 1]<< "]";
            if (pconfig->cnt >= _GOS_VSEN_AVG_MAX) {
                LOG(ERROR) << "Number of devices for average vsensor is over.";
                return ERR;
            }
        }
    }
    parg->config = (void *)pconfig;
    
    return OK;
}

ret_t
_gos_load_current (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    gos_vsensor_config_t *pconfig;

    parg->_get_value = _gos_get_current;
    parg->_get_stat = _gos_get_statofnodevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_vsensor_config_t *)CF_MALLOC (sizeof(gos_vsensor_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for current virtual sensor argument failed";
        return ERR;
    }
    parg->config = (void *)pconfig;
    return OK;
}

ret_t
_gos_load_sunrisesec (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    gos_vsensor_config_t *pconfig;

    parg->_get_value = _gos_get_sunrisesec;
    parg->_get_stat = _gos_get_statofnodevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_vsensor_config_t *)CF_MALLOC (sizeof(gos_vsensor_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for sunrise virtual sensor argument failed";
        return ERR;
    }
    parg->config = (void *)pconfig;
    return OK;
}

ret_t
_gos_load_sunsetsec (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    gos_vsensor_config_t *pconfig;

    parg->_get_value = _gos_get_sunsetsec;
    parg->_get_stat = _gos_get_statofnodevice;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_vsensor_config_t *)CF_MALLOC (sizeof(gos_vsensor_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for sunset virtual sensor argument failed";
        return ERR;
    }
    parg->config = (void *)pconfig;
    return OK;
}

ret_t
_gos_load_filevsen (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    gos_filevsen_config_t *pconfig;

    parg->_get_value = _gos_get_filevalue;
    parg->_get_stat = _gos_get_statofnodevice;
    parg->_release_config = _gos_release_file;

    pconfig = (gos_filevsen_config_t *)CF_MALLOC (sizeof(gos_filevsen_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for file virtual sensor argument failed";
        return ERR;
    }

    sprintf (pconfig->fname, "/tmp/%d.farmos.vsensor", devid);
    LOG(INFO) << "file virtual sensor " << pconfig->fname;
    pconfig->fp = NULL;
    parg->config = (void *)pconfig;
    
    return OK;
}

ret_t
_gos_load_sharedmem (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    gos_shmvsen_config_t *pconfig;

    parg->_get_value = _gos_get_sharedmem;
    parg->_get_stat = _gos_get_statofnodevice;
    parg->_release_config = _gos_release_sharedmem;

    pconfig = (gos_shmvsen_config_t *)CF_MALLOC (sizeof(gos_shmvsen_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for shared memory virtual sensor argument failed";
        return ERR;
    }

    pconfig->shmkey= devid;
    pconfig->shmid = 0;
    pconfig->shm = (void *)-1;
    LOG(INFO) << "shared memeory key " << pconfig->shmkey;
    parg->config = (void *)pconfig;
    
    return OK;
}

ret_t
_gos_load_dnwhumidity (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    int i;
    gos_dnw_config_t *pconfig;

    parg->_get_value = _gos_get_dnwhumidity;
    parg->_get_stat = _gos_get_statoftwodevices;
    parg->_release_config = _gos_release_common;
    pconfig = (gos_dnw_config_t *)CF_MALLOC (sizeof(gos_dnw_config_t));
    if (pconfig == NULL) {
        LOG(ERROR) << "memory allocation for dnw humidity virtual sensor argument failed";
        return ERR;
    }

    for (i = 1; i <= rows; i++) {
        if (strcmp (result[i * columns], _GOS_VS_DNW_DRY) == 0) {
            pconfig->dry_device_id = atoi (result[i * columns + 1]);
        } else if (strcmp (result[i * columns], _GOS_VS_DNW_WET) == 0) {
            pconfig->wet_device_id = atoi (result[i * columns + 1]);
        }
    }
    LOG(INFO) << "dry, wet bulbs " << pconfig->dry_device_id << pconfig->wet_device_id; 
    parg->config = (void *)pconfig;
    
    return OK;
}

ret_t
_gos_load_dnwdewpoint (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns) {
    ret_t ret = _gos_load_dnwhumidity (devid, parg, result, rows, columns);
    if (ret == OK) {
        parg->_get_value = _gos_get_dnwdewpoint;
        parg->_get_stat = _gos_get_statoftwodevices;
        parg->_release_config = _gos_release_common;
    }
    return ret;
}

/** 가상 센서의 설정을 로드하기 위한 함수 포인터 */
typedef ret_t (*_gos_load_vsensor_config_func) 
    (int devid, gos_cvt_vsensor_arg_t *parg, char **result, int rows, int columns);

static _gos_load_vsensor_config_func _load_funcs[_GOS_VSENS_MAX] = {
    _gos_load_dnwhumidity,
    _gos_load_dnwdewpoint,
    _gos_load_dailyacc,
    _gos_load_movingaverage,
    _gos_load_actuatortime,
    _gos_load_motorposition,
    _gos_load_windside,
    _gos_load_opvsensor,
    _gos_load_current,
    _gos_load_lasttime,
    _gos_load_filevsen,
    _gos_load_sharedmem,
    _gos_load_sunrisesec,
    _gos_load_sunsetsec,
    _gos_load_avgvsen,
};

ret_t
gos_default_load (gos_cvt_vsensor_arg_t *parg, int devid, cf_db_t *db, int vsidx) {
    char query[_GOS_BUF_LEN];
    char **result;
    char *errmsg;
    int rows, columns, rc;
    ret_t ret;

    sprintf(query, "select p.name, p.channel, p.opt, p.ptype, "
            " e.nvalue, UNIX_TIMESTAMP(e.obstime) "
            " from gos_device_portmap p, gos_environment_current e "
            " where p.device_id = %d and p.device_id = e.device_id ", devid);

    //LOG(INFO) << "virtual Sensor " << query;

    rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
    if (rc != OK) {
        LOG(ERROR) <<"database query execution (for getting devicemap) failed. ";
        cf_db_free(errmsg);
        return ERR;
    }

    LOG(INFO) << "virtual Sensor [" << devid << "] config loading.";
    ret = _load_funcs[vsidx] (devid, parg, result, rows, columns);

    cf_db_free_table (result);
    return ret;
}

void *
gos_generate_vsensor_arg (gos_dev_t *pdev, char *configstr, double offset, cf_db_t *db) {
    gos_cvt_vsensor_arg_t  *parg;
    int i;

    for (i = 0; i < _GOS_VSENS_MAX; i++) {
        if (strcmp (_str_vsensor[i], configstr) == 0) {
            parg = (gos_cvt_vsensor_arg_t  *)CF_MALLOC (sizeof (gos_cvt_vsensor_arg_t));
            if (parg == NULL) {
                LOG(ERROR) << "memory allocatioin for virtual sensor argument failed.";
                return NULL;
            }

            if (ERR == gos_default_load (parg, pdev->id, db, i)) { 
                LOG(ERROR) << "to load virtual sensor config failed.";
                return NULL;
            }

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

