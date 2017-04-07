#ifndef _CFLORA_CONTROL_H_
#define _CFLORA_CONTROL_H_

/**
 * 센서의 측정치를 하나 읽어온다. 단, ntime초 이내의 하나의 최근 데이터만 읽어온다.
 * @param pdb 데이터베이스 핸들러
 * @param sensorid 읽고자 하는 센서의 센서아이디
 * @param value 측정치를 담을 공간
 * @param ntime 시간 제한. 최근 ntime 초 이내의 데이터만 읽는다.
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_get_environment (cf_db_t *pdb, int sensorid, double *value, int ntime);

/**
 * 여러 센서의 측정치를 읽어온다. 단, ntime초 이내의 데이터의 평균을 읽어온다. 입력된 센서아이디값중 값이 없는 센서의 값은 읽지 않는다.
 * @param pdb 데이터베이스 핸들러
 * @param num 읽고자 하는 센서의 개수
 * @param sensorid 읽고자 하는 센서의 센서아이디들
 * @param rsensor 값이 있는 센서의 센서아이디들. 값이 없다면 -1로 세팅되어 리턴된다.
 * @param value 측정치를 담을 공간 배열
 * @param ntime 시간 제한. 최근 ntime 초 이내의 데이터만 읽는다.
 * @return 읽은 센서의 개수
 */
int
cf_get_environments (cf_db_t *pdb, int num, int *sensorid, int *rsensor, double *values, int ntime);

/**
 * 하나의 구동기를 제어하기 위한 명령을 내린다. (정확히는 저장한다.)
 * @param pdb 데이터베이스 핸들러
 * @param actuatorid 구동기 식별자
 * @param argument 구동기 작동 인자
 * @param wtime 구동기 작동 시간
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_execute_command (cf_db_t *pdb, int actuatorid, int argument, int wtime);

/**
 * 다수의 구동기를 제어하기 위한 명령을 내린다. (정확히는 저장한다.) 하나라도 (저장에) 실패하는 경우 전체 명령을 취소한다.
 * @param pdb 데이터베이스 핸들러
 * @param num 구동하고자 하는 구동기의 개수
 * @param actuatorid 구동기 식별자들
 * @param argument 구동기 작동 인자들
 * @param wtime 구동기 작동 시간들
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
cf_execute_commands (cf_db_t *pdb, int num, int *actuatorid, int *argument, int *wtime);


#endif
