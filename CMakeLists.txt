cmake_minimum_required(VERSION 3.5)

project(BoolX LANGUAGES C)

add_executable(boolx src/interpreter.c)
target_link_libraries(boolx m)

add_executable(compactorx src/compactor.c)

set(EXECUTABLE_OUTPUT_PATH ../bin/)
