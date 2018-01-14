/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gcg_server.cpp
 * \brief GCG 서버관련 코드. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <uv.h>
#include <cf.h>
#include <tp3.h>
#include <gnodehelper.h>

#include "gcg_base.h"
#include "gcg_config.h"
#include "gcg_connection.h"
#include "gcg_server.h"

/** GCG 설정 및 내부데이터 */
static struct {
    gcg_config_t config;        /** GCG 설정 */
    gcg_server_t server;        /** GCG 운영을 위한 내부 데이터 */
    gcg_conninfo_t conninfo;    /** GCG에 접속된 GOS, NODE들과 통신하기위한 핸들정보 */
    GNODEGCGHelper helper;       /** GCG에 접속된 Node 와 통신을 위한 헬퍼 */
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

GNODEGCGHelper *
gcg_get_gnodegcghelper () {
    return &(gcgdata.helper);
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
    if (buf->base == NULL)
        LOG(ERROR) << "buffer allocation failed.";
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
        LOG(WARNING) << "read error [" << nread << "]";
        gcg_shutdown_stream (handle);
    } else if (nread > 0) {
        if (gcg_is_gos (&(gcgdata.conninfo), handle)) {
            tp3_frame_t frame;
            cf_msgbuf_t *preadbuf = gcg_get_readmsgbuf (&(gcgdata.conninfo));

            cf_append_msgbuf (preadbuf, buf->base, nread);
            while (OK == gcg_parseframe_msgbuf (preadbuf, &frame)) {
                if (ERR == gcg_process_tta3frame (handle, &frame)) {
                    CF_FREE (frame.data);
                    CF_FREE (buf->base);
                    LOG(WARNING) << "gcg message processing failed.";
                    return;
                    
                }
                CF_FREE(frame.data);
            }
        } else {
            //*
            cf_msgbuf_t *preadbuf = gcg_get_node_readmsgbuf (&(gcgdata.conninfo), handle);
            cf_append_msgbuf (preadbuf, buf->base, nread);
            GNODEGCGHelper *helper = gcg_get_gnodegcghelper ();
            int nuse = 0;
            LOG (INFO) << "read from node [" << nread << "]";
            do {
                LOG(INFO) << "[" << cf_getlength_msgbuf (preadbuf) << "] bytes in buffer"; 
                nuse = helper->receive ((byte *)cf_getbuf_msgbuf (preadbuf), 
                        cf_getlength_msgbuf (preadbuf));

                if (nuse > 0) {
                    if (ERR == 
                        gcg_process_tta12packet (&(gcgdata.conninfo), handle, helper)) {
                        LOG(WARNING) << "node message processing failed.";
                    }
                    cf_setused_msgbuf (preadbuf, nuse);
                }
                LOG(INFO) << "[" << nuse << "] bytes was used to parse";
            } while (nuse > 0 && cf_getlength_msgbuf (preadbuf) > 0);
            //*/
        }
    }
    CF_FREE (buf->base);
}

void 
gcg_after_write (uv_write_t* req, int status) {
    if (status != 0) {
        LOG(ERROR) << "uv_write error : " << status;
    }
    CF_FREE (req);
}


ret_t
gcg_send_tta12packet (byte *buffer, uint len, int nodeid) {
    uv_buf_t buf;
    uv_write_t *req = (uv_write_t *) CF_MALLOC(sizeof (uv_write_t ));
    gcg_conninfo_t *pconn = gcg_get_conninfo ();
    uv_stream_t *handle = gcg_get_nodehandle (pconn, nodeid);
    
    if (handle == NULL) {
        LOG (WARNING) << "No node is connected.";
        return OK;
    }

    buf.base = (char *)buffer;
    buf.len = len;

    if (uv_write(req, handle, &buf, 1, gcg_after_write)) {
        LOG(WARNING) << "TTA12 packet message sending failed.";
        return ERR;
    }
    LOG(INFO) << "Send TTA12 Packet";
    return OK;
}

