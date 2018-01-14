/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_config.h
 * \brief GOS 설정관련 해더 파일. 기존 코드를 수정했음.
 */

#ifndef _GOS_CONFIG_H_
#define _GOS_CONFIG_H_

#include <cf.h>

#define GOS_DEFAULT_DB          "127.0.0.1;farmos;root;farmos;"      // DB 접속을 위한 문자열
#define GOS_DEFAULT_PORT		5972	// GCG 접속용 포트
#define GOS_DEFAULT_TIMER		500		// every 500 ms
#define GOS_DEFAULT_NWRITE		60		// every 30 sec
#define GOS_DEFAULT_NUPDATE     20		// every 10 sec
#define GOS_DEFAULT_NRULE		240		// every 2 min

#define GOS_DEFAULT_GOSID		123
#define GOS_DEFAULT_NUMOFGCG	1
#define GOS_DEFAULT_GCGID		1

/** GOS configuration */
typedef struct {
	int port;		///< GCG 접속을 위한 포트 
	cf_db_t db;		///< 데이터 베이스 핸들러
	int timer;		///< 내부 타이머 주기 
	int nupdate;	///< nupdate 회수만큼의 내부 타이머 마다 최신환경정보를 디비에 업데이트
	int nwrite;		///< nwrite 회수만큼의 내부 타이머 마다 환경정보를 디비에 기록 
	int nrule;		///< nrule 회수만큼의 내부 타이머 마다 자동제어룰을 적용
    double longitude; ///< 농장의 경도
    double latitude;  ///< 농장의 위도

	int gosid;		///< GOS ID 
	int numofgcg;	///< 접속할 GCG의 개수
	int *gcgids;	///< 접속할 GCG 식별자들
} gos_config_t;

/**
 * 디폴트 설정을 세팅한다.
 * @param pconfig 설정이 저장될 구조체의 포인터
 * @return 에러라면 ERR, 정상완료라면 OK
 */
ret_t
gos_default_config (gos_config_t *pconfig);

/**
 * 설정파일에서 설정을 읽는다. 
 * @param pconfig 설정이 저장될 구조체의 포인터
 * @param conffile 설정 파일의 경로
 * @return 에러라면 ERR, 정상완료라면 OK
 */
ret_t
gos_read_config (gos_config_t *pconfig, char *conffile);

/**
 * 설정 구조체를 해제한다.
 * @param pconfig 설정 구조체의 포인터
 */
void
gos_release_config (gos_config_t *pconfig);

#endif
