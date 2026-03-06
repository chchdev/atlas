cmake_minimum_required(VERSION 3.16)
project(atlas C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(
    atlas
    src/main.c
    src/util.c
    src/lexer.c
    src/ast.c
    src/parser.c
    src/runtime.c
)

target_include_directories(atlas PRIVATE include)

if(MSVC)
    target_compile_options(atlas PRIVATE /W4)
else()
    target_compile_options(atlas PRIVATE -Wall -Wextra -Wpedantic)
endif()
