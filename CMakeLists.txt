cmake_minimum_required(VERSION 3.5)
project(gameproj)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()
message("编译类型:" ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++11")

# clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# variable
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXTENDS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extends)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 公共链接库
link_directories(/usr/lib64/mysql/)

# 子模块
if (EXISTS "${EXTENDS_DIR}/liblcc/src")
    add_subdirectory(${EXTENDS_DIR}/liblcc)
endif ()


add_subdirectory(${TESTS_DIR}/TcpClient)
add_subdirectory(${TESTS_DIR}/TcpServer)