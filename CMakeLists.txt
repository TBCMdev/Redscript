cmake_minimum_required(VERSION 3.10)

project(redscript)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER_ID 1)


if (MSVC)
    add_compile_options(/W4)  # Use /W4 instead of -Wextra
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

add_compile_options(-g3 -pipe)

file(GLOB TYPES_SOURCES "${CMAKE_SOURCE_DIR}/src/types/*.cpp")

add_library(redscript_types ${TYPES_SOURCES})




add_library(redscript_lib
	src/config.cpp
	src/error.cpp
    src/file.cpp
	src/inb.cpp
	src/lexer.cpp
	src/mc.cpp
	src/rbc.cpp
	src/util.cpp
	src/parser.cpp
	src/bst.cpp
)

add_executable(rscript entry.cpp)
target_link_libraries(rscript PRIVATE redscript_lib)
target_link_libraries(rscript PRIVATE redscript_types)
target_include_directories(rscript PUBLIC ${CMAKE_SOURCE_DIR}/types)
target_include_directories(rscript PUBLIC src)
