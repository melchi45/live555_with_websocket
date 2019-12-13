cmake_minimum_required(VERSION 3.6)

set ( PROJECT_NAME live5555 )
project(${PROJECT_NAME})

find_package(websocketpp REQUIRED)

set(CMAKE_CXX_STANDARD 11)

add_custom_command(OUTPUT fetch
	COMMAND git submodule add https://github.com/melchi45/live555.git
)

add_custom_target(new DEPENDS fetch)

add_subdirectory(live555)
set(LIVE555_HOME /mnt/d/live5555/live555)

include_directories(${LIVE555_HOME}/UsageEnvironment/include)
include_directories(${LIVE555_HOME}/BasicUsageEnvironment/include)
include_directories(${LIVE555_HOME}/liveMedia/include)
include_directories(${LIVE555_HOME}/groupsock/include)


add_definitions(-D_WEBSOCKETPP_CPP11_FUNCTIONAL_)
add_definitions(-D_WEBSOCKETPP_CPP11_THREAD_)
add_definitions(-D_WEBSOCKETPP_CPP11_SYSTEM_ERROR_)
add_definitions(-D_WEBSOCKETPP_CPP11_MEMORY_)
add_definitions(-D_REENTRANT)


include_directories(/usr/include/spdlog)

#include_directories(/home/alex/CPP/lib/websocketpp/include)
#include_directories(/home/alex/CPP/lib/boost/1.65.1/include)


set(CMAKE_CXX_FLAGS "-w -pthread")

set(LIVE5555_SRC SinkBase.cpp RTSPClientBase.cpp)

add_library(${PROJECT_NAME} ${LIVE5555_SRC})
include_directories(${PROJECT_NAME} CMAKE_CURRENT_SOURCE_DIR/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_IMPORTS=1)
target_link_libraries(
        ${PROJECT_NAME}
        ${LIVE555_HOME}/lib/libliveMedia.a
        ${LIVE555_HOME}/lib/libgroupsock.a
        ${LIVE555_HOME}/lib/libBasicUsageEnvironment.a
        ${LIVE555_HOME}/lib/libUsageEnvironment.a
)

set(WS_PUSH_SRC wspush.cpp WebSocketClient.cpp WebSocketClient.cpp)
add_executable(wspush ${WS_PUSH_SRC})
target_link_libraries(
        wspush
        live5555
        /home/alex/CPP/lib/boost/1.65.1/lib/libboost_system.so
)