ret_t
gcg_process_tta12packet (gcg_conninfo_t *pconn, uv_stream_t *handle, GNODEGCGHelper *helper) {
    //*
    tp3_nodedata_t node;
    gcg_server_t *pserver;
    int n, i;
    int ids[_GCG_MAX_DEV_PER_NODE];
    GNODEMessage *msg = helper->getrecvmsg ();
    frameheader_t *header = msg->getframeheader ();
    GNODEDataPayload *payload;
    devinfo_t *pdevinfo;

    switch (header->frametype) {
        case FT_DATA:
            payload = (GNODEDataPayload *)msg->getpayload ();
            n = header->payloadlen;
            if ( n > _GCG_MAX_DEV_PER_NODE ) 
                n = _GCG_MAX_DEV_PER_NODE ;

            for (i = 0; i < n; i++) {
                pdevinfo = payload->getdeviceinfo(i);
                ids[i] = tp3_getgid (header->nodeid, pdevinfo->devid);
            }

            if (header->devtype == DT_SENSOR) {
                LOG(INFO) << "Sensor Data packet : count " << n << "/" << (int)(header->payloadlen);
                tp3_initnodedata (&node, n, ids, TP3_DEV_SNODE);
            } else if (header->devtype == DT_ACTUATOR) {
                LOG(INFO) << "Actuator Data packet : count " << n << "/" << (int)(header->payloadlen);
                tp3_initnodedata (&node, n, ids, TP3_DEV_ANODE);
            }

            for (i = 0; i < n; i++) {
                pdevinfo = payload->getdeviceinfo(i);
                tp3_setdevenv (&node, ids[i], pdevinfo->value); 
                tp3_setdevstat (&node, ids[i], TP3_DEVST_NORMAL);
                LOG(INFO) << i << "th device " << header->nodeid << "-" 
                    << (int)(pdevinfo->devid) << " : " << pdevinfo->value;
            }

            tp3_updategcgnode (header->nodeid, &node);
            tp3_setgcgsnodestat (header->nodeid, TP3_NODEST_NORMAL);

            // node ID setting
            gcg_set_node_connection_info (pconn, handle, header->nodeid);
            pserver = gcg_get_server ();
            pserver->readytosend = 1;    
            break;

        default:
            // do nothing
            LOG(WARNING) << "Node : Not Data packet. TYPE[" << header->frametype 
                << "] NODE_ID[" << header->nodeid << "]"; 
            break;
    }

    /* 굳이 리턴 패킷을 만들어서 보내지 않는다.  */

    return OK;
}

ret_t
gcg_send_tta3frame (tp3_frame_t *pframe) {
    uv_buf_t buf;
    uv_write_t *req = (uv_write_t *) CF_MALLOC(sizeof (uv_write_t ));
    cf_msgbuf_t *pwritebuf = gcg_get_writemsgbuf (&(gcgdata.conninfo));
    uv_stream_t *handle = gcg_get_goshandle (&(gcgdata.conninfo));

    if (ERR == gcg_writeframe_msgbuf (pwritebuf, pframe)) {
        LOG(ERROR) << "TTA3 message writing failed.";
        return ERR;
    }

    buf.base = cf_getbuf_msgbuf (pwritebuf);
    buf.len = cf_getlength_msgbuf (pwritebuf);

    if (uv_write(req, handle, &buf, 1, gcg_after_write)) {
        LOG(ERROR) << "TTA3 message sending failed.";
        return ERR;
    }
    return OK;
}

ret_t
gcg_process_tta3frame (uv_stream_t *handle, tp3_frame_t *pframe) {
    tp3_frame_t res;
    tp3_stat_t sc;
    ret_t rc;

    tp3_msg_t req;
    tp3_initmsg (&req);
    tp3_parseframe (pframe, &req);
    //tp3_printmsg (&req);
    tp3_releasemsg (&req);
    
    sc = tp3_response (pframe, &res, NULL);
    if (sc == TP3_SC_IS_NOT_REQUEST) {
        return OK;
    }
    if (sc != TP3_SC_NOERROR) {
        LOG(ERROR) << "TTA3 message processing failed.";
        return ERR;
    }
    rc = gcg_send_tta3frame (&res);
    tp3_releaseframe (&res);

    return rc;
}


