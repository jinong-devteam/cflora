#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
	#include <unistd.h>
#else
	#define sleep(a) Sleep(a*1000)
	#define usleep(a) Sleep(a)
#endif

#include <uv.h>
#include <cf.h>
#include <tp3.h>
#include <util.h>
#include <tp12.h>

#include "gcg_base.h"
#include "gcg_config.h"
#include "gcg_connection.h"
#include "gcg_server.h"

/** GCG 설정 및 내부데이터 */
static struct {
	gcg_config_t config;        /** GCG 설정 */
	gcg_server_t server;    	/** GCG 운영을 위한 내부 데이터 */
	gcg_conninfo_t conninfo;    /** GCG에 접속된 GOS, NODE들과 통신하기위한 핸들정보 */
} gcgdata;

static uv_stream_t * gos_saved_handle = NULL ;
static int gos_client_restart = 0 ;

void
gcg_init_server (gcg_server_t *pserver) {
	memset (pserver, 0, sizeof (gcg_server_t));
}

void
gcg_release_server (gcg_server_t *pserver) {
	;
}

gcg_config_t *
gcg_get_config () {
	return &(gcgdata.config);
}

gcg_server_t *
gcg_get_server (){
	return &(gcgdata.server);
}

gcg_conninfo_t *
gcg_get_conninfo (){
	return &(gcgdata.conninfo);
}

void
gos_close_timer (uv_handle_t *handle) {
    ;
}

void 
gcg_on_close(uv_handle_t *peer) {
	//CF_FREE (peer);
}

void 
gcg_buf_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = (char *) CF_MALLOC(suggested_size);
	buf->len = suggested_size;
}

void 
gcg_after_shutdown(uv_shutdown_t *req, int status) {
	uv_close((uv_handle_t*) req->handle, gcg_on_close);
	CF_FREE (req);
	if ( gos_saved_handle == req->handle ) {
		gos_client_restart = 1 ;
	}
}

void
gcg_shutdown (uv_stream_t *handle) {
	uv_shutdown_t *sreq;
	sreq = (uv_shutdown_t *) CF_MALLOC (sizeof (uv_shutdown_t));
	uv_shutdown (sreq, handle, gcg_after_shutdown);
}

void
gcg_shutdown_stream (uv_stream_t *handle) {
	gcg_shutdown (handle);
	gcg_remove_connection (gcg_get_conninfo (), handle);
}

void
gcg_after_read (uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
	if (nread < 0) {
        if (gcg_is_gos (&(gcgdata.conninfo), handle)) {
            gos_saved_handle = handle ;
        }
        gcg_shutdown_stream (handle);
	} else if (nread > 0) {
		if (gcg_is_gos (&(gcgdata.conninfo), handle)) {
			tp3_frame_t frame;
			cf_msgbuf_t *preadbuf = gcg_get_readmsgbuf (&(gcgdata.conninfo));

			cf_append_msgbuf (preadbuf, buf->base, nread);
			while (CF_OK == gcg_parseframe_msgbuf (preadbuf, &frame)) {
				if ( CF_ERR == gcg_process_tta3frame (handle, &frame) ) {
					CF_FREE( frame.data ) ;
					CF_FREE (buf->base);
					CF_EXP_VRETURN ( TRUE , "gcg message processing failed.");
				}
				CF_FREE( frame.data ) ;
			}
		} else {
			//*
			TTA12PacketParsing *parser = gcg_get_tta12parser (&(gcgdata.conninfo), handle);
			int idx = 0;
			do {
				parser->Clear ();
				(parser->packet).Add ((unsigned char *)buf->base + idx, nread - idx);
				CF_EXP_VRETURN (CF_ERR 
					== gcg_process_tta12packet (&(gcgdata.conninfo), handle, parser), 
					"node message processing failed.");
				idx += (parser->packet).GetReadIndex () + 1;
		
			} while (nread > idx);
			//} while (0);
			//*/
		}
	}
	CF_FREE (buf->base);
}

void 
gcg_after_write (uv_write_t* req, int status) {
	CF_EXP_VRETURN (status != 0, "uv_write error");
	CF_FREE (req);
}


