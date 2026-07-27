cmake_minimum_require(VERSION 3.15)
project(chess-trainer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The engine logic lives in a library so both the game binary and the perft
# test can link against it without recompiling everything twice
add_library(chess_core
    src/types.cpp
    src/move.cpp
    src/board.cpp
    src/movegen.cpp
    src/pgn.cpp
    src/uci_engine.cpp
    src/review.cpp
    src/game.cpp
)

target_include_directories(chess_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(chess-trainer src/main.cpp)
target_link_libraries(chess-trainer PRIVATE chess-core)

add_executable(perft tests/perft.cpp)
target_link_libraries(perft PRIVATE chess_core)

enable_testing()
add_test(NAME perft_suite COMMAND perft 4)