void
gcg_on_connect (uv_connect_t *conn, int status) {
    if (status == UV_ECONNREFUSED || status == -110 ) {
        LOG(INFO) << "connect error : " << status;
        sleep (5);
        LOG(INFO) << "retry to connect.";
        gcg_ttaclient_connect (gcg_get_server (), gcg_get_config ());
    } else if ( status == 0 ) {
        tp3_msg_t req;
        tp3_frame_t frame;

        gcg_set_gos_connection (gcg_get_conninfo (), conn->handle);
        tp3_connectmsg (&req);
        tp3_printmsg (&req);
        tp3_generateframe (&req, &frame);
        gcg_send_tta3frame (&frame);
        tp3_releaseframe (&frame);
        tp3_releasemsg (&req);

        uv_read_start (conn->handle, gcg_buf_alloc, gcg_after_read);
    } else {
        LOG(INFO) << "unknown status on connect : " << status;
    }
}


void 
gcg_on_connection (uv_stream_t *server, int status) {
    uv_stream_t *stream;
    int r;

    if (status) {
        LOG(WARNING) << "Connection error";
        return ;
    }

    stream = (uv_stream_t *) CF_MALLOC (sizeof(uv_tcp_t));
    if (stream == NULL) {
        LOG(WARNING) << "Connection error";
        return ;
    }
    r = uv_tcp_init(uv_default_loop (), (uv_tcp_t*)stream);
    if (r) {
        LOG(WARNING) << "Connection initialization error";
        return ;
    }

    /* associate server with stream */
    stream->data = server;

    r = uv_accept(server, stream);
    if (r) {
        LOG(WARNING) << "Connection accept error";
        return ;
    }

    r = uv_read_start(stream, gcg_buf_alloc, gcg_after_read);
    if (r) {
        LOG(WARNING) << "Connection read error";
        return ;
    }
    
    LOG(INFO) << "a node connected.";

    if (ERR == gcg_set_node_connection (gcg_get_conninfo (), stream)) {
        gcg_shutdown (stream);
    }
}

void 
gcg_on_server_close(uv_handle_t* handle) {
    ;
}

ret_t
gcg_ttaserver_start (gcg_server_t *pgcg, gcg_config_t *pconfig) {
    struct sockaddr_in addr;
    int r;

    r = uv_tcp_init(uv_default_loop (), &(pgcg->ttaserver));
    if (r) {
        LOG(WARNING) << "Socket creation error";
        return ERR;
    }

    r = uv_ip4_addr("0.0.0.0", pconfig->gcgport, &addr);
    if (r) {
        LOG(WARNING) << "Socket address error";
        return ERR;
    }

    r = uv_tcp_bind(&(pgcg->ttaserver), (const struct sockaddr*) &addr, 0);
    if (r) {
        LOG(WARNING) << "Socket binding error";
        return ERR;
    }

    r = uv_listen((uv_stream_t *)&(pgcg->ttaserver), SOMAXCONN, gcg_on_connection);
    if (r) {
        LOG(WARNING) << "Socket listen error";
        return ERR;
    }

    LOG (INFO) << "gcg server for node started (port: " << pconfig->gcgport << ")";

    return OK;
}

ret_t
gcg_ttaclient_connect (gcg_server_t *pgcg, gcg_config_t *pconfig) {
    struct sockaddr_in addr;
    int r;
    
    LOG (INFO) << "gos server (host: " << pconfig->gosip << ",port: " << pconfig->gosport << ")";
    r = uv_ip4_addr(pconfig->gosip, pconfig->gosport, &addr);
    if (r) {
        LOG(WARNING) << "Socket address error";
        return ERR;
    }

    r = uv_tcp_init(uv_default_loop (), &(pgcg->ttaclient));
    if (r) {
        LOG(WARNING) << "Socket creation error";
        return ERR;
    }

    r = uv_tcp_connect (&(pgcg->connreq), &(pgcg->ttaclient), 
            (const struct sockaddr*) &addr, gcg_on_connect);
    if (r) {
        LOG(WARNING) << "Socket connection error";
        return ERR;
    }

    return OK;
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
            LOG(INFO) << "status notice : " << noti.header.msgseq;
            //tp3_printmsg (&noti);
            tp3_generateframe (&noti, &frame);
            gcg_send_tta3frame (&frame);
            tp3_releaseframe (&frame);
            tp3_releasemsg (&noti);

            tp3_notifyenvmsg (&noti);
            LOG(INFO) << "env notice : " << noti.header.msgseq;
            //tp3_printmsg (&noti);
            tp3_generateframe (&noti, &frame);
            //tp3_printframe (&frame);
            gcg_send_tta3frame (&frame);
            tp3_releaseframe (&frame);
            tp3_releasemsg (&noti);
        
            pserver->readytosend = 0;
        } else {
            LOG(INFO) << "GCG is not ready to send any message.";
        }
    } else {
        LOG(INFO) << "GCG is waiting for connection.";
        if (gcg_get_goshandle (pconn) == NULL && gos_client_restart ) {
            LOG(INFO) << "GCG reset connection.";
            gos_client_restart = 0;
            gcg_ttaclient_connect (gcg_get_server (), gcg_get_config ());
        }
    }
}

