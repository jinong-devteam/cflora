#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cf.h>

#include <gos_base.h>
#include <gos_config.h>
#include <gos_rule.h>

double
gos_get_interpolated (gos_rarg_t *parg, int span, int ndiff) {
	double ratio = (parg->arg2 - parg->arg1) / span;
	return parg->arg1 + ratio * ndiff / span;
}

int
gos_rop_gt (double nvalue, int ndiff, void *arg, int span) {
	if (nvalue > gos_get_interpolated ((gos_rarg_t *)arg, span, ndiff))
		return TRUE;
	return FALSE;
}

int
gos_rop_lt (double nvalue, int ndiff, void *arg, int span) {
	if (nvalue < gos_get_interpolated ((gos_rarg_t *)arg, span, ndiff))
		return TRUE;
	return FALSE;
}

int
gos_rop_eq (double nvalue, int ndiff, void *arg, int span) {
	if (nvalue == gos_get_interpolated ((gos_rarg_t *)arg, span, ndiff))
		return TRUE;
	return FALSE;
}

int
gos_rop_gte (double nvalue, int ndiff, void *arg, int span) {
	if (nvalue >= gos_get_interpolated ((gos_rarg_t *)arg, span, ndiff))
		return TRUE;
	return FALSE;
}

int
gos_rop_lte (double nvalue, int ndiff, void *arg, int span) {
	if (nvalue <= gos_get_interpolated ((gos_rarg_t *)arg, span, ndiff))
		return TRUE;
	return FALSE;
}

int
gos_rop_neq (double nvalue, int ndiff, void *arg, int span) {
	if (nvalue != gos_get_interpolated ((gos_rarg_t *)arg, span, ndiff))
		return TRUE;
	return FALSE;
}

static gos_rexp_func _funcs[CF_ROP_MAX] = {
	gos_rop_gt, gos_rop_lt, gos_rop_eq,
	gos_rop_gte, gos_rop_lte, gos_rop_neq
};


cf_ret_t
gos_init_rules (gos_ruleset_t *pruleset, gos_config_t *pconfig) {
	cf_ret_t rc;

	rc = gos_load_rules (pruleset, &(pconfig->db));
	if (rc != CF_OK) {
		CF_ERR_LOG ("rule load failed");
	}

	return rc; 
}

cf_ret_t
gos_load_rules_acts (gos_rule_t *prule, int ruleid, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char **result;
	char *errmsg;
	int rows, columns, rc, i;

	sprintf (query, "select istrue, actuator_id, argument, workingtime from gos_control_rule_action where id = %d order by istrue asc", ruleid);

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (get rule action) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	prule->pactions = (gos_ract_t *) CF_MALLOC (sizeof (gos_ract_t) * rows);
	if (prule->pactions == NULL) {
		CF_ERR_LOG ("memory allocation for rule action infomation failed.");
		cf_db_free_table (result);
		return CF_ERR;
	}
	prule->numofact = rows;
	prule->actsidx = -1;

	for (i = 1; i <= rows; i++) {
		gos_ract_t *pact = prule->pactions + i - 1;
		int idx = i * columns;

		if (prule->actsidx < 0 && atoi(result[idx]) > 0)
			prule->actsidx = i - 1;

		pact->actuatorid = atoi (result [idx + 1]);
		pact->arg = atoi (result[idx + 2]);
		pact->wtime = atoi (result[idx + 3]);
	}
	cf_db_free_table (result);
	return CF_OK;
}

gos_rop_t
gos_convert_ruleop (char *opstr) {
	int i;
	static char *opstrs[CF_ROP_MAX] = {
		"<", ">", "=", "<=", ">=", "!="
	};

	for (i = 0; i < CF_ROP_MAX; i++) {
		if (strcmp (opstrs[i], opstr) == 0)
			return i;
	}
	return 0;
}

