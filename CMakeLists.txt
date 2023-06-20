cmake_minimum_required(VERSION 3.24)
project(Server_Framwork)

include (cmake/utils.cmake)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(./third_part_apps/include)

link_directories(./third_part_apps/lib64)

set(LIB_SRC
    src/log.cc
    src/util.cc
    src/config.cc
    src/thread.cc
    src/macro.cc
    src/fiber.cc
    src/scheduler.cc)

add_library(src SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(src) #__FILE__

find_library(PTHREAD pthread)
find_library(YAMLCPP yaml-cpp)
set(LIB_LIB
        src
        pthread
        yaml-cpp
)

add_executable(test tests/test.cc)
add_dependencies(test src)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config src)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread src)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util src)
force_redefine_file_macro_for_sources(test_util) #__FILE__
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber src)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_link_libraries(test_fiber ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#add_executable(Server_Framwork main.cpp src/log.h src/log.cc)
