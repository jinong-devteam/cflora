#ifndef _GOS_VIRTUAL_SENSOR_H_
#define _GOS_VIRTUAL_SENSOR_H_

void *
gos_generate_vsensor_arg (gos_dev_t *pdev, char *configstr, double offset, cf_db_t *db);

void
gos_release_vsensor_arg (gos_cvt_vsensor_arg_t *parg);

double
gos_calclulate_vsensor_value (gos_cvt_vsensor_arg_t *parg, gos_devinfo_t *pdevinfo);

typedef enum {
	GOS_VSENS_DNWHUM = 1,
	GOS_VSENS_DNWDEW = 2,
	GOS_VSENS_DAILYACC = 3
} gos_vsenstype_t;

#define _GOS_VSENS_MAX 3

typedef struct {
	int dry_device_id;
	int wet_device_id;
} gos_dnw_config_t;

#define _DNW_DRY	"dry bulb"
#define _DNW_WET	"wet bulb"
//#define _DNW_DRY	"건구온도"
//#define _DNW_WET	"습구온도"
#define _DAY_ACC	"daily accum"

typedef struct {
    int device_id;
    double previous;
    time_t last;
    int today;
} gos_dayacc_config_t;

#endif
