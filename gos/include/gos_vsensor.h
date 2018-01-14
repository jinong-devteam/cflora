/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_vsensor.h
 * \brief GOS 가상센서관련 해더 파일. 기존 코드를 수정했음.
 */

#ifndef _GOS_VIRTUAL_SENSOR_H_
#define _GOS_VIRTUAL_SENSOR_H_

void *
gos_generate_vsensor_arg (gos_dev_t *pdev, char *configstr, double offset, cf_db_t *db);

void
gos_release_vsensor_arg (gos_cvt_vsensor_arg_t *parg);

double
gos_calclulate_vsensor_value (gos_cvt_vsensor_arg_t *parg, gos_devinfo_t *pdevinfo);

typedef struct {
    int device_id;
} gos_vsensor_config_t;

typedef struct {
    int first_id;
    int second_id;
} gos_vsensor_twodev_config_t;

typedef struct {
    int dry_device_id;
    int wet_device_id;
} gos_dnw_config_t;

typedef struct {
    int device_id;
    double previous;
    time_t last;
    int today;
    double ratio;
} gos_dayacc_config_t;

typedef struct {
    int device_id;
    int number;
    int isinit;
    double previous;
} gos_movavg_config_t;

typedef struct {
    int device_id;
    int angle;
} gos_windside_config_t;

typedef enum {
    OP_PLUS = 1,
    OP_MINUS = 2,
    OP_MULTIPLY = 3,
    OP_DIVIDE = 4,
    OP_MODULUS = 5,
    OP_ABSMINUS = 6
} gos_op_t;

typedef struct {
    int fop_device_id;
    int sop_device_id;
    gos_op_t optype;
} gos_opvsen_config_t;

typedef struct {
    char fname[_GOS_BUF_LEN];
    FILE *fp;
} gos_filevsen_config_t;

typedef struct {
    key_t shmkey;
    int shmid;
    void *shm;
} gos_shmvsen_config_t;

#endif
