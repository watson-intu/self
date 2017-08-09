cmake_minimum_required(VERSION 2.8.12.2)
project(self)
find_package(qibuild)
include_directories(src lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++0x -Wno-deprecated-declarations")

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
add_subdirectory(tools)

qi_create_bin(self_instance src/main.cpp)
qi_use_lib(self_instance self utils)


