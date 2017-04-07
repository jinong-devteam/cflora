
#ifndef _GOS_SERVER_H_
#define _GOS_SERVER_H_

/** GOS 운영을 위한 서버정보 구조체 */
typedef struct {
	uv_timer_t timer;   	///< 타이머 처리를 위한 내부 변수
	uv_tcp_t ttaserver; 	///< TTA P3 통신을 위한 내부 변수
	int restart;      		///< 서버 재시작을 위한 내부 변수 
	int control;      		///< 자동제어/수동제어를 위한 내부 변수 
} gos_server_t;

/**
 * 서버 정보를 초기화 한다.
 * @param pserver 서버정보
 */
void
gos_init_server (gos_server_t *pserver);

/**
 * 서버 정보를 해제한다.
 * @param pserver 서버정보
 */
void
gos_release_server (gos_server_t *pserver);

/**
 * 서버를 재시작 여부를 확인한다.
 * @param pserver 서버정보
 * @return 재시작이 필요하다면 true
 */
int
gos_get_restart (gos_server_t *pserver);

/**
 * 서버 재시작에 대한 플래그를 세팅한다.
 * @param pserver 서버정보
 * @param restart 재시작여부
 */
void
gos_set_restart (gos_server_t *pserver, int restart);

/**
 * 디비로 부터 서버를 재시작 할지 확인하고, 재시작이 필요하다면 이벤트 루프를 종료한다.
 * @param pserver 서버정보
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_check_restart (gos_server_t *pserver, gos_config_t *pconfig);

/**
 * 디비로 부터 서버를 자동제어 여부를 확인한다. 
 * @param pserver 서버정보
 * @param pconfig 설정 구조체의 포인터
 * @return 0이라면 수동제어, 나머진 자동제어
 */
int
gos_check_auto_control (gos_server_t *pserver, gos_config_t *pconfig);

/**
 * 현재 설정정보를 가져온다.
 * @return 설정 구조체의 포인터
 */
gos_config_t *
gos_get_config ();

/**
 * 현재 서버정보를 가져온다.
 * @return 서버정보 구조체의 포인터
 */
gos_server_t *
gos_get_server ();

/**
 * 현재 디바이스정보를 가져온다.
 * @return 디바이스정보 구조체의 포인터
 */
gos_devinfo_t *
gos_get_devinfo ();

/**
 * 현재 접속정보를 가져온다.
 * @return 접속정보 구조체의 포인터
 */
gos_conninfo_t *
gos_get_conninfo ();

/**
 * 설정된 자동제어룰 정보를 가져온다.
 * @return 룰셋 구조체의 포인터
 */
gos_ruleset_t *
gos_get_ruleset ();

/**
 * 메세지를 전송한다.
 * @param handle 스트림 핸들러
 * @param pframe 메세지 프레임
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_send_message (uv_stream_t *handle, tp3_frame_t *pframe);

/**
 * 메세지를 처리한다.
 * @param handle 스트림 핸들러
 * @param pframe 메세지 프레임
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_process_message (uv_stream_t *handle, tp3_frame_t *pframe);

/**
 * GOS 를 초기화 한다.
 * @param conffile 설정 파일의 경로
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_initialize (char *conffile);

/**
 * GOS 내부 정보를 해제 한다.
 */
void
gos_finalize ();

/**
 * Timer 서버를 시작한다.
 * @param pgos 서버 정보 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_timer_start (gos_server_t *pgos, gos_config_t *pconfig);

/**
 * TTA 서버를 시작한다.
 * @param pgos 서버 정보 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_ttaserver_start (gos_server_t *pgos, gos_config_t *pconfig);

#endif
