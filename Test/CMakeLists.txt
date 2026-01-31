cmake_minimum_required(VERSION 3.10)
project(CTPTraderTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# CTP库目录
set(CTP_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Framework/Linux")

# CTP头文件目录
include_directories(${CTP_LIB_DIR})

# 链接目录
link_directories(${CTP_LIB_DIR})

# 可执行文件
add_executable(ctp_trader_test main.cpp)

# 添加CTP库为IMPORTED目标
add_library(thosttraderapi_se SHARED IMPORTED)
set_target_properties(thosttraderapi_se PROPERTIES
    IMPORTED_LOCATION "${CTP_LIB_DIR}/thosttraderapi_se.so"
)

# 链接CTP交易库
target_link_libraries(ctp_trader_test
    thosttraderapi_se
    dl
    pthread
)

# 设置RPATH，使程序能找到.so文件
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_RPATH ${CTP_LIB_DIR})

# 输出信息
message(STATUS "CTP Library Directory: ${CTP_LIB_DIR}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")

# 安装规则
install(TARGETS ctp_trader_test DESTINATION bin)
