cmake_minimum_required(VERSION 3.16)
project(compiler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(compiler
    src/main.cpp
    src/lexer/token.cpp
    src/lexer/lexer.cpp
    src/parser/ast_printer.cpp
    src/parser/parser.cpp
)

target_include_directories(compiler PRIVATE src)

if(MSVC)
    target_compile_options(compiler PRIVATE /W4)
else()
    target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()