cf_ret_t
gcg_send_tta12packet (CPacket *packet, int nodeid) {
	uv_buf_t buf;
	uv_write_t *req = (uv_write_t *) CF_MALLOC(sizeof (uv_write_t ));
	gcg_conninfo_t *pconn = gcg_get_conninfo ();
	uv_stream_t *handle = gcg_get_nodehandle (pconn, nodeid);
	
	if (handle == NULL) {
		CF_ERR_LOG ("No node is connected.");
		return CF_OK;
	}

	buf.base = (char *)packet->GetBYTE ();
	buf.len = packet->Getsize ();

	if (uv_write(req, handle, &buf, 1, gcg_after_write)) {
		CF_ERR_RETURN (1, "gcg tta12 packet message sending failed.");
	}
	CF_VERBOSE (CF_VERBOSE_MID, "Send TTA12 Packet");
	return CF_OK;
}

cf_ret_t
gcg_process_tta12packet (gcg_conninfo_t *pconn, uv_stream_t *handle, TTA12PacketParsing *parser) {
	//*
	tp3_nodedata_t node;
	gcg_server_t *pserver;
	int n, i;
	int ids[_GCG_MAX_DEV_PER_NODE];
	SensorValue sval[_GCG_MAX_DEV_PER_NODE];
	ActuatorValue aval[_GCG_MAX_DEV_PER_NODE];

	switch (parser->ParsingPacket ()) {
		case FTYPE_DATA:
			if ((parser->FCF).device == FCF_SENSOR) {
				n = parser->GetSensorData (sval);
				if ( n > _GCG_MAX_DEV_PER_NODE ) 
					n = _GCG_MAX_DEV_PER_NODE ;
				CF_VERBOSE (CF_VERBOSE_MID, "Sensor Data packet : count %d.", n);

				for (i = 0; i < n; i++) {
					ids[i] = tp3_getgid (parser->NODE_ID, sval[i].id);
				}

				tp3_initnodedata (&node, n, ids, TP3_DEV_SNODE);

				for (i = 0; i < n; i++) {
					tp3_setdevenv (&node, ids[i], sval[i].value);
					tp3_setdevstat (&node, ids[i], TP3_DEVST_NORMAL);
					CF_VERBOSE (CF_VERBOSE_MID, "%dth sensor[%d-%d] : [%d]", 
						i, parser->NODE_ID, sval[i].id, sval[i].value);
				}

				tp3_updategcgnode (parser->NODE_ID, &node);
				tp3_setgcgsnodestat (parser->NODE_ID, TP3_NODEST_NORMAL);

			} else if ((parser->FCF).device == FCF_ACTUATOR) {
				n = parser->GetActuatorData (aval);
				if ( n > _GCG_MAX_DEV_PER_NODE ) 
					n = _GCG_MAX_DEV_PER_NODE ;
				CF_VERBOSE (CF_VERBOSE_MID, "Actuator Data packet : count %d.", n);

				for (i = 0; i < n; i++) {
					ids[i] = tp3_getgid (parser->NODE_ID, aval[i].id);
				}

				tp3_initnodedata (&node, n, ids, TP3_DEV_ANODE);

				for (i = 0; i < n; i++) {
					tp3_setdevenv (&node, ids[i], aval[i].value);
					tp3_setdevstat (&node, ids[i], TP3_DEVST_NORMAL);
					CF_VERBOSE (CF_VERBOSE_MID, "%dth actuator [%d-%d] : [%d,%d]", 
						i, parser->NODE_ID, aval[i].id, 
						(aval[i].value & 0xF000) >> 12, aval[i].value & 0xFFF);
				}

				tp3_updategcgnode (parser->NODE_ID, &node);
				tp3_setgcganodestat (parser->NODE_ID, TP3_NODEST_NORMAL);
			}

			// node ID setting
			gcg_set_node_connection_info (pconn, handle, parser->NODE_ID);
			pserver = gcg_get_server ();
			pserver->readytosend = 1;	
			break;

		default:
			// do nothing
			CF_VERBOSE (CF_VERBOSE_MID, "Node : Not Data packet. TYPE[%d] NODE_ID[%d]", parser->FCF.type, parser->NODE_ID);
			break;
	}

	/* 굳이 리턴 패킷을 만들어서 보내지 않는다. 
	CPacket *packet;
	packet = parser->GetReturnPacket ();
	return gcg_send_tta12packet (packet);
	*/

	return CF_OK;
}

