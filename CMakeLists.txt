cmake_minimum_required(VERSION 3.30)

project(roxyhttp)
set(TARGET roxyhttp)

add_executable(
  roxyhttp
  src/main.c
  src/main.h
  src/user_handlers.c
  src/user_handlers.h
  src/default_handlers.c
  src/default_handlers.h
  src/translate.c
  src/translate.h)

target_link_libraries(${TARGET} PRIVATE m)
target_link_libraries(${TARGET} PRIVATE -lbsd -lrt -llua)

target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)
