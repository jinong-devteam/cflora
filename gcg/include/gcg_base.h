/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gcg_base.h
 * \brief GCG 기본 해더 파일. 기존 코드를 수정했음.
 */

#ifndef _GCG_BASE_H_
#define _GCG_BASE_H_

#include <glog/logging.h>

#define _GCG_BUF_LEN 128

#define _GCG_MAX_GID 16
#define _GCG_MAX_DEV_PER_NODE 16

#define _GCG_TEMP_SERIAL "1234-5678-9098-7654"
#define _GCG_TEMP_MODEL	"FARMOS-V.01"

#define ERR_RETURN(expr,msg)    \
    do {                        \
        if (ERR == (expr)) { \
            LOG(ERROR) << msg;  \
            return ERR;      \
        }                       \
    } while (0);

#endif