cf_ret_t
gcg_send_tta3frame (tp3_frame_t *pframe) {
	uv_buf_t buf;
	uv_write_t *req = (uv_write_t *) CF_MALLOC(sizeof (uv_write_t ));
	cf_msgbuf_t *pwritebuf = gcg_get_writemsgbuf (&(gcgdata.conninfo));
	uv_stream_t *handle = gcg_get_goshandle (&(gcgdata.conninfo));

	if (CF_ERR == gcg_writeframe_msgbuf (pwritebuf, pframe)) {
		CF_ERR_RETURN (1, "gcg message writing failed.");
	}

	buf.base = cf_getbuf_msgbuf (pwritebuf);
	buf.len = cf_getlength_msgbuf (pwritebuf);

	//CF_VERBOSE (CF_VERBOSE_HIGH, "send data [%d].", buf.len);

	if (uv_write(req, handle, &buf, 1, gcg_after_write)) {
		CF_ERR_RETURN (1, "gcg message sending failed.");
	}
	return CF_OK;
}

cf_ret_t
gcg_process_tta3frame (uv_stream_t *handle, tp3_frame_t *pframe) {
	tp3_frame_t res;
	tp3_stat_t sc;
	cf_ret_t rc;

	tp3_msg_t req;
	tp3_initmsg (&req);
	tp3_parseframe (pframe, &req);
	//tp3_printmsg (&req);
	tp3_releasemsg (&req);
	
	sc = tp3_response (pframe, &res, NULL);
	if (sc == TP3_SC_IS_NOT_REQUEST) {
		// by chj 2015.9.28
		// memory leak
		//tp3_releaseframe (&res);
		return CF_OK;
	}
	if (sc != TP3_SC_NOERROR) {
		// by chj 2015.9.28
		// memory leak
		//tp3_releaseframe (&res);
		CF_ERR_RETURN (1, "gcg message processing failed.");
	}
	rc = gcg_send_tta3frame (&res);
	tp3_releaseframe (&res);

	return rc;
}


void
gcg_on_connect (uv_connect_t *conn, int status) {
	if (status == UV_ECONNREFUSED || status == -110 ) {
        fprintf(stderr,"tcp_connect retry  status %d\n" , status) ;
		//retry
		sleep (5);
		gcg_ttaclient_connect (gcg_get_server (), gcg_get_config ());
	} else if ( status == 0 ) {
		tp3_msg_t req;
		tp3_frame_t frame;

		gcg_set_gos_connection (gcg_get_conninfo (), conn->handle);
		tp3_connectmsg (&req);
		//tp3_printmsg (&req);
		tp3_generateframe (&req, &frame);
		gcg_send_tta3frame (&frame);
		tp3_releaseframe (&frame);
		tp3_releasemsg (&req);

		uv_read_start (conn->handle, gcg_buf_alloc, gcg_after_read);
	}
    else {
        fprintf(stderr,"tcp_connect return  status %d\n" , status) ;
    }
}


void 
gcg_on_connection (uv_stream_t *server, int status) {
	uv_stream_t *stream;
	int r;

	CF_EXP_VRETURN (status, "Connection error\n");

	stream = (uv_stream_t *) CF_MALLOC (sizeof(uv_tcp_t));
	CF_EXP_VRETURN (stream == NULL, "Connection error\n");
	r = uv_tcp_init(uv_default_loop (), (uv_tcp_t*)stream);
	CF_EXP_VRETURN (r, "Connection initialization error\n");

	/* associate server with stream */
	stream->data = server;

	r = uv_accept(server, stream);
	CF_EXP_VRETURN (r, "Connection accept error\n");

	r = uv_read_start(stream, gcg_buf_alloc, gcg_after_read);
	CF_EXP_VRETURN (r, "Connection read error\n");
	
	CF_VERBOSE (CF_VERBOSE_LOW, "a node connected.");

	if (CF_ERR == gcg_set_node_connection (gcg_get_conninfo (), stream)) {
		gcg_shutdown (stream);
	}
}

void 
gcg_on_server_close(uv_handle_t* handle) {
	;
}

