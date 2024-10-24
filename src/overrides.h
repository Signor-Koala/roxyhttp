#ifndef OVERRIDES_H
#define OVERRIDES_H

#include "main.h"
#include <lua.h>

extern char *get_config(lua_State *L);
extern int init_overrides(lua_State *L);
size_t override_handler(lua_State *L, req request, char **response);

#endif // !OVERRIDES_H
