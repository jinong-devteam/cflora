/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_process.cpp
 * \brief GOS 메세지 처리관련 소스 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <uv.h>
#include <tp3.h>
#include <cf.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_connection.h"
#include "gos_device.h"
#include "gos_process.h"
#include "gos_control.h"
#include "gos_rule.h"
#include "gos_server.h"
#include "gos_time.h"

void
gos_set_tp3arg (gos_tp3arg_t *parg, uv_stream_t *handle) {
    parg->pdevinfo = gos_get_devinfo ();
    parg->pconn = gos_get_conninfo ();
    parg->handle = handle;
}

tp3_stat_t
gos_statmsg_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data) {
    gos_tp3arg_t *parg = (gos_tp3arg_t *)data;
    gos_conn_t *pconn = gos_find_connection_by_handle (parg->pconn, parg->handle);
    gos_dev_t *pdev; 
    tp3_stat_t sc; 
    int i, n, gcgid, nodeid, devid;
    int id[_GOS_MAX_DEV], value[_GOS_MAX_DEV]; 

    sc = tp3_defaultconfirm (preq, pres, NULL);
    if (TP3_SC_NOERROR != sc) {
        LOG(ERROR) << "GCG connect accept message generation failed. " << tp3_geterrormsg (sc);
        return sc;
    }
    LOG(INFO) << "stat notice [" << (preq->header).msgseq << "].";

    if ((pres->header).rescode == TP3_RC_NORMAL) {
        gcgid = pconn->gcgid;
        tp3_parse_statmsg (preq, TP3_DEV_SNODE, &n, id, value, _GOS_MAX_DEV);
        LOG(INFO) << "sensor node count : " << n;
        for (i = 0; i < n; i++) {
            pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, id[i], _GOS_NO_ID);
            gos_update_stat (pdev, (gos_devstat_t)value[i]);
        }

        tp3_parse_statmsg (preq, TP3_DEV_ANODE, &n, id, value, _GOS_MAX_DEV);
        LOG(INFO) << "actuator node count : " << n;
        for (i = 0; i < n; i++) {
            pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, id[i], _GOS_NO_ID);
            gos_update_stat (pdev, (gos_devstat_t)value[i]);
        }

        tp3_parse_statmsg (preq, TP3_DEV_SENSOR, &n, id, value, _GOS_MAX_DEV);
        LOG(INFO) << "sensor count : " << n;
        for (i = 0; i < n; i++) {
            nodeid = tp3_getnid (id[i]);
            devid = tp3_getdevid (id[i]);
            pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
        
            if ( pdev == NULL ) {
                LOG(ERROR) << "not found sensor " << gcgid << nodeid << devid;
            } else {
                gos_update_stat (pdev, (gos_devstat_t)value[i]);
            }
        }

        tp3_parse_statmsg (preq, TP3_DEV_ACTUATOR, &n, id, value, _GOS_MAX_DEV);
        LOG(INFO) << "actuator count : " << n;

        for (i = 0; i < n; i++) {
            nodeid = tp3_getnid (id[i]);
            devid = tp3_getdevid (id[i]);
            pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
            if ( pdev == NULL ) {
                LOG(ERROR) << "not found actuator " << gcgid << nodeid << devid;
            } else {
                gos_update_stat (pdev, (gos_devstat_t)value[i]);
                
            }
        }

    } else {
        gos_remove_connection (parg->pconn, parg->handle);
    }

    return TP3_SC_NOERROR;
}

ret_t
gos_update_actuator_action (gos_dev_t *pdev, cf_db_t *db) {
    int arg, tm;
    gos_cmd_t *pcurrent = &((pdev->driver).currentcmd);
    char query[_GOS_BUF_LEN];

    gos_parse_actuator_argument (pdev->rawvalue, &arg, &tm);

    if (pcurrent -> stat == GOS_CMD_NONE && tm > 0) {// 명령이 없는데 동작하고 있는 경우 
        sprintf (query, "INSERT INTO gos_device_history (updatetime, device_id, rawvalue, argument, worktime) VALUES (now(), %d, %d, %d, %d)", pdev->id, pdev->rawvalue, arg, tm);
        LOG(ERROR) << "There is no command. But an actuator[" << pdev->id << "] is working now.";
        CF_EXECUTE (db, query);

    } else if (pcurrent -> stat != GOS_CMD_NONE) { // 명령에 따라 동작하는 경우
        sprintf (query, "INSERT INTO gos_device_history (updatetime, device_id, rawvalue, argument, worktime, control_id) VALUES (now(), %d, %d, %d, %d, %d)", pdev->id, pdev->rawvalue, arg, tm, pcurrent->id);
        CF_EXECUTE (db, query);
    }  // 명령도 없고 동작도 안하고 있다면 이력을 남길 이유가 없음 

    return OK;
}

