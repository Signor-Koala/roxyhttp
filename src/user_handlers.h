#ifndef OVERRIDES_H
#define OVERRIDES_H

#include "main.h"
#include <lua.h>

extern char *get_config(lua_State *L);
extern int init_handlers(lua_State *L);
size_t get_handler_response(lua_State *L, hheader req_hh, char **request,
                            size_t line_n, size_t body_size, char **response);

#endif // !OVERRIDES_H
