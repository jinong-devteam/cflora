#ifndef _GOS_DEVICE_H_
#define _GOS_DEVICE_H_

/** 디바이스 상태 */
typedef enum {
	GOS_DEVST_ACTIVATED = 0,
	GOS_DEVST_INSTALLED = 1,
	GOS_DEVST_DETECTED = 2,
	GOS_DEVST_SUSPENDED = 3,
	GOS_DEVST_ABNORMAL = 4,
	GOS_DEVST_DISCONNECTED = 5
} gos_devstat_t;

#define _GOS_DEVSTAT_MAX	6

/** 디바이스 종류 */
typedef enum {
	GOS_DEV_GOS = 0,
	GOS_DEV_GCG = 1,
	GOS_DEV_SNODE = 2,
	GOS_DEV_ANODE = 3,
	GOS_DEV_SENSOR = 4,
	GOS_DEV_ACTUATOR = 5,
	GOS_DEV_CCTV = 6,
	GOS_DEV_VSENSOR = 7,
	GOS_DEV_ASENSOR = 8,
	GOS_DEV_NACTUATOR = 9,
	GOS_DEV_UNKNOWN = 10 
} gos_devtype_t;

#define _GOS_DEVTYPE_MAX	10

/** 센서 데이터 변환 방식 */
typedef enum {
	GOS_CVT_NONE = 0,
	GOS_CVT_LINEAR = 1,
	GOS_CVT_TABLEMAP = 2,
	GOS_CVT_RATIO = 3,
	GOS_CVT_ORATIO = 4,
	GOS_CVT_VSENSOR = 5,
} gos_cvt_t;

#define _GOS_CVTTYPE_MAX	6

#define _GOS_ACTARG_BIT			12
#define _GOS_ACTARG_TIMEFILTER	0xFFF		
#define _GOS_ACTARG_ARGFILTER	0xF000

typedef enum {
	GOS_ASENSOR_LIMITER = 0,	///< 리미터
	GOS_ASENSOR_ANGLE = 1		///< 구현되어 있지는 않지만 추후에 사용할 수 있음
} gos_asen_t;

#define _GOS_ASENSOR_MAX	2

#define GOS_SIGNAL_OFF	0

#define _GOS_NOT_ASENSOR	-1	
#define _GOS_MAX_ASENSOR	2

#define _GOS_FORCESTOP_CTRLID	-1
#define _GOS_LIMITSTOP_CTRLID 	-2

#define _GOS_FORCESTOP_CTRLTYPE	"auto-forcestop"
#define _GOS_LIMITSTOP_CTRLTYPE	"auto-limitstop"

#define _GOS_MOTOR_ARG_OPEN		1
#define _GOS_MOTOR_ARG_CLOSE	2

#define _GOS_RESENT_IGNORE_COUNT	8

typedef enum {
	GOS_CMD_NONE = 0,				    ///> 명령이 없는 상태 (디비에는 있을 수 있음)
	GOS_CMD_IGNORED_WRONGCOMMAND = 1,	///> 명령이 잘못된 명령이기 때문에 무시된 상태
	GOS_CMD_IGNORED_BY_NEWCOMMAND = 2,	///> 명령이 새로운 명령으로 무시된 상태
	GOS_CMD_IGNORED_BY_AUTOCONTROL = 3,	///> 명령이 수동 명령이라 자동동작상태에서는 무시된 상태
	GOS_CMD_WAITING = 11,				///> 명령이 대기중인 상태
	GOS_CMD_SENT = 12,					///> 명령이 전송된 상태
	GOS_CMD_WORKING = 13,				///> 명령이 작동중인 상태
	GOS_CMD_STOPPING_BY_ASENSOR = 21,	///> 명령이 리미터에 의해 중지 예정인 상태
	GOS_CMD_STOPPING_BY_STOPCOMMAND = 22,///> 명령이 중지명령에 의해 중지 예정인 상태
	GOS_CMD_FINISHED_NORMALLY = 31,		///> 명령이 정상적으로 종료된 상태
	GOS_CMD_FINISHED_BY_ASENSOR = 32,	///> 명령이 리미터에 의해 종료된 상태
	GOS_CMD_FINISHED_BY_STOPCOMMAND = 33,///> 명령이 중지명령에 의해 중지된 상태
	GOS_CMD_STATMAX
} gos_cmdstat_t;

