
#ifndef _GCG_SERVER_H_
#define _GCG_SERVER_H_

/** GCG 운영을 위한 내부 데이터 */
typedef struct {
	uv_timer_t timer;   	/** 타이머 처리를 위한 내부 변수 */
	uv_tcp_t ttaclient; 	/** GOS로 접속하여 TTA P3 통신을 위한 내부 변수 */
	uv_tcp_t ttaserver; 	/** 노드에서 접속받아 TTA P1,2 통신을 위한 내부 변수 */
	uv_connect_t connreq;	/** connect request 변수 */
	int readytosend;		/** node 로부터 메세지를 받아서 GOS로 메세지 전송준비가 되면 1 */
} gcg_server_t;

void
gcg_init_server (gcg_server_t *pserver);

void
gcg_release_server (gcg_server_t *pserver);

void
gcg_get_ttaserver (gcg_server_t *pserver);

gcg_config_t *
gcg_get_config ();

gcg_server_t *
gcg_get_server ();

gcg_conninfo_t *
gcg_get_conninfo ();


cf_ret_t
gcg_send_tta3frame (uv_stream_t *handle, tp3_frame_t *pframe);

cf_ret_t
gcg_process_tta3frame (uv_stream_t *handle, tp3_frame_t *pframe);

cf_ret_t
gcg_process_tta12packet (gcg_conninfo_t *pconn, uv_stream_t *handle, TTA12PacketParsing *parser);

cf_ret_t
gcg_send_tta12packet (uv_stream_t *handle, TTA12Packet *packet);

cf_ret_t
gcg_initialize (char *conffile, int gcgid);

void
gcg_finalize ();

cf_ret_t
gcg_timer_start (gcg_server_t *pgcg, gcg_config_t *pconfig);

cf_ret_t
gcg_ttaserver_start (gcg_server_t *pgcg, gcg_config_t *pconfig);

cf_ret_t
gcg_ttaclient_connect (gcg_server_t *pgcg, gcg_config_t *pconfig);

#endif
