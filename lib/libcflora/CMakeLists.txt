# Copyright © 2017-2018 JiNong Inc. All Rights Reserved.
# 기존 파일을 살짝 수정함

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (libcflora)

SET(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Doxygen)
if(DOXYGEN_FOUND)
	add_custom_target(doc
			${DOXYGEN_EXECUTABLE} doc.src/doxy.conf
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Generating API documentation with Doxygen" VERBATIM
			)
endif(DOXYGEN_FOUND)


ADD_DEFINITIONS(-DLIBCF_CFG_BUILD_MODE="${CMAKE_BUILD_TYPE}")

ADD_DEFINITIONS(-DUSE_MYSQL -fsigned-char -Wall -DLIBCF_CFG_OS="${CMAKE_SYSTEM_NAME}" -DLIBCF_CFG_CPU="${CMAKE_SYSTEM_PROCESSOR}")

enable_testing ()
ADD_SUBDIRECTORY(src)
#ADD_SUBDIRECTORY(test)

