cmake_minimum_required(VERSION 3.14)

project(braillart)

set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra -Wshadow -Werror -g -mavx")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wshadow -Werror -g -mavx")

include_directories(include)

add_library(pnmlib SHARED src/pnmlib.c)
add_library(safelib SHARED src/safelib.c)
add_library(debuglib SHARED src/debug.c)

add_executable(brailleart src/brailleart.c)
target_link_libraries(brailleart pnmlib safelib debuglib)

add_executable(dithering src/dithering.c)
target_link_libraries(dithering pnmlib safelib debuglib)
