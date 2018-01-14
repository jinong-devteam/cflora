/**
 * Copyright © 2016-2017 JiNong Inc. 
 * All Rights Reserved.
 *
 * \file gos_time.h
 * \brief 일출일몰시를 다루는 해더파일
 */

#ifndef _GOS_TIME_H_
#define _GOS_TIME_H_

#include <time.h>

typedef struct {
    double longitude;
    double latitude;
    struct tm today;

    int sunrisetime;
    int sunsettime;
    int current;
} gos_time_t;

void
gos_inittime (double longtitude, double latitude);

int
gos_gettime_timer ();

int
gos_gettime_currentsec ();

int
gos_gettime_sunrisesec ();

int
gos_gettime_sunsetsec ();


#endif