ret_t
gcg_timer_start (gcg_server_t *pgcg, gcg_config_t *pconfig) {
    int r;
    r = uv_timer_init (uv_default_loop (), &(pgcg->timer));
    if (r) {
        LOG(WARNING) << "Timer creation error";
        return ERR;
    }
    r = uv_timer_start (&(pgcg->timer), gcg_timer_cb, 0, pconfig->timer);
    if (r) {
        LOG(WARNING) << "Timer start error";
        return ERR;
    }

    LOG(INFO) << "GCG set timer : " << pconfig->timer;

    return OK;
}

tp3_stat_t
gcg_actctrl_cb (ptp3_msg_t preq, ptp3_msg_t pres, void *data) {
    GNODEGCGHelper *helper = gcg_get_gnodegcghelper ();
    int gids[_GCG_MAX_GID], vals[_GCG_MAX_GID];
    int i, n;
    uint len;
    byte buf[1024];

    LOG(INFO) << "Actuator control message [msgseq : " 
              << (preq->header).msgseq << "] received.";
    n = tp3_readfield_array (preq, TP3_FC_ARROFGAID, gids, _GCG_MAX_GID);
    n = tp3_readfield_array (preq, TP3_FC_ARROFACTSVAL, vals, _GCG_MAX_GID);

    for (i = 0; i < n; i++) {
        helper->clear ();
        helper->setnodeid (tp3_getnid (gids[i]));
        helper->setnextcontrol (tp3_getdevid (gids[i]), vals[i]);

        LOG(INFO) << "A command (nid: " << tp3_getnid (gids[i]) 
                << ", did: " << tp3_getdevid (gids[i])
                << ", arg: " << ((vals[i]&0xF000)>>12) 
                << ", time: " << (vals[i]&0xFFF) << ") would be sent."; 

        len = helper->request_control (buf, 1024);
        LOG(INFO) << "A command length : " << len;
        gcg_send_tta12packet (buf, len, tp3_getnid (gids[i]));
        usleep (300);
    }
    
    return TP3_SC_NOERROR;
}

ret_t
gcg_initialize (char *conffile, int gcgid) {
    tp3_gcgprof_t gcgprof;
    byte ipv4[4];
    byte ipv6[6];
    char ip[_GCG_BUF_LEN];
    char *p;
    int i;

    gcgdata.config.gcgid = gcgid;
    ERR_RETURN (gcg_read_config (&(gcgdata.config), conffile), "read config failed.");

    gcg_init_server (&(gcgdata.server));
    gcg_init_conninfo (&(gcgdata.conninfo));

    strcpy (ip, gcgdata.config.gcgip);
    for (p = strtok (ip, "."), i = 0; p != NULL; p = strtok (NULL, "."), i++) {
        ipv4[i] = atoi(p);
    }
    ipv6[0] = 0; ipv6[1] = 0; ipv6[2] = 0; ipv6[3] = 0; ipv6[4] = 0; ipv6[5] = 0;
    tp3_setgcgprof (&gcgprof, (byte *)_GCG_TEMP_SERIAL, (byte *)_GCG_TEMP_MODEL, 
                    ipv4, ipv6, 0, 0, 0, 0, 0, 0, 0);
    tp3_initgcg (gcgdata.config.gcgid, gcgdata.config.gosid, &gcgprof);
    tp3_registcbfunc (TP3_MT_ACTCTRL, gcg_actctrl_cb, NULL);

    return OK;
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