tp3_stat_t
gos_envmsg_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data) {
    gos_tp3arg_t *parg = (gos_tp3arg_t *)data;
    gos_conn_t *pconn = gos_find_connection_by_handle (parg->pconn, parg->handle);
    tp3_stat_t sc; 
    gos_dev_t *pdev;
    int i, n, nodeid, devid, gcgid;
    int arg, tm;
    int id[_GOS_MAX_DEV], value[_GOS_MAX_DEV]; 
    double nvalue;
    gos_config_t *pconfig = gos_get_config ();
    cf_db_t *db = &(pconfig->db);

    sc = tp3_defaultconfirm (preq, pres, NULL);
    if (TP3_SC_NOERROR != sc) {
        LOG(ERROR) << "GCG connect accept message generation failed. " << tp3_geterrormsg (sc);
        return sc;
    }

    if (OK != cf_db_open (db)) {
        LOG(ERROR) << "database open error in envmsg callback";
    }

    LOG(INFO) << "env notice [" << (preq->header).msgseq << "]";
    if ((pres->header).rescode == TP3_RC_NORMAL) {
        gcgid = pconn->gcgid;
        tp3_parse_envmsg (preq, TP3_DEV_SENSOR, &n, id, value, _GOS_MAX_DEV);
        LOG(INFO) << "sensor count : " << n;
        for (i = 0; i < n; i++) {
            nodeid = tp3_getnid (id[i]);
            devid = tp3_getdevid (id[i]);
            pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
            if ( pdev == NULL ) {
                LOG(ERROR) << "not found sensor : " << gcgid << nodeid << devid;
            } else {
                nvalue = 0;
                if ( gos_convert_env (pdev, value[i], &nvalue) == OK ) {
                    LOG(INFO) << "sensor. devid : " << devid << ", raw : " << value[i] << ", nvalue : " << nvalue;
                    gos_update_env (pdev, nvalue, value[i], 0);
                    gos_test_rules (gos_get_ruleset (), pdev->deviceid, nvalue);
                } else {
                    LOG(ERROR) << "raw data converting failed. devid : " << devid << ", raw : " << value[i];
                }
            }
        }

        tp3_parse_envmsg (preq, TP3_DEV_ACTUATOR, &n, id, value, _GOS_MAX_DEV);
        LOG(INFO) << "actuator count : " << n;
        for (i = 0; i < n; i++) {
            nodeid = tp3_getnid (id[i]);
            devid = tp3_getdevid (id[i]);
            pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
            if ( pdev == NULL ) {
                LOG(ERROR) << "not found actuator: " << gcgid << nodeid << devid;
            } else {
                gos_parse_actuator_argument (value[i], &arg, &tm);
                LOG(INFO) << "actuator. devid : " << devid << ", arg: " << arg << ", time : " << tm;
                //debug
                if (arg > 2 || arg <= 0) {
                    LOG(INFO) << "strange arg : " << arg;
                    tp3_printmsg (preq);

                } else {
                    gos_update_env (pdev, value[i], value[i], 0);
                    //gos_update_actuator_action (pdev, db);
                }
            }
        }

    } else {
        gos_remove_connection (parg->pconn, parg->handle);
    }

    cf_db_close (db);

    return TP3_SC_NOERROR;
}

tp3_stat_t
gos_connmsg_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data) {
    gos_tp3arg_t *parg = (gos_tp3arg_t *)data;
    tp3_stat_t sc; 

    sc = tp3_defaultconfirm (preq, pres, NULL);
    if (TP3_SC_NOERROR != sc) {
        LOG(ERROR) << "GCG connect accept message generation failed. " << tp3_geterrormsg (sc);
        return sc;
    }

    if ((pres->header).rescode == TP3_RC_NORMAL) {
        gos_set_gcgid (parg->pconn, parg->handle, (pres->header).gcgid);
        LOG(INFO) << "Connection was accepted. [gcgid : " << (pres->header).gcgid << " ].";

        //gos_init_control (gos_get_config ());
    } else {
        LOG(INFO) << "Connection was not accepted [" << (pres->header).rescode << "].";
        gos_set_remove_connection (parg->pconn, parg->handle);
    }
    return TP3_SC_NOERROR;
}

void
gos_timer_cb (uv_timer_t *handle) {
	static int ncnt = 0 ;
	cf_db_t *db = &(gos_get_config ()->db);
	gos_config_t *pconfig ;

	gos_gettime_timer ();

	if (cf_db_open (db)) {
        LOG(ERROR) << "database open failed in timer callback.";
        return;
    }

	ncnt++;

	pconfig = gos_get_config ();
	gos_read_devinfo (gos_get_devinfo (), gos_get_config ());

	if (ncnt % pconfig->nupdate == 0) {
		LOG(INFO) << "Update environment infomation to db.";
		gos_update_devinfo (gos_get_devinfo (), gos_get_config ());
	}

	if (ncnt % pconfig->nwrite == 0) {
		LOG(INFO) << "Write device infomation to db.";
		gos_write_devinfo (gos_get_devinfo (), gos_get_config ());
	}

	if (ncnt % pconfig->nrule == 0 && gos_check_auto_control (gos_get_server (), pconfig)) {
		LOG(INFO) << "Evaluate rules.";
		gos_evaluate_rules (gos_get_ruleset (), pconfig);
		gos_reset_rulecondition (gos_get_ruleset ());
	}

	gos_control (gos_get_devinfo (), pconfig, gos_get_conninfo ());

	gos_check_restart (gos_get_server (), pconfig);

	cf_db_close (db);
}