cf_ret_t
gos_load_rules_exps (gos_rule_t *prule, int ruleid, cf_db_t *db) {
	char query[_GOS_BUF_LEN];
	char **result;
	char *errmsg;
	int rows, columns, rc, i, nspan;
	int from, to;
	gos_rtimespan_t *pts;

	sprintf (query, "select count(*) from (select * from gos_control_rule_condition where id = %d group by ftime, ttime) as COND", ruleid);
	CF_EXECUTE_GET_INT(db, query, &nspan);

	prule->ptimes = (gos_rtimespan_t *) CF_MALLOC (sizeof (gos_rtimespan_t) * nspan);
	CF_ERR_RETURN (prule->ptimes == NULL, "memory allocation for rule timespan failed.");
	prule->numofts = nspan;

	sprintf (query, "select ftime, ttime, operator, sensor_id, fvalue, tvalue from gos_control_rule_condition where id = %d order by ftime, ttime", ruleid);

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (get rule expression) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	prule->psatisfied = (int *) CF_MALLOC (sizeof (int) * rows);
	if (prule->psatisfied == NULL) {
		CF_ERR_LOG ("memory allocation for rule expression flag failed.");
		CF_FREE (prule->ptimes);
		cf_db_free_table (result);
		return CF_ERR;
	}
	memset (prule->psatisfied, 0, sizeof (int) * rows);

	prule->pexps = (gos_rexp_t *) CF_MALLOC (sizeof (gos_rexp_t) * rows);
	if (prule->pexps == NULL) {
		CF_ERR_LOG ("memory allocation for rule expressions failed.");
		CF_FREE (prule->ptimes);
		CF_FREE (prule->psatisfied);
		cf_db_free_table (result);
		return CF_ERR;
	}
	prule->numofexp = rows;

	nspan = 0;
	pts = prule->ptimes + nspan;
	pts->from = -1;
	pts->to = -1;
	for (i = 1; i <= rows; i++) {
		gos_rexp_t *pexp = prule->pexps + i - 1;
		int idx = i * columns;
		from = atoi (result[idx]);
		to = atoi (result[idx + 1]);

		if (pts->from != from && pts->to != to) {
			pts = prule->ptimes + nspan;
			pts->from = from;
			pts->to = to;
			pts->exp_start = i - 1;
			pts->exp_end = i;
			nspan++;
		} else {
			pts->exp_end += 1;
		}

		pexp->op = gos_convert_ruleop (result[idx + 2]);
		pexp->sensorid = atoi (result[idx + 3]);
		(pexp->arg).arg1 = atof (result[idx + 4]);
		(pexp->arg).arg2 = atof (result[idx + 5]);

	}
	cf_db_free_table (result);
	return CF_OK;
}

cf_ret_t
gos_load_rules (gos_ruleset_t *pruleset, cf_db_t *db) {
	char *query = "select id, priority, period from gos_control_rule order by priority asc";
	char **result;
	char *errmsg;
	int rows, columns, rc, i;

	rc = cf_db_get_table (db, query, &result, &rows, &columns, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database query execution (get rules) failed. %s", errmsg);
		cf_db_free(errmsg);
		return CF_ERR;
	}

	CF_VERBOSE (CF_VERBOSE_HIGH, "There are %d rules.", rows);

	pruleset->prules = (gos_rule_t *) CF_MALLOC (sizeof (gos_rule_t) * rows);
	if (pruleset->prules == NULL) {
		CF_ERR_LOG ("memory allocation for rule infomation failed.");
		cf_db_free_table (result);
	}
	pruleset->numofrules = rows;

	for (i = 1; i <= rows; i++) {
		gos_rule_t *prule = pruleset->prules + i - 1;
		int idx = i * columns;
		int id = atoi (result [idx]);
		prule->id = id;
		prule->priority = atoi (result [idx + 1]);
		prule->period = atoi (result [idx + 2]);
		prule->lastexec = 0;

		rc = gos_load_rules_acts (prule, id, db);
		if (rc != CF_OK) {
			prule->status = 0;
			prule->pactions = NULL;
			continue;
		}

		rc = gos_load_rules_exps (prule, id, db);
		if (rc != CF_OK) {
			prule->status = 0;
			prule->ptimes = NULL;
			prule->pexps = NULL;
			prule->psatisfied = NULL;
		} else {
			prule->status = 1;
		}
		CF_VERBOSE (CF_VERBOSE_HIGH, "A rule [%d] set up.", id);
	}
	cf_db_free_table (result);

	return CF_OK;
}

void
gos_release_rule (gos_rule_t *prule) {
	CF_FREE (prule->pactions);
	CF_FREE (prule->ptimes);
	CF_FREE (prule->pexps);
	CF_FREE (prule->psatisfied);
}

void
gos_release_rules (gos_ruleset_t *pruleset) {
	int i;
	for (i = 0; i < pruleset->numofrules; i++) {
		gos_release_rule (pruleset->prules + i);
	}
	CF_FREE (pruleset->prules);
}

void
gos_reset_rulecondition (gos_ruleset_t *pruleset) {
	int i;
	gos_rule_t *prule;

	for (i = 0; i < pruleset->numofrules; i++) {
		prule = pruleset->prules + i;
		memset (prule->psatisfied, 0, sizeof(int) * prule->numofexp);
	}
}

