cmake_minimum_required(VERSION 3.16)

set ( PROJECT_NAME live5555 )
project(${PROJECT_NAME})

find_package(websocketpp REQUIRED)

set(CMAKE_CXX_STANDARD 11)

add_custom_command(OUTPUT fetch
	COMMAND git submodule add https://github.com/melchi45/live555.git
)

add_custom_target(new DEPENDS fetch)

add_subdirectory(live555)
set(LIVE555_HOME ${CMAKE_SOURCE_DIR}/live555)

if ( LIVE555_ENABLE_OPENSSL )
   set_property(DIRECTORY "${LIVE555_HOME}" APPEND
      PROPERTY COMPILE_DEFINITIONS "LIVE555_ENABLE_OPENSSL=0")
endif ()

include_directories(${LIVE555_HOME}/UsageEnvironment/include)
include_directories(${LIVE555_HOME}/BasicUsageEnvironment/include)
include_directories(${LIVE555_HOME}/liveMedia/include)
include_directories(${LIVE555_HOME}/groupsock/include)


add_definitions(-D_WEBSOCKETPP_CPP11_FUNCTIONAL_)
add_definitions(-D_WEBSOCKETPP_CPP11_THREAD_)
add_definitions(-D_WEBSOCKETPP_CPP11_SYSTEM_ERROR_)
add_definitions(-D_WEBSOCKETPP_CPP11_MEMORY_)
add_definitions(-D_REENTRANT)


# include_directories(/usr/include/spdlog)

#include_directories(/home/alex/CPP/lib/websocketpp/include)
#include_directories(/home/alex/CPP/lib/boost/1.65.1/include)


set(CMAKE_CXX_FLAGS "-w -pthread")

set(LIVE5555_SRC SinkBase.cpp RTSPClientBase.cpp)

message(STATUS "binary dir: ${CMAKE_BINARY_DIR}")

add_library(${PROJECT_NAME} ${LIVE5555_SRC})
include_directories(${PROJECT_NAME} CMAKE_CURRENT_SOURCE_DIR/include)
# target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_IMPORTS=1)
target_link_libraries(
        ${PROJECT_NAME}
        ${CMAKE_BINARY_DIR}/live555/liveMedia/libliveMedia.a
        ${CMAKE_BINARY_DIR}/live555/groupsock/libgroupsock.a
        ${CMAKE_BINARY_DIR}/live555/BasicUsageEnvironment/libBasicUsageEnvironment.a
        ${CMAKE_BINARY_DIR}/live555/UsageEnvironment/libUsageEnvironment.a
)

set(WS_PUSH_SRC wspush.cpp WebSocketClient.cpp WebSocketClient.cpp)
add_executable(wspush ${WS_PUSH_SRC})

# if ( LIVE555_ENABLE_OPENSSL )
#    set(LIVE5555_LINK wspush live5555 OpenSSL::Crypto OpenSSL::SSL)
# else ()
#    set(LIVE5555_LINU wspush live5555)
# endif ()

target_link_libraries(
        wspush
        live5555
      #   ${CMAKE_BINARY_DIR}/live555/liveMedia/libliveMedia.a
      #   ${CMAKE_BINARY_DIR}/live555/groupsock/libgroupsock.a
      #   ${CMAKE_BINARY_DIR}/live555/BasicUsageEnvironment/libBasicUsageEnvironment.a
      #   ${CMAKE_BINARY_DIR}/live555/UsageEngironment/libUsageEnvironment.a
)