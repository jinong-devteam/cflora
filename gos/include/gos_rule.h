#ifndef _GOS_RULE_H_
#define _GOS_RULE_H_

/** 룰의 표현식을 확인하기 위한 연산자 */
typedef enum {
	GOS_ROP_GT = 0,			///< <
	GOS_ROP_LT = 1,			///< >
	GOS_ROP_EQ = 2,			///< ==
	GOS_ROP_GTE = 3,		///< <=
	GOS_ROP_LTE = 4,		///< >=
	GOS_ROP_NEQ = 5			///< !=
} gos_rop_t;

#define CF_ROP_MAX 6

/** 룰의 표현식의 실제 연산을 위한 함수 정의 */
typedef int (*gos_rexp_func) (double nvalue, int ndiff, void *arg, int span);

/** 룰의 표현식을 위한 디폴트 피연산자 구조체 */
typedef struct {
	double arg1;	///< 첫번째 피연산자 (시작시간을 위한 값)
	double arg2;	///< 두번째 피연산자 (종료시간을 위한 값)
} gos_rarg_t;

/** 룰의 표현식을 위한 구조체 */
typedef struct {
	int op;			///< 표현식 연산자
	gos_rarg_t arg;	///< 표현식 피연산자
	int sensorid;	///< 센서 식별자
} gos_rexp_t;

/** 룰의 시간범위 구조체 */
typedef struct {
	int from;		///< 시작시간 (초), 0시0분0초부터의 초
	int to;			///< 종료시간 (초)
	int exp_start;	///< 해당 시간범위에 적용될 표현식의 시작인덱스
	int exp_end;	///< 해당 시간범위에 적용될 표현식의 끝인덱스 (포함하지 않음)
} gos_rtimespan_t;

/** 룰 매치시 구동명령을 위한 구조체 */
typedef struct {
	int actuatorid;	///< 구동기 식별자
	int arg;		///< 구동 인자
	int wtime;		///< 구동 시간
} gos_ract_t;

/** 룰 구조체 */
typedef struct {
	int id;					///< 룰 식별자
	int priority;			///< 룰 우선순위
	int period;				///< 룰 적용대기시간
	int lastexec;			///< 룰 최종 적용시간
	int status;				///< 1 : activated, 0 : deactivated, -1 : error

	int numofact;			///< 룰 구동명령의 개수
	int actsidx;			///< 룰 구동명령 인덱스. 이 값보다 작으면 표현식 테스트 실패시 구동할 명령들이고, 이 값보다 크면 표현식 테스트 성공시 구동할 명령들
	gos_ract_t *pactions;	///< 전체 룰 구동명령

	int numofts;			///< 룰 시간범위의 개수
	gos_rtimespan_t *ptimes;///< 룰 시간범위들

	int numofexp;			///< 룰 표현식의 개수
	gos_rexp_t *pexps;		///< 룰 표현식들
	int *psatisfied;		///< 룰 표현식의 상태
} gos_rule_t;

/** 룰 세트 구조체 */
typedef struct {
	int numofrules;			///< 룰의 개수
	gos_rule_t *prules;		///< 룰 구조체의 배열
} gos_ruleset_t;

/**
 * 룰 정보를 초기화 한다.
 * @param pruelset 룰 세트 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_init_rules (gos_ruleset_t *pruleset, gos_config_t *pconfig);

/**
 * 룰 정보를 디비로부터 로드한다.
 * @param pruelset 룰 세트 구조체의 포인터
 * @param pdb 디비 핸들러의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_load_rules (gos_ruleset_t *pruleset, cf_db_t *pdb);

/**
 * 룰 정보를 해제한다.
 * @param pruelset 룰 세트 구조체의 포인터
 */
void
gos_release_rules (gos_ruleset_t *pruleset);

 
/**
 * 센서 측정값을 이용한 테스트 결과를 초기화한다.
 * @param pruelset 룰 세트 구조체의 포인터
 */
void
gos_reset_rulecondition (gos_ruleset_t *pruleset);

/**
 * 센서 측정값을 이용하여 룰을 테스트한다.
 * @param pruelset 룰 세트 구조체의 포인터
 * @param sensorid 센서 식별자
 * @param nvalue 센서의 측정치
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
void
gos_test_rules (gos_ruleset_t *pruleset, int sensorid, double nvalue);

/**
 * 룰을 평가한다.
 * @param pruelset 룰 세트 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_evaluate_rules (gos_ruleset_t *pruleset, gos_config_t *pconfig);

#endif

