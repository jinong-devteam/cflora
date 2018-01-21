/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_server.cpp
 * \brief GOS 서버관련 소스 파일. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>
#include <tp3.h>
#include <cf.h>

#include "gos_base.h"
#include "gos_config.h"
#include "gos_connection.h"
#include "gos_device.h"
#include "gos_process.h"
#include "gos_rule.h"
#include "gos_server.h"

/** GOS 설정 및 내부데이터 */
static struct {
    gos_config_t config;        /** GOS 설정 */
    gos_server_t server;        /** GOS 운영을 위한 내부 데이터 */
    gos_devinfo_t devinfo;      /** GOS와 통신하는 디바이스들의 현재정보 */
    gos_conninfo_t conninfo;    /** GOS에 접속된 GCG들과 통신하기위한 핸들정보 */
    gos_ruleset_t ruleset;        /** GOS 자동 운영을 위한 룰들의 집합 */
} gosdata;

void
gos_init_server (gos_server_t *pserver) {
    memset (pserver, 0, sizeof (gos_server_t));
}

void
gos_release_server (gos_server_t *pserver) {
    ;
}

int
gos_get_restart (gos_server_t *pserver) { 
    return pserver->restart;
}

void
gos_set_restart (gos_server_t *pserver, int restart) { 
    pserver->restart = restart;
}

gos_config_t *
gos_get_config () {
    return &(gosdata.config);
}

gos_server_t *
gos_get_server (){
    return &(gosdata.server);
}

gos_devinfo_t *
gos_get_devinfo (){
    return &(gosdata.devinfo);
}

gos_conninfo_t *
gos_get_conninfo (){
    return &(gosdata.conninfo);
}

gos_ruleset_t *
gos_get_ruleset (){
    return &(gosdata.ruleset);
}


int
gos_check_auto_control (gos_server_t *pserver, gos_config_t *pconfig) {
    cf_db_t *db = &(pconfig->db);
    char control[_GOS_BUF_LEN];
    int rc, rows, columns;          
    char *errmsg;     
    char **results;  

    rc = cf_db_get_table (db, "select control from gos_configuration", &results, &rows, &columns, &errmsg);   
    if (rc != OK) {          
        LOG(ERROR) << "database query execution (check auto control) failed. ";
        cf_db_free (errmsg);      
        return 0;
    }
    strcpy (control, results[1]);
    cf_db_free_table (results);

    return (control[0] == 'a') ? 1 : 0;
}

ret_t
gos_check_restart (gos_server_t *pserver, gos_config_t *pconfig) {
    cf_db_t *db = &(pconfig->db);
    int restart;

    CF_EXECUTE_GET_INT (db, "select restart from gos_configuration", &restart);

    if (restart) {
        CF_EXECUTE ( db , "update gos_configuration set restart=0" ) ;
        gos_set_restart (gos_get_server (), restart);
        uv_timer_stop(&(pserver->timer));
        uv_stop (uv_default_loop ());
    }

    return OK;
}

void
gos_close_timer (uv_handle_t *handle) {
    ;
}

void 
gos_on_close(uv_handle_t *peer) {
    LOG(INFO) << "GCG disconnected.";
    CF_FREE (peer);
}


void 
gos_after_write (uv_write_t* req, int status) {
    if (status != 0) {
        LOG(ERROR) << "uv_write error";
        return;
    }
    CF_FREE (req);
}

void 
gos_after_shutdown(uv_shutdown_t *req, int status) {
    uv_close((uv_handle_t*) req->handle, gos_on_close);
    CF_FREE (req);
}

void
gos_shutdown (uv_stream_t *handle) {
    uv_shutdown_t *sreq;
    sreq = (uv_shutdown_t *) CF_MALLOC (sizeof (uv_shutdown_t));
    uv_shutdown (sreq, handle, gos_after_shutdown);
}

void
gos_shutdown_stream (uv_stream_t *handle) {
    gos_shutdown (handle);
    gos_remove_connection (gos_get_conninfo (), handle);
}

void
gos_after_read (uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
    tp3_frame_t frame;
    LOG(INFO) << "read [" << (long)nread << "] characters from gcg.";

    if (nread < 0) {
        gos_shutdown_stream (handle);

    } else if (nread > 0) {
        cf_msgbuf_t *preadbuf = gos_get_readmsgbuf (&(gosdata.conninfo), handle);

        cf_append_msgbuf (preadbuf, buf->base, nread);
        while (OK == gos_parseframe_msgbuf (preadbuf, &frame)) {
            LOG(INFO) << "parse frame : " << frame.length;
            if ( ERR == gos_process_message (handle, &frame) ) {
                tp3_releaseframe (&frame);
                CF_FREE (buf->base);
                LOG(ERROR) << "gos message processing failed.";
                return ;
            }
            tp3_releaseframe (&frame);
        }
        CF_FREE (buf->base);
    }
}

ret_t
gos_process_message (uv_stream_t *handle, tp3_frame_t *pframe) {
    tp3_frame_t res ;
    tp3_stat_t sc;
    ret_t rc;
    gos_tp3arg_t arg;
    
    gos_set_tp3arg (&arg, handle);
    res.data = NULL ;
    sc = tp3_response (pframe, &res, (void *)&arg);
    if (sc != TP3_SC_NOERROR) {
        if ( res.data != NULL )
            tp3_releaseframe (&res);
        LOG(ERROR) << "gos message processing failed. " << tp3_geterrormsg (sc);
        return ERR;
    }
    rc = gos_send_message (handle, &res);
    LOG(INFO) << "send response frame [" << res.length << "].";
    tp3_releaseframe (&res);

    if (gos_get_remove_connection (gos_get_conninfo (), handle) == 1) {
        LOG(INFO) << "A connection will be lost.";
        gos_remove_connection (gos_get_conninfo (), handle);
        gos_shutdown_stream (handle);
    }
    return rc;
}

