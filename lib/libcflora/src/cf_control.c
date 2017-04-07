#include <stdio.h>
#include <stdlib.h>

#include <cf_base.h>
#include <cf_db_mysql.h>

#define _CF_MAX_QUERYLEN	1024
#define _CF_MAX_DEVICELEN	30

cf_ret_t
cf_get_environment (cf_db_t *pdb, int sensorid, double *value, int ntime) {
	char query[_CF_MAX_QUERYLEN];
	int rc, rows, columns;
	char **result;
	char *errmsg = NULL;

	sprintf (query, "select nvalue from gos_environment where device_id = %d and obstime >= datetime('now', 'localtime', '-%d seconds') order by obstime desc limit 1", sensorid, ntime);

	rc = cf_db_get_table (pdb, query, &result, &rows, &columns, &errmsg);

	if (CF_OK == rc) {
		*value = atof (result[1]);
		cf_db_free_table (result);
		return CF_OK;
	} else {
		CF_ERR_LOG ("database error : %s", errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}
}

int
cf_get_environments (cf_db_t *pdb, int num, int *sensorid, int *rsensor, double *values, int ntime) {
	char query[_CF_MAX_QUERYLEN];
	int i, idx, rc, rows, columns;
	char **result;
	char *errmsg = NULL;

	CF_ERR_RETURN (num > _CF_MAX_DEVICELEN, "too many sensors.");
	CF_ERR_RETURN (num < 1, "at least 1 sensor id is needed.");

	idx = sprintf (query, 
		"select device_id, avg(nvalue) from gos_environment where device_id in (%d", 
		sensorid[0]);
	for (i = 1; i < num; i++) {
		idx += sprintf (query + idx, ", %d", sensorid[i]);
	}
	sprintf (query + idx, 
		") and obstime >= datetime('now', 'localtime', '-%d seconds') group by device_id", 
		ntime);

	rc = cf_db_get_table (pdb, query, &result, &rows, &columns, &errmsg);
	if (CF_OK == rc) {
		for (i = 1; i <= rows; i++) {
			rsensor[i - 1] = (int)atof (result[i * columns]);
			values[i - 1] = atof (result[i * columns + 1]);
		}
		i--;
		while (i < num) {
			rsensor[i] = -1;
			i++;
		}
		cf_db_free_table (result);
		return rows;

	} else {
		CF_ERR_LOG ("database error : %s", errmsg);
		cf_db_free (errmsg);
		return -1;
	}
}

cf_ret_t
cf_execute_command (cf_db_t *pdb, int actuatorid, int argument, int wtime) {
	char query[_CF_MAX_QUERYLEN];
	char *errmsg = NULL;
	int rc;

	sprintf (query, "insert into gos_control (exectime, device_id, argument, ctrltype, ctrlid, worktime) "
		"values (datetime('now','localtime'), %d, %d, 'auto-cflora', 0, datetime('now','localtime', '+%d seconds'))", 
		actuatorid, argument, wtime);

	rc = cf_db_exec(pdb, query, NULL, 0, &errmsg);
	if (rc != CF_OK) {
		CF_ERR_LOG ("database error : %s", errmsg);
		cf_db_free (errmsg);
		return CF_ERR;
	}
	return CF_OK;
}

cf_ret_t
cf_execute_commands (cf_db_t *pdb, int num, int *actuatorid, int *argument, int *wtime) {
	int i;
	cf_ret_t ret;

	CF_BEGIN_TRANSACTION (pdb);
	for (i = 0; i < num; i++) {
		ret = cf_execute_command (pdb, actuatorid[i], argument[i], wtime[i]);
		if (ret != CF_OK) {
			CF_ROLLBACK (pdb);
			return ret;
		}
	}
	CF_END_TRANSACTION (pdb);

	return ret;
}
