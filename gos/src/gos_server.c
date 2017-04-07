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
#include "gos_control.h"
#include "gos_process.h"
#include "gos_rule.h"
#include "gos_server.h"

/** GOS 설정 및 내부데이터 */
static struct {
	gos_config_t config;        /** GOS 설정 */
	gos_server_t server;    	/** GOS 운영을 위한 내부 데이터 */
	gos_devinfo_t devinfo;      /** GOS와 통신하는 디바이스들의 현재정보 */
	gos_conninfo_t conninfo;    /** GOS에 접속된 GCG들과 통신하기위한 핸들정보 */
	gos_ruleset_t ruleset;		/** GOS 자동 운영을 위한 룰들의 집합 */
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
	if (rc != CF_OK) {          
		CF_ERR_LOG ("database query execution (check auto control) failed. %s", errmsg);
		cf_db_free (errmsg);      
		return 0;
	}
	strcpy (control, results[1]);
	cf_db_free_table (results);

	return (control[0] == 'a') ? 1 : 0;
}

cf_ret_t
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

	return CF_OK;
}

void
gos_close_timer (uv_handle_t *handle) {
	;
}

void 
gos_on_close(uv_handle_t *peer) {
	CF_VERBOSE (CF_VERBOSE_LOW, "GCG disconnected.");
	CF_FREE (peer);
}


void 
gos_after_write (uv_write_t* req, int status) {
	CF_EXP_VRETURN (status != 0, "uv_write error");
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
	CF_VERBOSE (CF_VERBOSE_HIGH, "read [%ld] characters from gcg.", (long)nread);

	if (nread < 0) {
		gos_shutdown_stream (handle);

	} else if (nread > 0) {
		cf_msgbuf_t *preadbuf = gos_get_readmsgbuf (&(gosdata.conninfo), handle);

		cf_append_msgbuf (preadbuf, buf->base, nread);
		while (CF_OK == gos_parseframe_msgbuf (preadbuf, &frame)) {
			CF_VERBOSE (CF_VERBOSE_HIGH, "parse frame [%d].", frame.length);
			if ( CF_ERR == gos_process_message (handle, &frame) ) {
				tp3_releaseframe (&frame);
				CF_FREE (buf->base);
				CF_EXP_VRETURN ( TRUE , "gos message processing failed.");
			}
			tp3_releaseframe (&frame);
		}
		CF_FREE (buf->base);
	}
}

cf_ret_t
gos_process_message (uv_stream_t *handle, tp3_frame_t *pframe) {
	tp3_frame_t res ;
	tp3_stat_t sc;
	cf_ret_t rc;
	gos_tp3arg_t arg;
	
	gos_set_tp3arg (&arg, handle);
	res.data = NULL ;
	sc = tp3_response (pframe, &res, (void *)&arg);
	if (sc != TP3_SC_NOERROR) {
		if ( res.data != NULL )
			tp3_releaseframe (&res);
		CF_ERR_RETURN (1, "gos message processing failed [%s].", tp3_geterrormsg (sc));
	}
	rc = gos_send_message (handle, &res);
	CF_VERBOSE (CF_VERBOSE_HIGH, "send response frame [%d].", res.length);
	tp3_releaseframe (&res);

	if (gos_get_remove_connection (gos_get_conninfo (), handle) == 1) {
		CF_VERBOSE (CF_VERBOSE_HIGH, "A connection will be lost.");
		gos_remove_connection (gos_get_conninfo (), handle);
		gos_shutdown_stream (handle);
	}
	return rc;
}

