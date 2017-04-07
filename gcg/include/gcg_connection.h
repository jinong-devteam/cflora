
#ifndef _GCG_CONNECTION_H_
#define _GCG_CONNECTION_H_

#define GCG_MAXNODE_IN_HANDLE	8

/** 접속정보 구조체 */
typedef struct {
	int nodeid[GCG_MAXNODE_IN_HANDLE];	///< Node ID
	uv_stream_t *handle;				///< Node 스트림 핸들
	TTA12PacketParsing parser;			///< Node 에서 받은 메세지 처리용
} gcg_nodeconn_t;
	
typedef struct {
	uv_stream_t *goshandle;  	///< GOS 스트림 핸들

	cf_msgbuf_t readbuf; 		///< GOS에서 받은 메세지의 버퍼
	cf_msgbuf_t writebuf; 		///< GOS로 보낼 메세지의 버퍼

	int nodecnt;
	int nodesize;
	gcg_nodeconn_t *pnodeconns;	///< 다중 노드가 연결될 경우에 대한 정보
} gcg_conninfo_t;

/**
 * 접속정보 구조체를 초기화한다.
 * @param 접속정보 구조체의 포인터
 */
void
gcg_init_conninfo (gcg_conninfo_t *pconn);

/**
 * 접속정보 구조체를 해제한다.
 * @param pconn 접속정보 구조체의 포인터
 */
void
gcg_release_conninfo (gcg_conninfo_t *pconn);

/**
 * GOS 접속정보를 추가한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 에러라면 GCG_ERR, 정상완료라면 GCG_OK
 */
cf_ret_t
gcg_set_gos_connection (gcg_conninfo_t *pconn, uv_stream_t *handle);

/**
 * NODE 접속정보를 추가한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 에러라면 GCG_ERR, 정상완료라면 GCG_OK
 */
cf_ret_t
gcg_set_node_connection (gcg_conninfo_t *pconn, uv_stream_t *handle);

/**
 * NODE ID 정보를 추가한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @param nodeid 노드 ID
 * @return 에러라면 GCG_ERR, 정상완료라면 GCG_OK
 */
cf_ret_t
gcg_set_node_connection_info (gcg_conninfo_t *pconn, uv_stream_t *handle, int nodeid);

/**
 * handle 을 이용하여 GCG 접속인지를 확인한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return GOS와의 연결이면 TRUE, 아니면 FALSE
 */
int
gcg_is_gos (gcg_conninfo_t *pconn, uv_stream_t *handle);

/**
 * GOS handle 을 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @return GOS의 handle, 없다면 NULL
 */
uv_stream_t *
gcg_get_goshandle (gcg_conninfo_t *pconn);

/**
 * node id 를 이용하여 handle 을 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param node 아이디
 * @return node의 handle, 없다면 NULL
 */
uv_stream_t *
gcg_get_nodehandle (gcg_conninfo_t *pconn, int nodeid);

/**
 * index 를 이용하여 handle 을 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param index
 * @return node의 handle, 없다면 NULL
 */
uv_stream_t *
gcg_get_nodehandle_by_index (gcg_conninfo_t *pconn, int index);

/**
 * GOS 읽기용 메세지 버퍼를 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @return 읽기용 메세지 버퍼의 포인터
 */
cf_msgbuf_t *
gcg_get_readmsgbuf  (gcg_conninfo_t *pconn);

/**
 * gos 쓰기용 메세지 버퍼를 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @return 쓰기용 메세지 버퍼의 포인터
 */
cf_msgbuf_t *
gcg_get_writemsgbuf  (gcg_conninfo_t *pconn);

/**
 * node 메세지 읽기용 TTA12PacketParsing을 찾는다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return TTA12PacketParsing 포인터
 */
TTA12PacketParsing *
gcg_get_tta12parser (gcg_conninfo_t *pconn, uv_stream_t *handle);

/**
 * 접속정보를 제거한다.
 * @param pconn 접속정보 구조체의 포인터
 * @param handle 스트림에 대한 핸들 
 * @return 에러라면 GCG_ERR, 정상완료라면 GCG_OK
 */
cf_ret_t
gcg_remove_connection (gcg_conninfo_t *pconn, uv_stream_t *handle);

/**
 * GCG가 GOS와 NODE 모두에 접속이 된 상태인지 확인한다.
 * @param pconn 접속정보 구조체의 포인터
 * @return 접속되었다면 1, 아니면 0
 */
int
gcg_is_connected (gcg_conninfo_t *pconn);

/**
 * 버퍼에 있는 TTA P3 메세지프레임을 파싱한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param pframe TTA P3 메세지프레임
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gcg_parseframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe);

/**
 * TTA P3 메세지프레임을 버퍼에 기록한다. 
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param pframe TTA P3 메세지프레임
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gcg_writeframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe);


#endif
