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
		CF_ERR_LOG ("GCG connect accept message generation failed. (%s)", tp3_geterrormsg (sc));
		return sc;
	}
	CF_VERBOSE (CF_VERBOSE_MID, "stat notice [%d]", (preq->header).msgseq);

	if ((pres->header).rescode == TP3_RC_NORMAL) {
		gcgid = pconn->gcgid;
		tp3_parse_statmsg (preq, TP3_DEV_SNODE, &n, id, value, _GOS_MAX_DEV);
		CF_VERBOSE (CF_VERBOSE_MID, "sensor node count : %d", n);
		for (i = 0; i < n; i++) {
			pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, id[i], _GOS_NO_ID);
			gos_update_stat (pdev, value[i]);
		}

		tp3_parse_statmsg (preq, TP3_DEV_ANODE, &n, id, value, _GOS_MAX_DEV);
		CF_VERBOSE (CF_VERBOSE_MID, "actuator node count : %d", n);
		for (i = 0; i < n; i++) {
			pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, id[i], _GOS_NO_ID);
			gos_update_stat (pdev, value[i]);
		}

		tp3_parse_statmsg (preq, TP3_DEV_SENSOR, &n, id, value, _GOS_MAX_DEV);
		CF_VERBOSE (CF_VERBOSE_MID, "sensor count : %d", n);
		for (i = 0; i < n; i++) {
			nodeid = tp3_getnid (id[i]);
			devid = tp3_getdevid (id[i]);
			pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
		
			// by chj 2015.9.28
			if ( pdev == NULL ) {
				CF_ERR_LOG("not found sensor [%d][%d][%d]", gcgid, nodeid, devid);
//				CF_EXP_RETURN (pdev == NULL, TP3_SC_NOT_FOUND_DEVID, "not found sensor [%d][%d][%d]", gcgid, nodeid, devid);
			}
			else {
				gos_update_stat (pdev, value[i]);
			}
		}

		tp3_parse_statmsg (preq, TP3_DEV_ACTUATOR, &n, id, value, _GOS_MAX_DEV);
		CF_VERBOSE (CF_VERBOSE_MID, "actuator count : %d", n);
		for (i = 0; i < n; i++) {
			nodeid = tp3_getnid (id[i]);
			devid = tp3_getdevid (id[i]);
			pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
			// by chj 2015.9.28
			if ( pdev == NULL ) {
				CF_ERR_LOG("not found actuator [%d][%d][%d]", gcgid, nodeid, devid);
//				CF_EXP_RETURN (pdev == NULL, TP3_SC_NOT_FOUND_DEVID, "not found actuator [%d][%d][%d]", gcgid, nodeid, devid);
			} else {
				gos_update_stat (pdev, value[i]);
				
			}
		}

	} else {
		gos_remove_connection (parg->pconn, parg->handle);
	}

	return TP3_SC_NOERROR;
}