ret_t
gos_send_message (uv_stream_t *handle, tp3_frame_t *pframe) {
    uv_write_t *req = (uv_write_t *) CF_MALLOC(sizeof (uv_write_t));
    uv_buf_t buf;
    cf_msgbuf_t *pwritebuf = gos_get_writemsgbuf (&(gosdata.conninfo), handle);

    if (ERR == gos_writeframe_msgbuf (pwritebuf, pframe)) {
        LOG(ERROR) << "gos message writing failed.";
        return ERR;
    }

    buf.base = cf_getbuf_msgbuf (pwritebuf);
    buf.len = cf_getlength_msgbuf (pwritebuf);

    LOG(INFO) << "write message. bytes : " << (int)(buf.len);

    if (uv_write(req, handle, &buf, 1, gos_after_write)) {
        LOG(ERROR) << "gos message sending failed.";
        return ERR;
    }
    return OK;
}

void 
gos_buf_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char *)CF_MALLOC(suggested_size);
    buf->len = suggested_size;
}


void 
gos_on_connection(uv_stream_t *server, int status) {
    uv_stream_t *stream;

    if (status) {
        LOG(ERROR) << "Connection error";
        return ;
    }

    stream = (uv_stream_t *) CF_MALLOC (sizeof(uv_tcp_t));
    if (stream == NULL) {
        LOG(ERROR) << "Connection error";
        return ;
    }

    if (uv_tcp_init(uv_default_loop (), (uv_tcp_t*)stream)) {
        LOG(ERROR) << "Connection initialization error";
        return ;
    }

    /* associate server with stream */
    stream->data = server;

    if (uv_accept(server, stream)) {
        LOG(ERROR) << "Connection accept error";
        return ;
    }

    LOG(INFO) << "GCG connected.";

    if (uv_read_start(stream, gos_buf_alloc, gos_after_read)) {
        LOG(ERROR) << "Connection read error";
        return ;
    }

    gos_add_connection (gos_get_conninfo (), stream);
}

void 
gos_on_server_close(uv_handle_t* handle) {
    ;
}

ret_t
gos_timer_start (gos_server_t *pgos, gos_config_t *pconfig) {
    if (uv_timer_init (uv_default_loop (), &(pgos->timer))) {
        LOG(ERROR) << "Timer creation error";
        return ERR;
    }
    if (uv_timer_start (&(pgos->timer), gos_timer_cb, 0, pconfig->timer)) {
        LOG(ERROR) << "Timer start error";
        return ERR;
    }

    return OK;
}

ret_t
gos_ttaserver_start (gos_server_t *pgos, gos_config_t *pconfig) {
    struct sockaddr_in addr;

    if (uv_tcp_init(uv_default_loop (), &(pgos->ttaserver))) {
        LOG(ERROR) << "Socket creation error";
        return ERR;
    }

    if (uv_ip4_addr("0.0.0.0", pconfig->port, &addr)) {
        LOG(ERROR) << "Socket address error";
        return ERR;
    }

    if (uv_tcp_bind(&(pgos->ttaserver), (const struct sockaddr*) &addr, 0)) {
        LOG(ERROR) << "Socket binding error";
        return ERR;
    }

    if (uv_listen((uv_stream_t *)&(pgos->ttaserver), SOMAXCONN, gos_on_connection)) {
        LOG(ERROR) << "Socket listen error";
        return ERR;
    }

    return OK;
}

ret_t
gos_initialize (char *conffile) {
    cf_db_t *db = &(gos_get_config ()->db);

    if (gos_read_config (&(gosdata.config), conffile)) {
        LOG(ERROR) << "read config failed.";
        return ERR;
    }

    LOG(INFO) << "GOS read configuration " << conffile;
    if (cf_db_open (db)) {
        LOG(ERROR) << "database open failed.";
        return ERR;
    }

    gos_init_server (&(gosdata.server));
    gos_init_devinfo (&(gosdata.devinfo), &(gosdata.config));
    gos_init_conninfo (&(gosdata.conninfo));
    gos_init_rules (&(gosdata.ruleset), &(gosdata.config));
    gos_init_control (&(gosdata.config));

    LOG(INFO) << "GOS set up server data.";

    tp3_initgos (gosdata.config.gosid, gosdata.config.gcgids, gosdata.config.numofgcg);

    tp3_registcbfunc (TP3_MT_STATINFO, gos_statmsg_cb, NULL);
    tp3_registcbfunc (TP3_MT_ENVINFO, gos_envmsg_cb, NULL);
    tp3_registcbfunc (TP3_MT_CONNAPPROVAL, gos_connmsg_cb, NULL);

    LOG(INFO) << "GOS set up data for TTA P3 library.";

    cf_db_close (db);

    return OK;
}

void on_timer_close_complete(uv_handle_t* handle)
{
    LOG(INFO) << "on_timer_close_complete.";
    CF_FREE(handle);
}

void
gos_finalize () {
    int i;
    gos_conninfo_t *pconn = gos_get_conninfo ();
    gos_server_t *pserver = gos_get_server ();
    uv_stream_t *handle;

    for (i = 0; i < pconn->size; i++) {
        handle = (pconn->pconn + i)->handle;
        gos_shutdown_stream (handle);
    }
    gos_shutdown ((uv_stream_t *)&(pserver->ttaserver));

    gos_release_config (&(gosdata.config));
    gos_release_server (&(gosdata.server));
    gos_release_devinfo (&(gosdata.devinfo));
    gos_release_conninfo (&(gosdata.conninfo));
    gos_release_rules (&(gosdata.ruleset));

}
