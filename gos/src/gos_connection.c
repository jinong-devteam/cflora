
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <tp3.h>
#include <cf.h>

#include "gos_base.h"
#include "gos_connection.h"

#define _GOS_STEP	1

void
gos_init_conninfo (gos_conninfo_t *pconn) {
	pconn->pconn = NULL;
	pconn->size = 0;
	pconn->len = 0;
}

void
gos_release_conninfo (gos_conninfo_t *pconn) {
	CF_FREE (pconn->pconn);
}

cf_ret_t
gos_add_connection (gos_conninfo_t *pconn, uv_stream_t *handle) {
	gos_conn_t *ptmp = NULL;
	int value = 87380 ;
	uv_recv_buffer_size((uv_handle_t *)handle, &value);	
	uv_send_buffer_size((uv_handle_t *)handle, &value);

	if (pconn->len >= pconn->size) {
		ptmp = (gos_conn_t *) CF_REALLOC (pconn->pconn, sizeof (gos_conn_t) * (pconn->size + _GOS_STEP));
		CF_ERR_RETURN (ptmp == NULL, "connection reallocation failed.");
		pconn->size += _GOS_STEP;
		pconn->pconn = ptmp;
	} 
	value = 0 ;

	ptmp = pconn->pconn + pconn->len;
	ptmp->handle = handle;
	cf_init_msgbuf (&(ptmp->readbuf));
	cf_init_msgbuf (&(ptmp->writebuf));
	ptmp->gcgid = _GOS_NO_ID;
	ptmp->setremove = 0;

	pconn->len += 1;

	return CF_OK;
}

gos_conn_t *
gos_find_connection (gos_conninfo_t *pconn, int gcgid) {
	int i;
	
	for (i = 0; i < pconn->len; i++) {
		if ((pconn->pconn)[i].gcgid == gcgid)
			return pconn->pconn + i;
	}
	return NULL;
}

gos_conn_t *
gos_find_connection_by_handle (gos_conninfo_t *pconn, uv_stream_t *handle) {
	int i;
	
	for (i = 0; i < pconn->len; i++) {
		if ((pconn->pconn)[i].handle == handle)
			return pconn->pconn + i;
	}
	return NULL;
}

cf_ret_t
gos_set_gcgid (gos_conninfo_t *pconninfo, uv_stream_t *handle, int gcgid) {
	gos_conn_t *pconn = gos_find_connection_by_handle (pconninfo, handle);
	CF_ERR_RETURN (pconn == NULL, "Finding connection failed.");
	pconn->gcgid = gcgid;
	return CF_OK;
}

cf_msgbuf_t *
gos_get_readmsgbuf  (gos_conninfo_t *pconninfo, uv_stream_t *handle) {
	gos_conn_t *pconn = gos_find_connection_by_handle (pconninfo, handle);
	if (pconn == NULL) {
		CF_ERR_LOG ("Finding connection failed.");
		return NULL;
	}
	return &(pconn->readbuf);
}

cf_msgbuf_t *
gos_get_writemsgbuf  (gos_conninfo_t *pconninfo, uv_stream_t *handle) {
	gos_conn_t *pconn = gos_find_connection_by_handle (pconninfo, handle);
	if (pconn == NULL) {
		CF_ERR_LOG ("Finding connection failed.");
		return NULL;
	}
	return &(pconn->writebuf);
}

int
gos_get_remove_connection (gos_conninfo_t *pconn, uv_stream_t *handle) {
	int i;
	
	for (i = 0; i < pconn->len; i++) {
		if ((pconn->pconn)[i].handle == handle) {
			return (pconn->pconn)[i].setremove;
		}
	}	
	return 0;
}

void
gos_set_remove_connection (gos_conninfo_t *pconn, uv_stream_t *handle) {
	int i;
	
	for (i = 0; i < pconn->len; i++) {
		if ((pconn->pconn)[i].handle == handle) {
			(pconn->pconn)[i].setremove = 1;
			break;
		}
	}	
}

cf_ret_t
gos_remove_connection (gos_conninfo_t *pconn, uv_stream_t *handle) {
	int i;
	
	for (i = 0; i < pconn->len; i++) {
		if ((pconn->pconn)[i].handle == handle) {
			memcpy (pconn->pconn + i, pconn->pconn + pconn->len, sizeof(gos_conn_t));
			pconn->len -= 1;
			return CF_OK;
		}
	}
	return CF_ERR;
}

void
gos_releaseconninfo (gos_conninfo_t *pconn) {
	CF_FREE (pconn->pconn);
}

cf_ret_t
gos_parseframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe) {
	int len;

	len = tp3_readframe (pframe, (byte *)cf_getbuf_msgbuf (pmsgbuf), cf_getlength_msgbuf (pmsgbuf));
	if (0 < len) {
		CF_VERBOSE (CF_VERBOSE_HIGH, "read frame [bytes : %d].", len);
		cf_setused_msgbuf (pmsgbuf, len);
		return CF_OK;
	}
	return CF_ERR;
}

cf_ret_t
gos_writeframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe) {
	int size = tp3_getframesize (pframe);

	CF_ERR_RETURN (cf_resize_msgbuf (pmsgbuf, size), "mesage buffer frame writing failed.");

	CF_EXP_RETURN (TP3_SC_NOERROR != tp3_writeframe (pframe, (byte *)cf_getbuf_msgbuf (pmsgbuf), cf_getsize_msgbuf (pmsgbuf)),
			CF_ERR, "message buffer frame writing failed");
	cf_setlength_msgbuf (pmsgbuf, size);

	return CF_OK;
}

