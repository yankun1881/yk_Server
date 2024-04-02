cmake_minimum_required(VERSION 2.8)
project(yk)



set(CMAKE_VERBOSE_MAKEFILE NO)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(.)

include_directories(/usr/include)

link_directories(/usr/lib)

find_package(yaml-cpp REQUIRED)


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
    )
    
add_executable(test tests/test.cc)
add_dependencies(test ${LIB_LIB})
target_link_libraries(test yk ${LIB_LIB})


add_executable(test_config tests/test_config.cc)
add_dependencies(test_config yk ${LIB_LIB})
target_link_libraries(test_config yk ${LIB_LIB})



add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread yk ${LIB_LIB}) 
target_link_libraries(test_thread yk ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util yk ${LIB_LIB}) 
target_link_libraries(test_util yk ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber yk ${LIB_LIB}) 
target_link_libraries(test_fiber yk ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler yk ${LIB_LIB}) 
target_link_libraries(test_scheduler yk ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager yk ${LIB_LIB}) 
target_link_libraries(test_iomanager yk ${LIB_LIB})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook yk ${LIB_LIB}) 
target_link_libraries(test_hook yk ${LIB_LIB})


add_executable(test_address tests/test_address.cc)
add_dependencies(test_address yk ${LIB_LIB}) 
target_link_libraries(test_address yk ${LIB_LIB})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket yk ${LIB_LIB}) 
target_link_libraries(test_socket yk ${LIB_LIB})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray yk ${LIB_LIB}) 
target_link_libraries(test_bytearray yk ${LIB_LIB})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http yk ${LIB_LIB}) 
target_link_libraries(test_http yk ${LIB_LIB})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server yk ${LIB_LIB}) 
target_link_libraries(test_tcp_server yk ${LIB_LIB})

add_executable(echo_server tests/echo_server.cc)
add_dependencies(echo_server yk ${LIB_LIB}) 
target_link_libraries(echo_server yk ${LIB_LIB})


add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server yk ${LIB_LIB}) 
target_link_libraries(test_http_server yk ${LIB_LIB})


add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection yk ${LIB_LIB}) 
target_link_libraries(test_http_connection yk ${LIB_LIB})


add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri yk ${LIB_LIB}) 
target_link_libraries(test_uri yk ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)