typedef struct {
	int id;				///> 디비 테이블의 제어명령 ID
    int exectm;			///> 제어명령 실행시각 (입력시각)
    int senttm;			///> 제어명령 전송시각 
    int deviceid;		///> 제어기 아이디

    int arg;			///> 제어인자
    int tm;				///> 작동시간
    int ctrlarg;		///> 로우레벨제어인자

    int ctrlid;			///> 자동일경우 자동 제어룰 아이디, 수동일 경우 UI에서 필요한 적당한 값을 넣어 사용할 수 있음.
    char ctrltype[_GOS_BUF_LEN];    ///> 수동일경우 ‘manual’, 자동일 경우 ‘auto’ 로 시작하고 적당한 내용을 붙일 수 있음.

    int workcnt;		///> 동작 명령을 보낸 회수
    int stopcnt;		///> 정지 명령을 보낸 회수
    int resentignore;	///> 동작 명령을 재전송시 딜레이 카운트
    gos_cmdstat_t stat;	///> 제어 명령의 상태
} gos_cmd_t;

/** 구동기 제어를 위한 데이터 */
typedef struct {
	int used_asensor;					///< asensor 사용여부
	int asensorid[_GOS_MAX_ASENSOR];	///< asensor id
	gos_asen_t type[_GOS_MAX_ASENSOR];	///< asensor type

	int totaltime;						///< 구동기 전체 작동시간
	int postime;						///< 구동기 작동시간 - 방향을 +/-로 감안하여 계산한 시간
	gos_cmd_t waitingcmd;				///< 대기중인 제어명령
	gos_cmd_t currentcmd;				///< 실행중인 제어명령
} gos_driver_t;

/** 단일 디바이스 정보구조체 */
typedef struct {
	int id;					///< 디바이스 아이디 (디비 기준)
	gos_devtype_t type;		///< 디바이스 종류

	gos_devstat_t status;	///< 디바이스 상태
	int ischanged;			///< 디바이스 상태 변경 여부

	double nvalue;			///< 가장 최신의 값 (변환)
	int rawvalue;			///< 가장 최신의 값 (로우)
	int isupdated;			///< 디바이스 값 변경 여부

	// for TTA
	int gcgid;				///< TTA 를 위한 GCG ID
	int nodeid;				///< TTA 를 위한 NODE ID
	int deviceid;			///< TTA 를 위한 Sensor ID or Actuator ID

	// for field control => by chj 2015.10.20 제어 위치 정보 필요(1구역,2구역, 양액기 등등 )
	int field_id ;
	char type_name[21] ;

	// for sensor & vsensor
	gos_cvt_t cvt;			///< 센서인경우 데이터 변환방식
	void *cvtarg;			///< 데이터 변환 인자 gos_cvt_linear_arg_t, gos_cvt_vsensor_arg_t

	// for actuator
	gos_driver_t driver;	///< 액츄에이터인 경우 컨트롤 드라이버
} gos_dev_t;

/** 전체 디바이스 정보 구조체 */
typedef struct {
	int size;				///< 디바이스의 개수
	gos_dev_t *pdev;		///< 디바이스 정보 구조체 배열의 포인터
	//int needsync;			///< 변동시 싱크 필요여부
} gos_devinfo_t;


/** 센서 데이터 변환을 위한 구간 정보 구조체 */
typedef struct {
	double from;			///< 입력 시작점 (<=)
	double to;				///< 입력 종료점 (<=)
	double slope;			///< 기울기
	double intercept;		///< y 절편 (시작점을 0으로 환산하여 처리하므로 실제로는 시작점의 값, GOS_CVT_TABLEMAP 에서는 리턴값)
} gos_cvt_segment_t;

/** 센서 데이터 선형보간을 위한 구조체 */
typedef struct {
	int nseg;					///< 구간정보의 개수
	gos_cvt_segment_t *pseg;	///< 구간정보의 배열
	double offset;				///< offset (동일한 제품의 센서라도 개별 센서의 차이가 있을수 있기 때문에 넣는 오프셋 정보)
} gos_cvt_linear_arg_t;

