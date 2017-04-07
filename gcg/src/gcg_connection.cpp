
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>
#include <tp3.h>
#include <tp12.h>
#include <cf.h>

#include "gcg_base.h"
#include "gcg_config.h"
#include "gcg_connection.h"
#include "gcg_server.h"

void
gcg_init_conninfo (gcg_conninfo_t *pconn) {
	pconn->goshandle = NULL;

	cf_init_msgbuf (&(pconn->readbuf));
	cf_init_msgbuf (&(pconn->writebuf));

	pconn->nodecnt = 0;
	pconn->nodesize = 0;
	pconn->pnodeconns = NULL;
}

void
gcg_init_nodeconn (gcg_nodeconn_t *pnodeconn) {
	memset (pnodeconn->nodeid, 0, sizeof (int) * GCG_MAXNODE_IN_HANDLE);
	pnodeconn->handle = NULL;
	(pnodeconn->parser).Clear ();
}

void
gcg_release_conninfo (gcg_conninfo_t *pconn) {
	CF_FREE (pconn->pnodeconns);
}

cf_ret_t
gcg_set_gos_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
	pconn->goshandle = handle;
	return CF_OK;
}

cf_ret_t
gcg_set_node_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
	gcg_nodeconn_t *ptmp;

	(pconn->nodecnt) ++;
	if (pconn->nodesize < pconn->nodecnt)
		(pconn->nodesize) = pconn->nodecnt;

	ptmp = (gcg_nodeconn_t *) CF_REALLOC (pconn->pnodeconns, 
										sizeof (gcg_nodeconn_t) * (pconn->nodesize));
	if (ptmp == NULL) {
		CF_VERBOSE (CF_VERBOSE_LOW, "Fail to add new connection.");
		return CF_ERR;

	} else {
		pconn->pnodeconns = ptmp;
		ptmp = pconn->pnodeconns + pconn->nodecnt - 1;
		gcg_init_nodeconn (ptmp);
		ptmp->handle = handle;

		return CF_OK;
	}
}

gcg_nodeconn_t *
gcg_get_nodeconn (gcg_conninfo_t *pconn, uv_stream_t *handle) {
	int i; 
	gcg_nodeconn_t *ptmp;

	ptmp = pconn->pnodeconns;
	for (i = 0; i < pconn->nodecnt; i++, ptmp++) {
		if (ptmp->handle == handle) {
			return ptmp;
		}
	}
	return NULL;
}

uv_stream_t *
gcg_get_goshandle (gcg_conninfo_t *pconn) {
	return pconn->goshandle;	
}

uv_stream_t *
gcg_get_nodehandle (gcg_conninfo_t *pconn, int nodeid) {
	int i, j; 
	gcg_nodeconn_t *ptmp;

	ptmp = pconn->pnodeconns;
	for (i = 0; i < pconn->nodecnt; i++, ptmp++) {
		for (j = 0; j < GCG_MAXNODE_IN_HANDLE; j++) {
			if (ptmp->nodeid[j] == nodeid) {
				return ptmp->handle;
			}
		}
	}
	return NULL;
}

uv_stream_t *
gcg_get_nodehandle_by_index (gcg_conninfo_t *pconn, int index) {
	if (pconn->nodecnt <= index || index < 0)
		return NULL;
	return (pconn->pnodeconns + index)->handle;
}

cf_ret_t
gcg_set_node_connection_info (gcg_conninfo_t *pconn, uv_stream_t *handle, int nodeid) {
	int j;
	gcg_nodeconn_t *ptmp = gcg_get_nodeconn (pconn, handle);

	if (ptmp == NULL)
		return CF_ERR;

	for (j = 0; j < GCG_MAXNODE_IN_HANDLE; j++) {
		if (ptmp->nodeid[j] == 0) {
			ptmp->nodeid[j] = nodeid;
			return CF_OK;
		} else if (ptmp->nodeid[j] == nodeid) {
			return CF_OK;
		}
	}
	CF_ASSERT (0, "There is no room to save node id.");
	return CF_ERR;
}

int
gcg_is_gos (gcg_conninfo_t *pconn, uv_stream_s *handle) {
	if (handle == pconn->goshandle)
		return 1;
	else
		return 0;	
}

cf_msgbuf_t *
gcg_get_readmsgbuf  (gcg_conninfo_t *pconn) {
	return &(pconn->readbuf);
}

cf_msgbuf_t *
gcg_get_writemsgbuf  (gcg_conninfo_t *pconn) {
	return &(pconn->writebuf);
}

TTA12PacketParsing *
gcg_get_tta12parser (gcg_conninfo_t *pconn, uv_stream_t *handle) {
	gcg_nodeconn_t *ptmp = gcg_get_nodeconn (pconn, handle);
	if (ptmp == NULL)
		return NULL;
	return &(ptmp->parser);
}

cf_ret_t
gcg_remove_node_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
	gcg_nodeconn_t *ptmp;

	ptmp = gcg_get_nodeconn (pconn, handle);

	if (ptmp == NULL) {
		CF_VERBOSE (CF_VERBOSE_HIGH, "It tried to remove a connection, "
					"but there is no handle same with the input handle.");
		return CF_OK;
	}

	(pconn->nodecnt)--;
	memcpy (ptmp, pconn->pnodeconns + pconn->nodecnt, sizeof (gcg_nodeconn_t));
	return CF_OK;
}

cf_ret_t
gcg_remove_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
	if (pconn->goshandle == handle) {
		pconn->goshandle = NULL;
	} else {
		gcg_remove_node_connection (pconn, handle);
	}
	return CF_OK;
}

int
gcg_is_connected (gcg_conninfo_t *pconn) {
	if (pconn->goshandle != NULL && pconn->nodecnt > 0)
		return 1;
	return 0;
}

cf_ret_t
gcg_parseframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe) {
	int len;

	len = tp3_readframe (pframe, (byte *)cf_getbuf_msgbuf (pmsgbuf), cf_getlength_msgbuf (pmsgbuf));
	if (0 < len) {
		cf_setused_msgbuf (pmsgbuf, len);
		return CF_OK;
	}
	return CF_ERR;
}

cf_ret_t
gcg_writeframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe) {
	int size = tp3_getframesize (pframe);

	CF_ERR_RETURN (cf_resize_msgbuf (pmsgbuf, size), "mesage buffer frame writing failed.");

	CF_EXP_RETURN (
		TP3_SC_NOERROR != tp3_writeframe (pframe, (byte *)cf_getbuf_msgbuf (pmsgbuf), cf_getsize_msgbuf (pmsgbuf)),
		CF_ERR, "message buffer frame writing failed");
	cf_setlength_msgbuf (pmsgbuf, size);

	return CF_OK;
}

