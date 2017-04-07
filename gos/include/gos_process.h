
#ifndef _GOS_PROCESS_H_
#define _GOS_PROCESS_H_

/** TTA P3 메세지 처리 콜백함수에 사용하기 위한 사용자 정의 인자 */
typedef struct {
	gos_conninfo_t *pconn;		///< 접속 정보
	gos_devinfo_t *pdevinfo;	///< 디바이스 정보
	uv_stream_t *handle;		///< 스트림 핸들러
} gos_tp3arg_t;

/**
 * 사용자 정의 인자를 세팅한다.
 * @param parg 사용자 정의 인자의 포인터
 * @param handel 스트림 핸들러
 */
void
gos_set_tp3arg (gos_tp3arg_t *parg, uv_stream_t *handle);


/**
 * 통합제어기에서 전송된 상태정보메세지를 처리한다.
 * @param preq 통합제어기에서 전송된 상태정보 메세지
 * @param pres 메세지에 대한 응답
 * @param data 사용자 정의 데이터
 * @return 상태코드.
 */
tp3_stat_t
gos_statmsg_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data);

/**
 * 통합제어기에서 전송된 환경정보메세지를 처리한다.
 * @param preq 통합제어기에서 전송된 환경정보 메세지
 * @param pres 메세지에 대한 응답
 * @param data 사용자 정의 데이터
 * @return 상태코드.
 */
tp3_stat_t
gos_envmsg_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data);

/**
 * 통합제어기에서 전송된 연결시도메세지를 처리한다.
 * @param preq 통합제어기에서 전송된 연결시도 메세지
 * @param pres 메세지에 대한 응답
 * @param data 사용자 정의 데이터
 * @return 상태코드.
 */
tp3_stat_t
gos_connmsg_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data);

/**
 * 타이머를 처리한다.
 * @param handle 타이머 핸들러
 */
void
gos_timer_cb (uv_timer_t *handle);

#endif
