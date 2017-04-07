
#ifndef _CFLORA_MESSAGE_BUFFER_H_
#define _CFLORA_MESSAGE_BUFFER_H_

#define _CFLORA_MSGBUF_STEP    1000

/** 스트림 메세지 수신을 위한 버퍼 */
typedef struct {
	char *buf;      /** 버퍼의 포인터 */
	int bufsize;    /** 버퍼에 할당된 메모리의 실제 크기 */
	int len;        /** 저장된 메세지의 길이 */
	int idx;        /** 버퍼의 시작지점 */
} cf_msgbuf_t;

/**
 * 메세지 버퍼를 초기화 한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_init_msgbuf (cf_msgbuf_t *pmsgbuf);

/**
 * 메세지 버퍼에 문자들을 추가한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param buf 추가할 문자열
 * @param len 추가할 문자열의 길이
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_append_msgbuf (cf_msgbuf_t *pmsgbuf, char *buf, int len);

/**
 * 메세지버퍼내 사용가능한 글자들의 수를 세팅한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param length 메세지 버퍼내 사용가능한 글자들의 수
 * @see cf_getlength_msgbuf
 */
void
cf_setlength_msgbuf (cf_msgbuf_t *pmsgbuf, int length);

/**
 * 메세지버퍼내 사용가능한 글자들의 수를 확인한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @return 메세지 버퍼내 사용가능한 글자들의 수
 * @see cf_getsize_msgbuf
 */
int
cf_getlength_msgbuf (cf_msgbuf_t *pmsgbuf);

/**
 * 메세지버퍼의 크기를 확인한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @return 메세지 버퍼의 크기
 * @see cf_getlength_msgbuf
 */
int
cf_getsize_msgbuf (cf_msgbuf_t *pmsgbuf);

/**
 * 메세지버퍼내 문자열을 꺼낸다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @return 메세지 버퍼내 문자열의 포인터
 */
char *
cf_getbuf_msgbuf (cf_msgbuf_t *pmsgbuf);

/**
 * 메세지버퍼에서 메세지를 사용한 결과를 세팅한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param usedlen 사용한 글자수
 */
void
cf_setused_msgbuf (cf_msgbuf_t *pmsgbuf, int usedlen);

/**
 * 메세지버퍼를 해제한다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 */
void
cf_release_msgbuf (cf_msgbuf_t *pmsgbuf);

/**
 * 메세지 버퍼의 사이즈를 재설정한다. 재설정시 원래 들어있던 값들은 모두 사라진다.
 * @param pmsgbuf 메세지버퍼 구조체의 포인터
 * @param size 원하는 길이
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_resize_msgbuf (cf_msgbuf_t *pmsgbuf, int size);

#endif
