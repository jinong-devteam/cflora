
#ifndef _GCG_CONFIG_H_
#define _GCG_CONFIG_H_

#define GCG_DEFAULT_GOS_PORT	5972	// GOS 접속용 포트
#define GCG_DEFAULT_GCG_PORT	5000	// NODE 접속용 포트

#define GCG_DEFAULT_GCGID		1		// 디폴트 GCG ID
#define GCG_DEFAULT_GOSID		100		// 디폴트 GOS ID

#define GCG_DEFAULT_GOS_IP		"127.0.0.1"	// 디폴트 GOS IP
#define GCG_DEFAULT_GCG_IP		"127.0.0.1"	// 디폴트 GCG IP

//#define GCG_DEFAULT_SNODE_COUNT	3
//#define GCG_DEFAULT_ANODE_COUNT	1

#define GCG_DEFAULT_TIMER		1000	// 디폴트 TIMER

/** GCG configuration */
typedef struct {
	char gcgip[_GCG_BUF_LEN];	///< 자체 IP
	char gosip[_GCG_BUF_LEN];	///< GOS 접속을 위한 IP
	int gosport;		///< GOS 접속을 위한 포트 
	int gcgport;		///< NODE 접속을 위한 포트 
	int gcgid;		///< GCG ID 
	int gosid;		///< GOS ID 
	int timer;		///< GCG TIMER

	//int snodecnt;		///< Sensor Node Count
	//int anodecnt;		///< Actuator Node Count
} gcg_config_t;

/**
 * 디폴트 설정을 세팅한다.
 * @param pconfig 설정이 저장될 구조체의 포인터
 * @return 에러라면 GCG_ERR, 정상완료라면 GCG_OK
 */
cf_ret_t
gcg_default_config (gcg_config_t *pconfig);

/**
 * 설정파일에서 설정을 읽는다. 
 * @param pconfig 설정이 저장될 구조체의 포인터
 * @param conffile 설정 파일의 경로
 * @return 에러라면 GCG_ERR, 정상완료라면 GCG_OK
 */
cf_ret_t
gcg_read_config (gcg_config_t *pconfig, char *conffile);

/**
 * 설정 구조체를 해제한다.
 * @param pconfig 설정 구조체의 포인터
 */
void
gcg_release_config (gcg_config_t *pconfig);

#endif
