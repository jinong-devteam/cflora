# 제어 알고리즘 처리 방법

## 개요

farmos 는 기본적으로 제어알고리즘을 처리하는 알고리즘을 가지고 있지는 않다. 하지만 몇가지 방법을 통해서 제어알고리즘을 추가하는 것이 가능하다.

1. 자체 룰 사용
1. 외부 프로그램 사용

## 자체 룰 사용
farmos 는 구동되는 시점에 디비에서 룰을 읽어서 이를 적용하는 기능이 있다. 자체적으로 지원하는 룰은 꽤 강력하기때문에 이를 사용해서 손쉽게 제어가 가능하다.

### 룰 관련 테이블
1. gos_control_rule
 룰의 기본적인 정보가 들어있는 테이블이다. id, 우선순위, period 가 주된 데이터이다.
1. gos_control_rule_condition
 룰이 적용되는 시점을 판단하기 위한 정보가 들어있는 테이블이다. 판단을 위한 주요정보를 제공할 센서아이디, 적용시간범위, 센서값의 범위, 비교연산자가 주된 정보이다.
1. gos_control_rule_action
 룰이 매칭되었을때 제어할 방법이 들어있는 테이블이다. 룰의 매치여부, 제어대상 구동기아이디, 제어인자, 작동시간으로 구성되어 있다.

### 적용조건
룰의 적용조건을 판단하기 위해서 적용시간범위와 센서값의 범위, 비교연산자가 활용된다. 비교연산자로 다음과 같은 6개의 문자열이 사용될 수 있다.
```
<, >, =, <=, >=, !=
```
적용시간범위는 하루를 초로 나타낸 단위를 사용한다. 오전 6시에서 7시를 적용시간 범위라고 한다면, ftime은 21600이고, ttime은 25200이다.
센서값의 범위는 시작시간의 값과 종료시간의 값으로 나타낸다. 즉 ftime의 값과 ttime의 값으로 나타낸다. 만약 10도에서 20도로 증가하는 경우라면 fvalue 는 10, tvalue는 20이 된다.

### 적용방법
위에서 설명한 테이블들을 활용하면 "온실 내부온도가 상한치를 초과할때 창을 열어라." 와 같은 명령을 쉽게 만들 수 있다. 

예를 들어 다음과 같은 설정이 가능하다. (여기서, 온실 내부온도 센서의 아이디는 11, 좌우측창 구동기의 아이디는 12, 13 이라고 가정한다. 장비아이디에 대한 정보는 gos_devicemap 테이블을 통해서 확인이 가능하다.)
1. 정오부터 오후2시 사이에 온실 내부 온도가 30도를 넘어가면 20초간 측창을 열어라. 이 룰은 매 1분마다 적용하라.
  ```
  insert into gos_control_rule (id, priority, period) values (1, 1, 60); // 60초마다 평가한다.
  insert into gos_control_condition (id, operator, sensor_id, ftime, ttime, fvalue, tvalue)
      values (1, '>', 11, 43200, 50400, 30, 30);
  insert into gos_control_action (id, istrue, actuator_id, argument, workingtime)
      values (1, 1, 12, 1, 20), (1, 1, 13, 1, 20);
  ```
1. 오전 7시부터 정오사이에 온실 내부 온도가 10도에서 20도로 오르지 못하면 측창을 10초간 닫아라. 이 룰은 매 30초마다 적용하라. 
  ```
  insert into gos_control_rule (id, priority, period) values (2, 1, 30); // 30초마다 평가한다.
  insert into gos_control_condition (id, operator, sensor_id, ftime, ttime, fvalue, tvalue)
      values (2, '<', 11, 25200, 43200, 10, 20);
  insert into gos_control_action (id, istrue, actuator_id, argument, workingtime)
      values (2, 1, 12, 2, 10), (1, 1, 13, 2, 10);
  ```

## 외부 프로그램 사용
자체 룰을 사용하는 경우 단순제어는 용이할 수 있지만, 복합환경제어를 하는 경우에는 한계가 있다. 이때 전용 프로그램을 작성하여 활용하는 것이 가능하다. 

외부 프로그램을 사용하는 경우에도 기본적인 동작방식은 비슷하다. 센서의 값을 확인하고 그 값을 평가하여 제어명령을 생성, 저장하면 된다.

### 관련 테이블

관련 테이블로 설명하면 외부 프로그램은 gos_environment_current 에서 값을 읽어서, 제어 명령을 생성하고, gos_control 에 해당 명령을 기록하면 된다.

1. gos_environment_current
 현재 센서들의 최종값을 가지고 있는 테이블이다. 입력시간, 센서아이디, 관측치가 주요 컬럼이다.

1. gos_control
 구동기에게 적용될 명령이 기록되는 테이블이다. 명령이 처리되면 입력된 레코드가 gos_control_history 테이블로 이동한다. 명령시간, 구동기아이디, 명령의 타입, 작동시간, 작동인자 등이 주요 컬럼이다.

### 명령 예시
1. 20초간 측창을 열어라. 
  ```
  insert into gos_control (device_id, ctrltype, worktime, argument)
      values (11, 'external', 20, 1);
  ```
1. 30초간 측창을 닫아라.
  ```
  insert into gos_control (device_id, ctrltype, worktime, argument)
      values (11, 'external', 30, 2);
  ```

### 외부 프로그램 적용 예

1. (주)그린시에스는 farmos 의 전신인 cflora를 기반으로 온실운영 알고리즘을 가진 외부프로그램을 작성하였고, 이를 국립원예과학원의 온실에 적용 테스트한 사례가 있다.
1. 한국과학기술원 강릉분원에서는 farmos를 기반으로 온실운영알고리즘인 welgro를 개발하여 테스트중에 있다. welgro는 총 3개의 버전으로 만들어졌으며, 최종 버전은 세종두레농업타운에 적용되었다.
