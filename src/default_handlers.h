#ifndef HANDLER_H
#define HANDLER_H

#include "cache.h"
#include "main.h"
#include <lua.h>
#include <stddef.h>

size_t handle_get(hheader req, char **response, lru_table *cache);
size_t add_200_header_html(char **response);
size_t handle_error(lua_State *L, enum ERROR_STATUS status, char **response);

#endif // !HANDLER_H