cf_ret_t
gos_send_message (uv_stream_t *handle, tp3_frame_t *pframe) {
	uv_write_t *req = (uv_write_t *) CF_MALLOC(sizeof (uv_write_t));
	uv_buf_t buf;
	cf_msgbuf_t *pwritebuf = gos_get_writemsgbuf (&(gosdata.conninfo), handle);

	if (CF_ERR == gos_writeframe_msgbuf (pwritebuf, pframe)) {
		CF_ERR_RETURN (1, "gos message writing failed.");
	}

	buf.base = cf_getbuf_msgbuf (pwritebuf);
	buf.len = cf_getlength_msgbuf (pwritebuf);

	CF_VERBOSE (CF_VERBOSE_HIGH, "write [%d] bytes.", buf.len);

	if (uv_write(req, handle, &buf, 1, gos_after_write)) {
		CF_ERR_RETURN (1, "gos message sending failed.");
	}
	return CF_OK;
}

void 
gos_buf_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = CF_MALLOC(suggested_size);
	buf->len = suggested_size;
}


void 
gos_on_connection(uv_stream_t *server, int status) {
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

	CF_VERBOSE (CF_VERBOSE_LOW, "GCG connected.");

	r = uv_read_start(stream, gos_buf_alloc, gos_after_read);
	CF_EXP_VRETURN (r, "Connection read error\n");

	gos_add_connection (gos_get_conninfo (), stream);
}

void 
gos_on_server_close(uv_handle_t* handle) {
	;
}

cf_ret_t
gos_timer_start (gos_server_t *pgos, gos_config_t *pconfig) {
	int r;
	r = uv_timer_init (uv_default_loop (), &(pgos->timer));
	CF_EXP_RETURN (r, CF_ERR, "Timer creation error\n");
	r = uv_timer_start (&(pgos->timer), gos_timer_cb, 0, pconfig->timer);
	CF_EXP_RETURN (r, CF_ERR, "Timer start error\n");

	return CF_OK;
}

cf_ret_t
gos_ttaserver_start (gos_server_t *pgos, gos_config_t *pconfig) {
	struct sockaddr_in addr;
	int r;

	r = uv_tcp_init(uv_default_loop (), &(pgos->ttaserver));
	CF_EXP_RETURN (r, CF_ERR, "Socket creation error\n");

	r = uv_ip4_addr("0.0.0.0", pconfig->port, &addr);
	CF_EXP_RETURN (r, CF_ERR, "Socket address error\n");

	r = uv_tcp_bind(&(pgos->ttaserver), (const struct sockaddr*) &addr, 0);
	CF_EXP_RETURN (r, CF_ERR, "Socket binding error\n");

	r = uv_listen((uv_stream_t *)&(pgos->ttaserver), SOMAXCONN, gos_on_connection);
	CF_EXP_RETURN (r, CF_ERR, "Socket listen error\n");

	return CF_OK;
}

cf_ret_t
gos_initialize (char *conffile) {
	cf_db_t *db = &(gos_get_config ()->db);

	CF_ERR_RETURN (gos_read_config (&(gosdata.config), conffile), "read config failed.");

	CF_VERBOSE (CF_VERBOSE_MID, "GOS read configuration [%s].", conffile);
	CF_ERR_RETURN (cf_db_open (db), "database open failed.");

	gos_init_server (&(gosdata.server));
	gos_init_devinfo (&(gosdata.devinfo), &(gosdata.config));
	gos_init_conninfo (&(gosdata.conninfo));
	gos_init_rules (&(gosdata.ruleset), &(gosdata.config));
	gos_init_control (&(gosdata.config));

	CF_VERBOSE (CF_VERBOSE_MID, "GOS set up server data.");

	tp3_initgos (gosdata.config.gosid, gosdata.config.gcgids, gosdata.config.numofgcg);

	tp3_registcbfunc (TP3_MT_STATINFO, gos_statmsg_cb, NULL);
	tp3_registcbfunc (TP3_MT_ENVINFO, gos_envmsg_cb, NULL);
	tp3_registcbfunc (TP3_MT_CONNAPPROVAL, gos_connmsg_cb, NULL);

	CF_VERBOSE (CF_VERBOSE_MID, "GOS set up data for TTA P3 library.");

	cf_db_close (db);

	return CF_OK;
}

void on_timer_close_complete(uv_handle_t* handle)
{
	CF_VERBOSE (CF_VERBOSE_LOW, "on_timer_close_complete.");
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
