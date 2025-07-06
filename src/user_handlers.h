#ifndef OVERRIDES_H
#define OVERRIDES_H

#include "main.h"
#include <lua.h>

extern char *get_config(lua_State *L);
extern int init_handlers(lua_State *L);
size_t get_handler_response(lua_State *L, hheader req_hh, char **response);
size_t exec_middleware(lua_State *L, hheader req_hh);

#endif // !OVERRIDES_H
