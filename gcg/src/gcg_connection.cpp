/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gcg_connection.cpp
 * \brief GCG 통신관련 코드. 기존 코드를 수정했음.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>
#include <tp3.h>
#include <gnodehelper.h>
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
    cf_init_msgbuf (&(pnodeconn->readbuf));
    cf_init_msgbuf (&(pnodeconn->writebuf));
}

void
gcg_release_conninfo (gcg_conninfo_t *pconn) {
    CF_FREE (pconn->pnodeconns);
}

ret_t
gcg_set_gos_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
    pconn->goshandle = handle;
    return OK;
}

ret_t
gcg_set_node_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
    gcg_nodeconn_t *ptmp;

    (pconn->nodecnt) ++;
    if (pconn->nodesize < pconn->nodecnt)
        (pconn->nodesize) = pconn->nodecnt;

    ptmp = (gcg_nodeconn_t *) CF_REALLOC (pconn->pnodeconns, 
                                        sizeof (gcg_nodeconn_t) * (pconn->nodesize));
    if (ptmp == NULL) {
        LOG (WARNING) << "Fail to add new connection." << handle;
        return ERR;

    } else {
        pconn->pnodeconns = ptmp;
        ptmp = pconn->pnodeconns + pconn->nodecnt - 1;
        gcg_init_nodeconn (ptmp);
        ptmp->handle = handle;

        return OK;
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

ret_t
gcg_set_node_connection_info (gcg_conninfo_t *pconn, uv_stream_t *handle, int nodeid) {
    int j;
    gcg_nodeconn_t *ptmp = gcg_get_nodeconn (pconn, handle);

    if (ptmp == NULL)
        return ERR;

    for (j = 0; j < GCG_MAXNODE_IN_HANDLE; j++) {
        if (ptmp->nodeid[j] == 0) {
            ptmp->nodeid[j] = nodeid;
            return OK;
        } else if (ptmp->nodeid[j] == nodeid) {
            return OK;
        }
    }
    CHECK(0) << "There is no room to save node id.";
    return ERR;
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
gcg_get_writemsgbuf (gcg_conninfo_t *pconn) {
    return &(pconn->writebuf);
}

cf_msgbuf_t *
gcg_get_node_readmsgbuf (gcg_conninfo_t *pconn, uv_stream_t *handle) {
    gcg_nodeconn_t *ptmp;
    ptmp = gcg_get_nodeconn (pconn, handle);
    return &(ptmp->readbuf);
}

cf_msgbuf_t *
gcg_get_node_writemsgbuf (gcg_conninfo_t *pconn, uv_stream_t *handle) {
    gcg_nodeconn_t *ptmp;
    ptmp = gcg_get_nodeconn (pconn, handle);
    return &(ptmp->writebuf);
}

ret_t
gcg_remove_node_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
    gcg_nodeconn_t *ptmp;

    ptmp = gcg_get_nodeconn (pconn, handle);

    if (ptmp == NULL) {
        LOG(INFO) << "It tried to remove a connection, "
                  << "but there is no handle same with the input handle.";
        return OK;
    }

    (pconn->nodecnt)--;
    memcpy (ptmp, pconn->pnodeconns + pconn->nodecnt, sizeof (gcg_nodeconn_t));
    return OK;
}

ret_t
gcg_remove_connection (gcg_conninfo_t *pconn, uv_stream_t *handle) {
    if (pconn->goshandle == handle) {
        pconn->goshandle = NULL;
    } else {
        gcg_remove_node_connection (pconn, handle);
    }
    return OK;
}

int
gcg_is_connected (gcg_conninfo_t *pconn) {
    if (pconn->goshandle != NULL && pconn->nodecnt > 0)
        return 1;
    return 0;
}

ret_t
gcg_parseframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe) {
    int len;

    len = tp3_readframe (pframe, (byte *)cf_getbuf_msgbuf (pmsgbuf), cf_getlength_msgbuf (pmsgbuf));
    if (0 < len) {
        cf_setused_msgbuf (pmsgbuf, len);
        return OK;
    }
    return ERR;
}

ret_t
gcg_writeframe_msgbuf (cf_msgbuf_t *pmsgbuf, tp3_frame_t *pframe) {
    int size = tp3_getframesize (pframe);

    ERR_RETURN (cf_resize_msgbuf (pmsgbuf, size), "mesage buffer frame writing failed.");

    if (TP3_SC_NOERROR != tp3_writeframe (pframe, (byte *)cf_getbuf_msgbuf (pmsgbuf), cf_getsize_msgbuf (pmsgbuf))) {
        LOG (ERROR) << "message buffer frame writing failed";
        return ERR;
    }

    cf_setlength_msgbuf (pmsgbuf, size);

    return OK;
}