/** 가상 센서의 상태를 계산하기 위한 함수 포인터 - SUSPEND 상태는 고려할 필요 없음 */
typedef cf_ret_t (*_gos_get_vsensor_stat_func) (gos_dev_t *pself, gos_devinfo_t *pdevinfo, void *config, gos_devstat_t *stat);
/** 가상 센서의 값을 계산하기 위한 함수 포인터 */
typedef cf_ret_t (*_gos_get_vsensor_value_func) (gos_devinfo_t *pdevinfo, void *config, double *nvalue);
/** 가상 센서의 설정을 해제하기 위한 함수 포인터 */
typedef void (*_gos_release_vsensor_config_func) (void *config);

/** 가상 센서의 값을 계산하기 위한 정보 구조체 */
typedef struct {
	void *config;           ///< 설정정보 구조체의 포인터 (개별 센서마다 필요한 것을 만들어 사용)
	double offset;			///< offset 
	_gos_get_vsensor_value_func _get_value;    		///< 다른 실센서의 데이터와 설정정보를 활용하여 가상센서의 값을 계산해내는 함수
	_gos_get_vsensor_stat_func _get_stat;     		///< 다른 실센서의 데이터와 설정정보를 활용하여 가상센서의 상태를 계산해내는 함수
	_gos_release_vsensor_config_func _release_config;	///< 설정정보를 해제하는 함수
} gos_cvt_vsensor_arg_t;

/**
 * 디바이스 정보를 초기화 한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_init_devinfo (gos_devinfo_t *pdevinfo, gos_config_t *pconfig);

/**
 * 디바이스 정보를 해제한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 */
void
gos_release_devinfo (gos_devinfo_t *pdevinfo);

/**
 * 디바이스 정보에 존재하는 디바이스 인지를 확인한다. 없다면 입력한 디바이스 정보는  새로운 디바이스의 정보이다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param pdev 확인하고자 하는 디바이스정보
 * @return 디바이스 정보에 있다면 새로운 디바이스가 아니므로 0 (false), 없다면 1(true)
 */
int
gos_is_newdevice (gos_devinfo_t *pdevinfo, gos_dev_t *pdev);

/*
cf_ret_t
gos_add_devices (gos_devinfo_t *pdevinfo, gos_config_t *pconfig, int size, gos_dev_t *pdev);
*/

/**
 * 디바이스 정보에 디바이스를 추가한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param pdev 추가하고자 하는 디바이스정보
 * @return 추가된 디바이스에 대한 포인터. 인자로 전달된 디바이스 포인터 아님.
 */
gos_dev_t *
gos_add_device (gos_devinfo_t *pdevinfo, gos_dev_t *pdev);

/**
 * 디바이스 정보를 디비와 동기화 한다. 
 * 2015.06.24 : 설정파일에 기준하기 때문에 사용할 일이 없을 것으로 예상됨
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_sync_devices (gos_devinfo_t *pdevinfo, gos_config_t *pconfig);

/**
 * 디바이스 정보에서 디바이스를 찾는다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param deviceid 디바이스 식별자
 * @return 찾은 디바이스에 대한 포인터
 */
gos_dev_t *
gos_find_device (gos_devinfo_t *pdevinfo, int deviceid);

/**
 * TTA ID를 기반으로 디바이스 정보에서 디바이스를 찾는다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param gcgid GCG 식별자
 * @param nodeid 노드 식별자
 * @param deviceid 센서 혹은 액츄에이터 식별자
 * @return 찾은 디바이스에 대한 포인터
 * @see gos_get_device
 */
gos_dev_t *
gos_find_device_by_tta (gos_devinfo_t *pdevinfo, int gcgid, int nodeid, int deviceid);

/**
 * TTA ID를 기반으로 디바이스 정보에서 디바이스를 찾는다. 단, 없으면 추가한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param devtype 디바이스 종류
 * @param gcgid GCG 식별자
 * @param nodeid 노드 식별자
 * @param deviceid 센서 혹은 액츄에이터 식별자
 * @return 찾거나 추가한 디바이스에 대한 포인터
 * @see gos_find_device_by_tta
 */
