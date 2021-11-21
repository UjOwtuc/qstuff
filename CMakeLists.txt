cmake_minimum_required(VERSION 3.0)
project(qstuff)

find_package(Qt5 REQUIRED COMPONENTS Gui Widgets Network Charts)
find_package(Qt5 COMPONENTS Test)

if(EXISTS ${Qt5Test_DIR})
	enable_testing()
endif()

set(LOGSTUFF_PATH "../logstuff" CACHE STRING "Path to logstuff's build directory")
find_library(
	logstuff_query
	NAMES logstuff_query liblogstuff_query
	PATHS ${LOGSTUFF_PATH}/target ${LOGSTUFF_PATH}/query/target
	PATH_SUFFIXES release debug
	DOC "liblogstuff_query library path"
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(src)