cf_ret_t
gos_update_actuator_action (gos_dev_t *pdev, cf_db_t *db) {
	int arg, tm;
	gos_cmd_t *pcurrent = &((pdev->driver).currentcmd);
	char query[_GOS_BUF_LEN];

	gos_parse_actuator_argument (pdev->rawvalue, &arg, &tm);

	if (pcurrent -> stat == GOS_CMD_NONE && tm > 0) {// 명령이 없는데 동작하고 있는 경우 
		sprintf (query, "INSERT INTO gos_device_history (updatetime, device_id, rawvalue, argument, worktime) VALUES (now(), %d, %d, %d, %d)", pdev->id, pdev->rawvalue, arg, tm);
		CF_ERR_LOG ("There is no command. But an actuator[%d] is working now.", pdev->id);
		CF_EXECUTE (db, query);

	} else if (pcurrent -> stat != GOS_CMD_NONE) { // 명령에 따라 동작하는 경우
		sprintf (query, "INSERT INTO gos_device_history (updatetime, device_id, rawvalue, argument, worktime, control_id) VALUES (now(), %d, %d, %d, %d, %d)", pdev->id, pdev->rawvalue, arg, tm, pcurrent->id);
		CF_EXECUTE (db, query);
	}  // 명령도 없고 동작도 안하고 있다면 이력을 남길 이유가 없음 

	return CF_OK;
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
		CF_ERR_LOG ("GCG connect accept message generation failed. (%s)", tp3_geterrormsg (sc));
		return sc;
	}

	if (CF_OK == cf_db_open (db)) {
#ifdef USE_SQLITE3
		cf_db_busy_handler (db, cf_db_busy, "device information initialzation");
		cf_db_busy_timeout (db, BUSY_TIMEOUT);
		CF_EXECUTE ( db, "PRAGMA journal_mode = WAL");
#endif
	} else {
		CF_ERR_LOG ("database open error");
	}

	CF_VERBOSE (CF_VERBOSE_MID, "env notice [%d]", (preq->header).msgseq);
	if ((pres->header).rescode == TP3_RC_NORMAL) {
		gcgid = pconn->gcgid;
		tp3_parse_envmsg (preq, TP3_DEV_SENSOR, &n, id, value, _GOS_MAX_DEV);
		CF_VERBOSE (CF_VERBOSE_MID, "sensor count : %d", n);
		for (i = 0; i < n; i++) {
			nodeid = tp3_getnid (id[i]);
			devid = tp3_getdevid (id[i]);
			pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
			// by chj 2015.9.28
			if ( pdev == NULL ) {
				CF_ERR_LOG("not found sensor [%d][%d][%d]", gcgid, nodeid, devid);
//				CF_EXP_RETURN (pdev == NULL, TP3_SC_NOT_FOUND_DEVID, "not found sensor [%d][%d][%d]", gcgid, nodeid, devid);

			} else {
				nvalue = 0;
				if ( gos_convert_env (pdev, value[i], &nvalue) == CF_OK ) {
					CF_VERBOSE (CF_VERBOSE_HIGH, "sensor. devid : %d, raw : %d, nvalue : %f", devid, value[i], nvalue);
					gos_update_env (pdev, nvalue, value[i]);
					gos_test_rules (gos_get_ruleset (), pdev->deviceid, nvalue);
				} else {
					CF_ERR_LOG ("raw data converting failed. devid : %d, raw : %d", devid, value[i]);
				}
			}
		}

		tp3_parse_envmsg (preq, TP3_DEV_ACTUATOR, &n, id, value, _GOS_MAX_DEV);
		CF_VERBOSE (CF_VERBOSE_MID, "actuator count : %d", n);
		for (i = 0; i < n; i++) {
			nodeid = tp3_getnid (id[i]);
			devid = tp3_getdevid (id[i]);
			pdev = gos_find_device_by_tta (parg->pdevinfo, gcgid, nodeid, devid);
			// by chj 2015.9.28
			if ( pdev == NULL ) {
				CF_ERR_LOG("not found actuator [%d][%d][%d]", gcgid, nodeid, devid);
//				CF_EXP_RETURN (pdev == NULL, TP3_SC_NOT_FOUND_DEVID, "not found actuator [%d][%d][%d]", gcgid, nodeid, devid);
			} else {
				gos_parse_actuator_argument (value[i], &arg, &tm);
				CF_VERBOSE (CF_VERBOSE_HIGH, 
						"actuator. devid : %d, arg : %d, time : %d", devid, arg, tm);
				//debug
				if (arg > 2 || arg <= 0) {
					CF_VERBOSE (CF_VERBOSE_LOW, "strange arg : %d.", arg);
					tp3_printmsg (preq);

				} else {
					nvalue = 0;
					gos_update_env (pdev, nvalue, value[i]);
					gos_update_actuator_action (pdev, db);
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
		CF_ERR_LOG ("GCG connect accept message generation failed. (%s)", tp3_geterrormsg (sc));
		return sc;
	}

	if ((pres->header).rescode == TP3_RC_NORMAL) {
		gos_set_gcgid (parg->pconn, parg->handle, (pres->header).gcgid);
		CF_VERBOSE (CF_VERBOSE_HIGH, "connection was accepted. [gcgid : %d].", (pres->header).gcgid);

		//gos_init_control (gos_get_config ());
	} else {
		CF_VERBOSE (CF_VERBOSE_HIGH, "connection was not accepted [%d].", (pres->header).rescode);
		gos_set_remove_connection (parg->pconn, parg->handle);
	}
	return TP3_SC_NOERROR;
}

void
gos_timer_cb (uv_timer_t *handle) {
	static int ncnt = 0 ;
	long current = (long) uv_now(uv_default_loop());
	cf_db_t *db = &(gos_get_config ()->db);
#ifdef USE_SQLITE3
	int rc;
#endif
	
	// 정확한 시간 타임에 저장과 룰 검사를 하기위해 시간 체트하는 기능 추가 
	// by chj 2015.9.27
	struct tm *struct_time ;
	time_t t ;
	int cnt_seconds ;
	gos_config_t *pconfig ;

	CF_ERR_VOIDRETURN (cf_db_open (db), "database open failed.");

#ifdef USE_SQLITE3
	cf_db_busy_handler (db, cf_db_busy, "device information initialzation");
	cf_db_busy_timeout (db, BUSY_TIMEOUT);

	rc = cf_db_exec (db, "PRAGMA journal_mode=WAL", NULL, 0, NULL);
	CF_ERR_RETURN (rc != CF_OK, "database journal mode change failed.");
#endif	
	// 정확한 시간 타임에 저장과 룰 검사를 하기위해 시간 체트하는 기능 추가 
	// by chj 2015.9.27
	t = time(NULL);
	struct_time = localtime( &t);
	cnt_seconds = struct_time->tm_sec + struct_time->tm_min * 60 ;

	pconfig = gos_get_config ();
	if ( cnt_seconds % pconfig->nwrite == 0 && !(ncnt&0x01) ) {
		CF_VERBOSE (CF_VERBOSE_MID, "write device infomation to db.");
		gos_write_devinfo (gos_get_devinfo (), gos_get_config ());
		ncnt |= 0x01 ;
	}
	else if ( cnt_seconds % pconfig->nwrite != 0 ) {
		ncnt &= ~0x01 ;
	}
	if ( cnt_seconds % pconfig->nrule == 0  && !(ncnt&0x02)
			&& gos_check_auto_control (gos_get_server (), pconfig)) {
		CF_VERBOSE (CF_VERBOSE_MID, "evaluate rules.");
		gos_evaluate_rules (gos_get_ruleset (), pconfig);
		gos_reset_rulecondition (gos_get_ruleset ());
		ncnt |= 0x02 ;
	}
	else if ( cnt_seconds % pconfig->nrule != 0 ) {
		ncnt &= ~0x02 ;
	}
/*
	gos_config_t *pconfig = gos_get_config ();

	if (ncnt % pconfig->nwrite == 0) {
		CF_VERBOSE (CF_VERBOSE_MID, "write device infomation to db.");
		gos_write_devinfo (gos_get_devinfo (), gos_get_config ());
	}

	if (ncnt % pconfig->nrule == 0 
			&& gos_check_auto_control (gos_get_server (), pconfig)) {
		CF_VERBOSE (CF_VERBOSE_MID, "evaluate rules.");
		gos_evaluate_rules (gos_get_ruleset (), pconfig);
		gos_reset_rulecondition (gos_get_ruleset ());
	}
	ncnt ++;
*/

	gos_control (gos_get_devinfo (), pconfig, gos_get_conninfo ());

	gos_check_restart (gos_get_server (), pconfig);

	cf_db_close (db);

	CF_VERBOSE (CF_VERBOSE_HIGH, "timer %dth time : %ld(%d,%d), %ld ms.", ncnt, current,cnt_seconds%pconfig->nwrite, pconfig->nwrite,(long) uv_now(uv_default_loop()) - current);
}
