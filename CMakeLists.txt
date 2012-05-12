cmake_minimum_required(VERSION 2.8)
project (mirb)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++0x -Wall -Wextra -Wunused")
add_definitions(-DMIRB_THREADED -DMIRB_GC_SKIP_BLOCKS -DDEBUG)
include_directories("${mirb_SOURCE_DIR}/../Prelude/include", "${mirb_SOURCE_DIR}/../Libraries/pcre")
link_directories("${mirb_SOURCE_DIR}/../Libraries/pcre")

file(GLOB MIRB_SOURCES
    "src/*.cpp"
    "src/classes/*.cpp"
    "src/platform/*.cpp"
    "src/codegen/*.cpp"
    "src/generic/*.cpp"
    "src/lexer/*.cpp"
    "src/modules/*.cpp"
    "src/parser/*.cpp"
    "src/tree/*.cpp"
)

add_executable(mirb ${MIRB_SOURCES})
target_link_libraries(mirb pcre)
