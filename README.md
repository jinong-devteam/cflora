# farmos : Farm Operating System

## Introduction

FarmOS는 [cflora](https://github.com/ezfarm-farmcloud/cflora)라는 (주)이지팜과 농촌진흥청이 함께 개발한 온실환경관리 플랫폼의 포크 프로젝트입니다. 기존의 프로젝트에서 사용하던 일부 라이브러리를 사용하지 않으며, 기존 하드웨어 이외에 아두이노 지원이 추가되어 있습니다. 그외 앞으로 많은 변경사항도 예정되어 있습니다.

## Maintainers

* 김준용 (joonyong.jinong@gmail.com)

## 특징
* TTAK.KO-06.0288 을 지원합니다.
 * Part 1, Part 2 는 주식회사 지농에서 공개한 libgnode 를 사용해 지원합니다. 단, (주)이지팜에서 공개한 버전의 libtp12와는 호환이 되지 않습니다.
 * Part 3는 (주)이지팜에서 공개한 libtp3를 사용해 지원합니다.

* node를 포함하지 않습니다.
 * farmos는 특정한 노드와의 호환을 목적하지 않기 때문에 노드에 관한 소스는 없습니다. libgnode를 사용하는 노드와 호환이 됩니다.

## How to build

### Dependency
FarmOS 는 다음의 오픈소스를 사용하고 있습니다.
* [iniparser](https://github.com/ndevilla/iniparser)
* [libtp3](https://github.com/ezfarm-farmcloud/libtp3)
* [libgnode](https://github.com/jinong-devteam/libgnode)
* [glog](https://github.com/google/glog)

iniparser 와 libtp3는 git submodule 로 연동되어 있습니다. 따라서 다음과 같이 입력하면 컴파일을 위한 소스들을 받아올 수 있습니다.
```
git submodule init
git submodule update
```

단, libtp3는 [libuv](https://github.com/libuv/libuv) 를 사용하기 때문에 libuv 를 먼저 설치해주어야 합니다. 더 상세한 설명은 libuv 홈페이지를 참조하시길 바랍니다.
```
wget https://github.com/libuv/libuv/archive/v1.x.zip
unzip v1.x.zip -d v1.x
cd v1.x/libuv-1.x
./autogen.sh
./configure
make
make install
```

glog는 다음과 같이 설치 가능합니다.
```
sudo apt install  libgoogle-glog-dev
```

* [mysql](https://www.mysql.com)
mysql 은 사용하는 OS에 따라 적절히 설치하면 됩니다. debian 계열의 Ubuntu나 Rasibian 등을 사용하는 경우에는 다음과 같이 설치할 수 있습니다.
```
sudo apt install mysql-server libmysqlclient20 libmysqlclient-dev
```

### Getting Source 
 원하는 디렉토리로 이동후 다음의 명령을 입력합니다.
 ```
 git clone http://github.com/jinong-devteam/farmos.git
 cd farmos
 git submodule init
 git submodule update
 ```

### Building
 scripts 폴더로 이동하여 build.sh 를 실행합니다.
 ```
 cd scripts
 ./build.sh
 ```
 
