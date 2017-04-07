
#ifndef _GOS_CONNECTION_H_
#define _GOS_CONNECTION_H_

/** 단일 접속에 대한 데이터 */
typedef struct {
	int gcgid;           	///< GCG ID for TTA connection
	int setremove;			///< 삭제예정표시

	uv_stream_t *handle;  	///< 스트림 핸들
	cf_msgbuf_t readbuf; 	///< 통신으로 받은 메세지의 버퍼
	cf_msgbuf_t writebuf; 	///< 통신으로 보낼 메세지의 버퍼
} gos_conn_t;

/** 접속정보 구조체 (단일 GCG에서는 큰 의미는 없음) */
typedef struct {
	int len;				///< 실제 접속의 개수
	int size;				///< 접속을 받기위해 준비된 크기 
	gos_conn_t *pconn;		///< 단일 접속정보 구조체의 배열을 저장하기 위한 포인터
} gos_conninfo_t;

/**
 * 접속정보 구조체를 초기화한다.
 * @param 접속정보 구조체의 포인터
 */
void
gos_init_conninfo (gos_conninfo_t *pconn);

/**
 * 접속정보 구조체를 해제한다.
 * @param pconn 접속정보 구조체의 포인터
 */
void
gos_release_conninfo (gos_conninfo_t *pconn);

/**
 * 접속정보를 추가한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_add_connection (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * 접속삭제 예정표시를 한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 */
void
gos_set_remove_connection (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * 접속삭제 예정표시를 가져온다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 예정표시값.없는 핸들이라면 0
 */
int
gos_get_remove_connection (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * gcgid 을 이용해서 단일접속정보를 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param gcgid GCG 식별자
 * @return 접속정보 포인터, 못찾으면 NULL
 */
gos_conn_t *
gos_find_connection (gos_conninfo_t *pconn, int gcgid);

/**
 * handle을 이용해서 단일접속정보를 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 접속정보 포인터, 못찾으면 NULL
 */
gos_conn_t *
gos_find_connection_by_handle (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * 접속정보에 GCG 식별자를 설정한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @param gcgid GCG 식별자
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_set_gcgid (gos_conninfo_t *pconn, uv_stream_t *handle, int gcgid);

/**
 * handle을 이용해서 읽기용 메세지 버퍼를 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 읽기용 메세지 버퍼의 포인터
 */
cf_msgbuf_t *
gos_get_readmsgbuf  (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * handle을 이용해서 쓰기용 메세지 버퍼를 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 쓰기용 메세지 버퍼의 포인터
 */
cf_msgbuf_t *
gos_get_writemsgbuf  (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * 접속정보 하나를 제거한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_remove_connection (gos_conninfo_t *pconn, uv_stream_t *handle);

/**
 * 접속정보를 해제한다.
 * @param pconn 접속정보 구조체의 포인터
 */
void
gos_release_conninfo (gos_conninfo_t *pconn);

/**
 * 버퍼에 있는 TTA P3 메세지프레임을 파싱한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param pframe TTA P3 메세지프레임
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_parseframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe);

/**
 * TTA P3 메세지프레임을 버퍼에 기록한다. 
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param pframe TTA P3 메세지프레임
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_writeframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe);


#endif
