cmake_minimum_required(VERSION 3.5)
project(lcc)

include_directories(
        ${EXTENDS_DIR}
        ${EXTENDS_DIR}/libuv
        ${EXTENDS_DIR}/liblcc/inc
)

# 源文件查找
set(EXCLUDE_DIR "cmake")
file(GLOB_RECURSE SOURCES "*.cpp" "*.c")
foreach (TMP_PATH ${SOURCES})
    string(FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list(REMOVE_ITEM SOURCES ${TMP_PATH})
    endif ()
endforeach (TMP_PATH)

message("编译文件:" ${SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})