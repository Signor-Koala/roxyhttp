#ifndef OVERRIDES_H
#define OVERRIDES_H

#include "main.h"
#include <lua.h>

extern char *get_config(lua_State *L);
extern int init_handlers(lua_State *L);
size_t get_handler_response(lua_State *L, req request, char **response);

#endif // !OVERRIDES_H
