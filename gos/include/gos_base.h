/**
 * Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
 * \file gos_base.h
 * \brief GOS 기본 해더 파일. 기존 코드를 수정했음.
 */

#ifndef _GOS_BASE_H_
#define _GOS_BASE_H_

#include <glog/logging.h>

#include <cf.h>

/** 없는 아이디를 표현하기 위한 정의 */
#define _GOS_NO_ID		-1
/** 한노드가 가질 수 있는 최대 디바이스의 개수 */
#define _GOS_MAX_DEV	256
/** 내부적으로 사용되는 버퍼의 크기 */
#define _GOS_BUF_LEN	256
#define _GOS_BBUF_LEN	512
#define _GOS_BBBUF_LEN	1024

/** 설치할 수 있는 최대 구동기 수 */
#define _GOS_MAX_ACTUATOR	64

#endif

