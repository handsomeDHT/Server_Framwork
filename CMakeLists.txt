cmake_minimum_required(VERSION 3.24)
project(Server_Framwork)

include (cmake/utils.cmake)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-builtin-macro-redefined")


include_directories(.)
include_directories(./third_part_apps/include)
link_directories(./third_part_apps/lib64)

option(BUILD_TEST "ON for complile test" OFF)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
    src/log.cc
    src/util.cc
    src/config.cc
    src/thread.cc
    src/macro.cc
    src/fiber.cc
    src/scheduler.cc
    src/iomanager.cc
    src/timer.cc
    src/hook.cc
    src/fd_manager.cc
    src/address.cc
    src/endian.h
    src/socket.cc
    src/noncopyable.h
    src/mutex.cc
    src/bytearray.cc
    src/tcp_server.cc
    src/stream.cc
    src/uri.rl
    src/streams/socket_stream.cc
    src/http/http.cc
     src/http/http11_parser.rl.cc
     src/http/httpclient_parser.rl.cc
     src/http/http_parser.cc
     src/http/http_session.cc
     src/http/servlet.cc
     src/http/http_server.cc
     src/http/http_connection.cc
        src/util/crypto_util.cc
        src/util/hash_util.cc
        src/util/json_util.cc
        src/streams/socket_stream.cc
        src/streams/zlib_stream.cc
     src/daemon.cc
     src/env.cc
        )

ragelmaker(src/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/http)
ragelmaker(src/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(src SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(src) #__FILE__

find_library(PTHREAD pthread)
find_library(YAMLCPP yaml-cpp)
set(LIBS
        src
        dl
        pthread
        yaml-cpp
        jsoncpp
        ${OPENSSL_LIBRARIES}
)

dht_add_executable(test "tests/test.cc" src "${LIBS}")
dht_add_executable(test_config "tests/test_config.cc" src "${LIBS}")
dht_add_executable(test_thread "tests/test_thread.cc" src "${LIBS}")
dht_add_executable(test_util "tests/test_util.cc" src "${LIBS}")
dht_add_executable(test_fiber "tests/test_fiber.cc" src "${LIBS}")
dht_add_executable(test_scheduler "tests/test_scheduler.cc" src "${LIBS}")
dht_add_executable(test_iomanager "tests/test_iomanager.cc" src "${LIBS}")
dht_add_executable(test_hook "tests/test_hook.cc" src "${LIBS}")
dht_add_executable(test_address "tests/test_address.cc" src "${LIBS}")
dht_add_executable(test_socket "tests/test_socket.cc" src "${LIBS}")
dht_add_executable(test_bytearray "tests/test_bytearray.cc" src "${LIBS}")
dht_add_executable(test_http "tests/test_http.cc" src "${LIBS}")
dht_add_executable(test_http_parser "tests/test_http_parser.cc" src "${LIBS}")
dht_add_executable(test_tcp_server "tests/test_tcp_server.cc" src "${LIBS}")
dht_add_executable(echo_server "examples/echo_server.cc" src "${LIBS}")
dht_add_executable(test_http_server "tests/test_http_server.cc" src "${LIBS}")
dht_add_executable(test_http_connection "tests/test_http_connection.cc" src "${LIBS}")
dht_add_executable(test_uri "tests/test_uri.cc" src "${LIBS}")
dht_add_executable(my_http_server "samples/my_http_server.cc" src "${LIBS}")
dht_add_executable(echo_server_udp "examples/echo_server_udp.cc" src "${LIBS}")
dht_add_executable(echo_udp_client "examples/echo_udp_client.cc" src "${LIBS}")
dht_add_executable(test_daemon "tests/test_daemon.cc" src "${LIBS}")
dht_add_executable(test_env "tests/test_env.cc" src "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#add_executable(Server_Framwork main.cpp src/log.h src/log.cc)
