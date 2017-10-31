cmake_minimum_required(VERSION 2.8.12.2)
project(self)
find_package(qibuild)
include_directories(src lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wno-deprecated-declarations")
if("$ENV{TARGET}" STREQUAL "linux" OR "$ENV{TARGET}" STREQUAL "sota")
	add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()
if("$ENV{TARGET}" STREQUAL "raspi")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
endif()


add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
add_subdirectory(tools)

qi_create_bin(self_instance src/main.cpp)
qi_use_lib(self_instance self utils)