cf_ret_t
gcg_ttaserver_start (gcg_server_t *pgcg, gcg_config_t *pconfig) {
	struct sockaddr_in addr;
	int r;

	r = uv_tcp_init(uv_default_loop (), &(pgcg->ttaserver));
	CF_EXP_RETURN (r, CF_ERR, "Socket creation error\n");

	r = uv_ip4_addr("0.0.0.0", pconfig->gcgport, &addr);
	CF_EXP_RETURN (r, CF_ERR, "Socket address error\n");

	r = uv_tcp_bind(&(pgcg->ttaserver), (const struct sockaddr*) &addr, 0);
	CF_EXP_RETURN (r, CF_ERR, "Socket binding error\n");

	r = uv_listen((uv_stream_t *)&(pgcg->ttaserver), SOMAXCONN, gcg_on_connection);
	CF_EXP_RETURN (r, CF_ERR, "Socket listen error\n");

	CF_VERBOSE (CF_VERBOSE_LOW, "gcg server for node started (port: %d).", pconfig->gcgport);

	return CF_OK;
}

cf_ret_t
gcg_ttaclient_connect (gcg_server_t *pgcg, gcg_config_t *pconfig) {
	struct sockaddr_in addr;
	int r;
	
	r = uv_ip4_addr(pconfig->gosip, pconfig->gosport, &addr);
	CF_EXP_RETURN (r, CF_ERR, "Socket address error\n");

	r = uv_tcp_init(uv_default_loop (), &(pgcg->ttaclient));
	CF_EXP_RETURN (r, CF_ERR, "Socket creation error\n");

	r = uv_tcp_connect (&(pgcg->connreq), &(pgcg->ttaclient), 
			(const struct sockaddr*) &addr, gcg_on_connect);
	CF_EXP_RETURN (r, CF_ERR, "Socket connection error\n");


	return CF_OK;
}

void
gcg_timer_cb (uv_timer_t *handle) {
	tp3_msg_t noti;
	tp3_frame_t frame;
	gcg_conninfo_t *pconn = gcg_get_conninfo();
	gcg_server_t *pserver = gcg_get_server ();

	if (gcg_is_connected (pconn)) {
		if (pserver->readytosend) {
			tp3_notifystatmsg (&noti);
			CF_VERBOSE (CF_VERBOSE_MID, "status notice [%d].", noti.header.msgseq);
			//tp3_printmsg (&noti);
			tp3_generateframe (&noti, &frame);
			gcg_send_tta3frame (&frame);
			tp3_releaseframe (&frame);
			tp3_releasemsg (&noti);

			tp3_notifyenvmsg (&noti);
			CF_VERBOSE (CF_VERBOSE_MID, "env notice [%d].", noti.header.msgseq);
			//tp3_printmsg (&noti);
			tp3_generateframe (&noti, &frame);
			//tp3_printframe (&frame);
			gcg_send_tta3frame (&frame);
			tp3_releaseframe (&frame);
			tp3_releasemsg (&noti);
		
			pserver->readytosend = 0;
		} else {
			CF_VERBOSE (CF_VERBOSE_MID, "GCG is not ready to send any message.");
/*
			tp3_keepalivemsg (&noti);
			CF_VERBOSE (CF_VERBOSE_MID, "keep alive [%d].", noti.header.msgseq);
			tp3_generateframe (&noti, &frame);
			gcg_send_tta3frame (&frame);
			tp3_releaseframe (&frame);
			tp3_releasemsg (&noti);
*/
		}
	} else {
		CF_VERBOSE (CF_VERBOSE_MID, "GCG waits for connection .");
        if (gcg_get_goshandle (pconn) == NULL && gos_client_restart ) {
			CF_VERBOSE (CF_VERBOSE_MID, "GCG reset connection.");
            gos_client_restart = 0;
            gcg_ttaclient_connect (gcg_get_server (), gcg_get_config ());
        }
	}
}

cf_ret_t
gcg_timer_start (gcg_server_t *pgcg, gcg_config_t *pconfig) {
	int r;
	r = uv_timer_init (uv_default_loop (), &(pgcg->timer));
	CF_EXP_RETURN (r, CF_ERR, "Timer creation error\n");
	r = uv_timer_start (&(pgcg->timer), gcg_timer_cb, 0, pconfig->timer);
	CF_VERBOSE (CF_VERBOSE_MID, "GCG set timer [%d ms].", pconfig->timer);
	CF_EXP_RETURN (r, CF_ERR, "Timer start error\n");

	return CF_OK;
}

