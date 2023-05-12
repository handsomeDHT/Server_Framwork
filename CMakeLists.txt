cmake_minimum_required(VERSION 3.24)
project(Server_Framwork)

## include (cmake/utils.cmake)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(./third_part_apps/include)

link_directories(./third_part_apps/lib64)

set(LIB_SRC
    src/log.cc
    src/util.cc
    src/config.cc
        )

add_library(src SHARED ${LIB_SRC})
add_executable(test tests/test.cc)
add_dependencies(test src)
target_link_libraries(test src yaml-cpp)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config src)
target_link_libraries(test_config src yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#add_executable(Server_Framwork main.cpp src/log.h src/log.cc)
