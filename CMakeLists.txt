cmake_minimum_required(VERSION 3.10)
project(roxyhttp)
add_executable(
  roxyhttp
  src/main.c
  src/main.h
  src/handlers.c
  src/handlers.h
  src/translate.c
  src/translate.h
  src/cache.c
  src/cache.h
  src/overrides.c
  src/overrides.h)
target_compile_options(roxyhttp PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(roxyhttp -llua)