int
gos_get_secofday () {
	time_t tm = time (NULL);
	struct tm *ptm = localtime (&tm);

	return ptm->tm_sec + ptm->tm_min * 60 + ptm->tm_hour * 60 * 60;
}

void
gos_test_rule (gos_rule_t *prule, int now, int sensorid, double nvalue) {
	int j, k;
	gos_rtimespan_t *ptime;
	gos_rexp_t *pexp;

	for (j = 0; j< prule->numofts; j++) {
		ptime = prule->ptimes + j;

		if (now < ptime->from || now >= ptime->to) 
			continue;

		for (k = ptime->exp_start; k < ptime->exp_end; k++) {
			pexp = prule->pexps + k;
			if (sensorid != pexp->sensorid)
				continue;

			(prule->psatisfied)[k] = _funcs[pexp->op]
				(nvalue, now - ptime->from, (void *)&(pexp->arg), ptime->to - ptime->from);
			CF_VERBOSE (CF_VERBOSE_HIGH, "rule[%d] sensor[%d] value[%f] : [%d].", prule->id, sensorid, nvalue, (prule->psatisfied)[k]);
		}
	}
}

void
gos_test_rules (gos_ruleset_t *pruleset, int sensorid, double nvalue) {
	int i, now;

	now = gos_get_secofday ();

	for (i = 0; i < pruleset->numofrules; i++) {
		gos_test_rule (pruleset->prules + i, now, sensorid, nvalue);
	}
}

cf_ret_t
gos_execute_action (gos_ract_t *pact, int num, int ruleid, cf_db_t *db) {
	int i;
	char query[_GOS_BUF_LEN];
	char *errmsg = NULL;
	int rc;
	char date_now[20]; // YYYY-MM-DD HH:MM:ss
	cf_db_timestring( date_now, sizeof(date_now), (int)time(NULL));

	CF_BEGIN_TRANSACTION (db);
	for (i = 0; i < num; i++) {
		sprintf (query, "insert into gos_control "
				"(exectime, device_id, argument, ctrltype, ctrlid, worktime) "
				"values ('%s', %d, %d, 'auto-control', %d, %d)", 
				date_now, pact[i].actuatorid, pact[i].arg, ruleid, pact[i].wtime);

		rc = cf_db_exec(db, query, NULL, 0, &errmsg);
		if (rc != CF_OK) {
			CF_ERR_LOG ("database error : %s", errmsg);
			cf_db_free (errmsg);
			CF_ROLLBACK (db);
			return CF_ERR;
		}

		CF_VERBOSE (CF_VERBOSE_MID, "auto-control actuator[%d] arg[%d] time[%d].", pact[i].actuatorid, pact[i].arg, pact[i].wtime);
	}
	CF_END_TRANSACTION (db);
	return CF_OK;
}

cf_ret_t
gos_evaluate_rules (gos_ruleset_t *pruleset, gos_config_t *pconfig) {
	int i, j, now, k, m, stop = 0;
	gos_rule_t *prule;
	gos_rtimespan_t *ptime;
	cf_db_t *db = &(pconfig->db);
	cf_ret_t ret;

	now = gos_get_secofday ();

	for (i = 0; i < pruleset->numofrules; i++) {
		prule = pruleset->prules + i;
		if (prule->lastexec + prule->period >= now)
			continue;

		for (j = 0; j< prule->numofts; j++) {
			ptime = prule->ptimes + j;
			if (now >= ptime->from && now < ptime->to) {
				for (k = ptime->exp_start, m = 0; k < ptime->exp_end; k++) {
					m += (prule->psatisfied)[k];
				}

				if (m == ptime->exp_end - ptime->exp_start) {
					// true
					ret = gos_execute_action (prule->pactions + prule->actsidx, prule->numofact - prule->actsidx, prule->id, db);
					CF_VERBOSE (CF_VERBOSE_HIGH, "rule[%d] was matched - stop applying other rules.", prule->id);
					stop = 1;
				} else {
					// false
					ret = gos_execute_action (prule->pactions, prule->actsidx, prule->id, db);
					CF_VERBOSE (CF_VERBOSE_HIGH, "rule[%d] was not matched.", prule->id);
				}

				if (CF_OK != ret) {
					CF_ERR_LOG ("to apply a rule [%d] was failed.", prule->id);
				} else {
					prule->lastexec = gos_get_secofday ();
				}
			}
			if (stop)
				break;
		}
	}

	return CF_OK;
}

