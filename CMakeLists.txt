cmake_minimum_required(VERSION 2.8)
project (mirb)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++0x -Wall")
add_definitions( -DNO_GC -DMIRB_THREADED )
include_directories("../Prelude/include")

file(GLOB MIRB_SOURCES
    "src/*.cpp"
    "src/classes/*.cpp"
    "src/codegen/*.cpp"
    "src/generic/*.cpp"
    "src/lexer/*.cpp"
    "src/modules/*.cpp"
    "src/parser/*.cpp"
    "src/tree/*.cpp"
)

add_executable(mirb ${MIRB_SOURCES})

