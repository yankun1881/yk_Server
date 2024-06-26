cmake_minimum_required(VERSION 2.8)
project(yk)



set(CMAKE_VERBOSE_MAKEFILE NO)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-deprecated-declarations")

include_directories(.)

include_directories(/usr/include)

link_directories(/usr/lib)

find_package(yaml-cpp REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)

set(
    LIB_SRC
    yk/log.cc
    yk/util.cc
    yk/config.cc
    yk/thread.cc
    yk/fiber.cc
    yk/scheduler.cc
    yk/iomanager.cc
    yk/timer.cc
    yk/hook.cc
    yk/fdmanager.cc
    yk/address.cc
    yk/socket.cc
    yk/bytearray.cc
    http/http.cc  
    http/http_parser.cc  
    http/http_server.cc  
    http/http11_parser.cc
    http/httpclient_parser.cc
    yk/tcp_server.cc 
    yk/stream.cc     
    yk/socket_stream.cc   
    http/http_session.cc
    http/http_servlet.cc
    http/http_connection.cc
    yk/uri.cc   
    yk/daemon.cc
    yk/env.cc
    yk/application.cc    
    yk/module.cc
    yk/library.cc
    yk/worker.cc
    http/ws_session.cc
    http/ws_servlet.cc
    http/ws_server.cc
    http/ws_connection.cc
    
    chat/chat_servlet.cc
    chat/myModule.cc
    chat/protocol.cc
    chat/resource_servlet.cc
)
add_library(yk SHARED ${LIB_SRC})
#add_library(yk_static  STATIC$ {LIB_SRC})
#SET_TARGET_PROPERTIES(yk_static  PROPERTIES OUTPUT_NAME "yk")

#set(LIB_LIB ${LIB_LIB} dl)


set(LIB_LIB
    yk
    pthread
    dl
    yaml-cpp
    OpenSSL::Crypto
    jsoncpp
    )
    
function(add_test_executable NAME)
    add_executable(${NAME} tests/${NAME}.cc)
    add_dependencies(${NAME} yk ${LIB_LIB})
    target_link_libraries(${NAME} yk ${LIB_LIB})
endfunction()

add_test_executable(test)
add_test_executable(test_config)
add_test_executable(test_thread)
add_test_executable(test_fiber)
add_test_executable(test_scheduler)
add_test_executable(test_iomanager)
add_test_executable(test_hook)
add_test_executable(test_address)
add_test_executable(test_socket)
add_test_executable(test_bytearray)
add_test_executable(test_http)
add_test_executable(test_tcp_server)
add_test_executable(echo_server)
add_test_executable(test_http_server)
add_test_executable(test_http_connection)
add_test_executable(test_uri)
add_test_executable(http_server)
add_test_executable(test_daemon)
add_test_executable(test_env)
add_test_executable(test_application)
add_test_executable(test_ws_client)
add_test_executable(test_ws_server)
add_test_executable(chat_room)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)




