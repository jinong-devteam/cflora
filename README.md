# FarmOS : Farm Oprating System

## Introduction

FarmOS는 [cflora](https://github.com/ezfarm-farmcloud/cflora)라는 (주)이지팜과 농촌진흥청이 함께 개발한 온실환경관리 플랫폼의 포크 프로젝트입니다. 기존의 프로젝트에서 사용하던 일부 라이브러리를 사용하지 않으며, 기존 하드웨어 이외에 아두이노 지원이 추가되어 있습니다. 그외 앞으로 많은 변경사항도 예정되어 있습니다. 

## Feature highlights

### Dependency
cflora 는 다음의 오픈소스를 사용하고 있습니다.
* [iniparser](https://github.com/ndevilla/iniparser)
* [libtp3](https://github.com/ezfarm-farmcloud/libtp3)

iniparser 와 libtp3는 git submodule 로 연동되어 있습니다. 따라서 다음과 같이 입력하면 컴파일을 위한 소스들을 받아올 수 있습니다.
```
git submodule init
git submodule update
```

단 libtp3는 [libuv](https://github.com/libuv/libuv) 를 사용하기 때문에 libuv 를 먼저 설치해주어야 합니다. 더 상세한 설명은 libuv 홈페이지를 참조하시길 바랍니다.
```
wget https://github.com/libuv/libuv/archive/v1.x.zip
unzip v1.x.zip -d v1.x
cd v1.x/libuv-1.x
./autogen.sh
./configure
make
make install
```

* [mysql](https://www.mysql.com)
mysql 은 사용하는 OS에 따라 적절히 설치하면 됩니다. debian 계열의 Ubuntu나 Rasibian 등을 사용하는 경우에는 다음과 같이 설치할 수 있습니다.
```
sudo apt install mysql-server
```

### Build Instructions
cmake 를 이용해서 컴파일이 가능합니다. 개별 폴더로 이동하셔서 다음의 명령을 입력하시면 컴파일이 됩니다.
```
mkdir release
cd release
cmake ..
make
```

