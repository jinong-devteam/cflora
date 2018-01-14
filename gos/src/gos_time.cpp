/**
 * Copyright © 2016-2017 JiNong Inc. 
 * All Rights Reserved.
 *
 * \file gos_time.cpp
 * \brief 일출일몰시를 다루는 파일
 */


#include <stdio.h>
#include <time.h>
#include <math.h>
#include <cf.h>
#include "gos_base.h"
#include "gos_time.h"


void 
settoday (gos_time_t *gtime, struct tm *ptm) {
    (gtime->today).tm_year = 1900 + ptm->tm_year;
    (gtime->today).tm_mon = 1 + ptm->tm_mon;
    (gtime->today).tm_mday = ptm->tm_mday;
    (gtime->today).tm_yday = ptm->tm_yday;
}

double 
getgamma(gos_time_t *gtime) {
    return (2.0 * M_PI / 365.0) * ((gtime->today).tm_yday);
}

double 
getgamma2(gos_time_t *gtime, int hour) {
    return (2.0 * M_PI / 365.0) * ((gtime->today).tm_yday + (hour/24.0));
}

double 
getequationtime (double gamma) {
    return (229.18 * (0.000075 + 0.001868 * cos(gamma) - 0.032077 * sin(gamma)
                - 0.014615 * cos(2 * gamma) - 0.040849 * sin(2 * gamma)));
}

double 
getsolardeclination (double gamma) {
    return (0.006918 - 0.399912 * cos(gamma) + 0.070257 * sin(gamma)
            - 0.006758 * cos(2 * gamma) + 0.000907 * sin(2 * gamma));
}

double 
degtorad (double deg) {
    return (M_PI * deg/ 180.0);
}

double 
radtodeg (double rad) {
    return (180 * rad / M_PI);
}

double 
gethourangle (double latitude, double declination, int time) {
    double latrad = degtorad (latitude);
    double hourangle = acos(cos(degtorad (90.833)) / (cos(latrad) * cos(declination))
            - tan(latrad) * tan(declination));

    if (time==1) {
        return hourangle;
    } else if (time==0){
        return -hourangle;
    }
    return 0;
}

double 
gettime (gos_time_t *gtime, double gamma, int isrise) {
    double eqtime = getequationtime (gamma);
    double declination = getsolardeclination (gamma);
    double hourangle = gethourangle (gtime->latitude, declination, isrise);
    double delta = gtime->longitude - radtodeg (hourangle);
    return 720.0 + 4.0 * delta - eqtime;
}

void 
setsuntime (gos_time_t *gtime) {
    double dtime = gettime (gtime, getgamma (gtime), 1);
    dtime = gettime (gtime, getgamma2 (gtime, (int) (dtime / 60.0)), 1) + 540;
    gtime->sunrisetime = (int)(dtime * 60);
    LOG(INFO) << "sunrise time " << gtime->sunrisetime;
    
    dtime = gettime (gtime, getgamma (gtime), 0);
    dtime = gettime (gtime, getgamma2 (gtime, (int) (dtime / 60.0)), 0) + 540;
    gtime->sunsettime = (int)(dtime * 60);
    LOG(INFO) <<  "sunset time " << gtime->sunsettime;
}

static gos_time_t suntime;

void 
gos_inittime (double longitude, double latitude) {
    time_t now;
    struct tm tms;

    suntime.longitude = longitude;
    suntime.latitude = latitude;
    now = time (NULL);
    localtime_r (&now, &tms);
    settoday (&suntime, &tms);
    setsuntime (&suntime);
}

int
gos_gettime_timer () {
    struct tm tms;
    time_t now, today;
    
    now = time (NULL);

    localtime_r (&now, &tms);
    tms.tm_hour = 0;
    tms.tm_min = 0;
    tms.tm_sec = 0;
    today = mktime (&tms);

    if (tms.tm_mday != suntime.today.tm_mday) { // recalculate
        settoday (&suntime, &tms);
        setsuntime (&suntime);
    }

    suntime.current = now - today;

    return suntime.current;
}

int 
gos_gettime_currentsec () {
    return suntime.current;
}

int 
gos_gettime_sunrisesec () {
    return suntime.current - suntime.sunrisetime;
}

int
gos_gettime_sunsetsec () {
    return suntime.current - suntime.sunsettime;
}