gos_dev_t *
gos_get_device (gos_devinfo_t *pdevinfo, gos_devtype_t devtype, int gcgid, int nodeid, int deviceid);

/**
 * 디바이스 상태정보와 환경정보를 디비에 기록한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param pconfig 설정 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_write_devinfo (gos_devinfo_t *pdevinfo, gos_config_t *pconfig);

/**
 * 구동기의 구동 상황(환경정보)를 디비에 업데이트한다.
 * @param pdev 디바이스 구조체의 포인터
 * @param db 데이터베이스 커넥션 정보
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_update_actuator_status (gos_dev_t *pdev, cf_db_t *db);

/**
 * 디바이스 환경정보를 디비에 기록한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param db 데이터베이스 커넥션 정보
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_write_env (gos_devinfo_t *pdevinfo, cf_db_t *db);

/**
 * 디바이스 상태정보를 디비에 기록한다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param db 데이터베이스 커넥션 정보
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_write_stat (gos_devinfo_t *pdevinfo, cf_db_t *db);


/**
 * 단일 디바이스 정보를 세팅한다.
 * @param pdev 단일 디바이스 정보 구조체의 포인터
 * @param devtype 디바이스 종류
 * @param gcgid GCG 식별자
 * @param nodeid 노드 식별자
 * @param deviceid 센서 혹은 액츄에이터 식별자
 */
void
gos_set_device (gos_dev_t *pdev, gos_devtype_t type, int gcgid, int nodeid, int deviceid);

/**
 * 디바이스 정보에 환경정보를 업데이트한다. 디비에 기록되지는 않는다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param nvalue 환경정보값
 * @param rawvalue 환경정보값 (RAW)
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_update_env (gos_dev_t *pdev, double nvalue, int rawvalue);

/**
 * 디바이스 정보에 상태정보를 업데이트한다. 디비에 기록되지는 않는다.
 * @param pdevinfo 디바이스 정보 구조체의 포인터
 * @param status 상태정보값
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_update_stat (gos_dev_t *pdev, int status);

/**
 * 센서의 출력을 실제 값으로 변환해 준다.
 * @param pdev 디바이스 구조체의 포인터
 * @param raw 센서의 출력
 * @param nvalue 변환된 실제값의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_convert_env (gos_dev_t *pdev, int raw, double *nvalue);

/**
 * 센서의 출력을 변환하기위한 정보를 디비로부터 로드한다.
 * @param pdev 디바이스 구조체의 포인터
 * @param db 데이터베이스 커넥션 정보
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_load_convertinfo (gos_dev_t *pdev, cf_db_t *db);

/**
 * 구동기 제어를 위한 센서정보를 디비로부터 로드한다.
 * @param pdev 디바이스 구조체의 포인터
 * @param db 데이터베이스 커넥션 정보
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_load_driver (gos_dev_t *pdev, cf_db_t *db);

/*
void
gos_release_convertinfo (gos_cvt_linear_arg_t *parg);
*/

/**
 * 구동기 제어를 위한 raw level 인자를 세팅한다.
 * @param arg 인자값  
 * @param worktime  인자값  
 * @return raw level 인자
 */
int
gos_get_actuator_argument (int arg, int worktime);

/**
 * 구동기 제어 상태를 읽는다.
 * @param rawvalue raw level 인자
 * @param arg 인자값  
 * @param worktime  인자값  
 */
void
gos_parse_actuator_argument (int rawvalue, int *arg, int *worktime);

/**
 * 구동기 제어명령을 처리한다. 
 * @param pdevinfo 장치정보 구조체의 포인터
 * @param pconfig 설정 정보 포인터
 * @param pconninfo 접속정보 구조체의 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_control (gos_devinfo_t *pdevinfo, gos_config_t *pconfig, gos_conninfo_t *pconninfo);

/**
 * 구동기 초기 제어명령을 처리한다. 
 * @param pconfig 설정 정보 포인터
 * @return 에러라면 CF_ERR, 정상완료라면 CF_OK
 */
cf_ret_t
gos_init_control (gos_config_t *pconfig);

#endif