tp3_stat_t
gcg_actctrl_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data) {
	gcg_config_t *pconfig = gcg_get_config ();
	int gids[_GCG_MAX_GID], vals[_GCG_MAX_GID];
	int i, n, nid;
	//*
	TTA12Packet packet;
	CPacket *pk;
	ActuatorValue actdata[_GCG_MAX_DEV_PER_NODE];

	CF_VERBOSE (CF_VERBOSE_MID, "Actuator control message [msgseq : %d] received.", 
				(preq->header).msgseq);
	n = tp3_readfield_array (preq, TP3_FC_ARROFGAID, gids, _GCG_MAX_GID);
	n = tp3_readfield_array (preq, TP3_FC_ARROFACTSVAL, vals, _GCG_MAX_GID);

	// 각각 전송
	for (i = 0; i < n; i++) {
		packet.Clear ();
		packet.SetFrameDevice (FCF_ACTUATOR);
		packet.SetFrameType (FTYPE_REQUEST);
		packet.SetSecurity (1);
		packet.SetAckConfirm (1);
		packet.SetControlID (pconfig->gcgid);
		SequenceCount ++;
		nid = tp3_getnid (gids[i]);
		packet.SetNodeID (nid);
		packet.SetRequestType (REQCMD_ACTUATOR_SET);
		packet.MakeHeader ();
		packet.MakePayload ();

		actdata[0].id = tp3_getdevid (gids[i]);
		actdata[0].type = ACT_WARM;
		actdata[0].value = vals[i];
		CF_VERBOSE (CF_VERBOSE_MID, "a command (%d %d arg: %d time: %d) would be sent.", 
				nid, actdata[0].id, (vals[i]&0xF000)>>12, vals[i]&0xFFF);

		packet.MakeActuatorData (PLTYPE_VALUE, 1, actdata);
		pk = packet.GetPacket ();

		gcg_send_tta12packet (pk, nid);
		usleep (300);
	}
	
	return TP3_SC_NOERROR;
}

cf_ret_t
gcg_initialize (char *conffile, int gcgid) {
	tp3_gcgprof_t gcgprof;
	byte ipv4[4];
	byte ipv6[6];
	char ip[_GCG_BUF_LEN];
	char *p;
	int i;

	gcgdata.config.gcgid = gcgid;
	CF_ERR_RETURN (gcg_read_config (&(gcgdata.config), conffile), "read config failed.");

	gcg_init_server (&(gcgdata.server));
	gcg_init_conninfo (&(gcgdata.conninfo));

	strcpy (ip, gcgdata.config.gcgip);
	for (p = strtok (ip, "."), i = 0; p != NULL; p = strtok (NULL, "."), i++) {
		ipv4[i] = atoi(p);
	}
	ipv6[0] = 0; ipv6[1] = 0; ipv6[2] = 0; ipv6[3] = 0; ipv6[4] = 0; ipv6[5] = 0;
	tp3_setgcgprof (&gcgprof, (byte *)_GCG_TEMP_SERIAL, (byte *)_GCG_TEMP_MODEL, ipv4, ipv6, 0, 0, 0, 0, 0, 0, 0);
	tp3_initgcg (gcgdata.config.gcgid, gcgdata.config.gosid, &gcgprof);

	/*
	for (i = 0; i < gcgdata.config.snodecnt; i++) {
		tp3_initnodedata (&node, 0, NULL, TP3_DEV_SNODE);
		tp3_updategcgnode (i, &node);
		tp3_setgcgsnodestat (i, TP3_NODEST_NORMAL);
	}

	for (i = 0; i < gcgdata.config.anodecnt; i++) {
		tp3_initnodedata (&node, 0, NULL, TP3_DEV_ANODE);
		tp3_updategcgnode (i, &node);
		tp3_setgcganodestat (i, TP3_NODEST_NORMAL);
	}
	*/

	tp3_registcbfunc (TP3_MT_ACTCTRL, gcg_actctrl_cb, NULL);

	/* init tta 1,2 */
	PacketVariable_init() ; 
	ReadDevice_Config() ;

	return CF_OK;
}

void
gcg_finalize () {
	int i;
	uv_stream_t *handle;

	gcg_conninfo_t *pconn = gcg_get_conninfo ();
	gcg_server_t *pserver = gcg_get_server ();

	gcg_shutdown_stream (gcg_get_goshandle (pconn));
	for (i = 0; NULL != (handle = gcg_get_nodehandle_by_index (pconn, i)); i++)
		gcg_shutdown_stream (handle);

	gcg_shutdown ((uv_stream_t *)&(pserver->ttaserver));

	gcg_release_config (&(gcgdata.config));
	gcg_release_server (&(gcgdata.server));
	gcg_release_conninfo (&(gcgdata.conninfo));
}

