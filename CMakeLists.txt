cmake_minimum_required(VERSION 3.30)

project(roxyhttp)
set(TARGET roxyhttp)

set(LUA_VERSION "lua-5.4.7")

include(ExternalProject) 

ExternalProject_Add(lua
  PREFIX ${CMAKE_BINARY_DIR}/${LUA_VERSION}
  URL https://www.lua.org/ftp/${LUA_VERSION}.tar.gz 
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make generic
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE true
  DOWNLOAD_EXTRACT_TIMESTAMP false
)
ExternalProject_Get_property(lua BINARY_DIR)
set(LUA_INCLUDE_DIR ${BINARY_DIR}/src)
set(LUA_LIBRARIES ${BINARY_DIR}/src/liblua.a)

add_executable(
  roxyhttp
  src/main.c
  src/main.h
  src/user_handlers.c
  src/user_handlers.h
  src/default_handlers.c
  src/default_handlers.h
  src/translate.c
  src/translate.h
  src/cache.c
  src/cache.h)

add_dependencies(${TARGET} lua)
target_include_directories(${TARGET} PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(${TARGET} PRIVATE ${LUA_LIBRARIES})
target_link_libraries(${TARGET} PRIVATE m)

target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)
