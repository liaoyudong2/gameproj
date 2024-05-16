cmake_minimum_required(VERSION 3.5)
project(TestWebSocketServer)

message("编译测试单元:" ${PROJECT_NAME})

# 包含目录
include_directories(
        ${EXTENDS_DIR}
        ${EXTENDS_DIR}/libuv
        ${EXTENDS_DIR}/liblcc/inc
        ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(${LIBRARY_OUTPUT_PATH})

# 源文件查找
file (GLOB_RECURSE SOURCES "*.cpp")

# 二进制文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} lcc)
target_link_libraries(${PROJECT_NAME} uv)
target_link_libraries(${PROJECT_NAME} mbedtls)
target_link_libraries(${PROJECT_NAME} mbedcrypto)
target_link_libraries(${PROJECT_NAME} mbedx509)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} dl)
target_link_libraries(${PROJECT_NAME} rt)
