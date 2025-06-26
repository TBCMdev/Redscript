cmake_minimum_required(VERSION 3.10)

project(redscript)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

add_library(redscript_lib
	src/config.cpp
	src/error.cpp
    src/file.cpp
	src/inb.cpp
	src/lang.cpp
	src/lexer.cpp
	src/mc.cpp
	src/rbc.cpp
	src/util.cpp
)

add_executable(redscript_cli entry.cpp)
target_link_libraries(redscript_cli PRIVATE redscript_lib)
target_include_directories(redscript_cli PUBLIC src)
