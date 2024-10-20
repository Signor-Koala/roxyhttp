#include "main.h"
#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *keys[] = {
    // Numbers
    "Buffer_size",
    "Max_filepath",
    "Max_response_size",
    "Max_cache_entry_number",
    "Max_cache_entry_size",
    // Port for some reason needs to be a string
    "Port",
    // Strings
    "Filepath",
};

const char *fname = "config.lua";

char *get_config(lua_State *L) {

    if (luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0))
        fprintf(stderr, "Cannot run config file for some reason\n");

    size_t *number_vars[] = {
        &conf_buffer_size,       &conf_max_filepath,   &conf_max_response_size,
        &conf_max_cache_entries, &conf_max_entry_size,
    };
    int isnum;
    size_t res_num;
    for (int i = 0; i < 5; i++) {
        lua_getglobal(L, keys[i]);
        res_num = lua_tointegerx(L, -1, &isnum);
        if (!isnum)
            fprintf(stderr, "%s invalid, defaulting to %zu\n", keys[i],
                    *(number_vars[i]));
        else
            *(number_vars[i]) = res_num;
        lua_pop(L, 1);
    }

    lua_getglobal(L, keys[5]);
    res_num = lua_tointegerx(L, -1, &isnum);
    if (!isnum)
        fprintf(stderr, "%s invalid, defaulting to %s\n", keys[5], conf_port);
    else if (res_num > 65535 && res_num < 1)
        fprintf(stderr, "%s should be between 1 - 65535, defaulting to %s\n",
                keys[5], conf_port);
    else
        sprintf(conf_port, "%zu", res_num);
    lua_pop(L, 1);

    lua_getglobal(L, keys[6]);
    size_t len;
    const char *res_str = lua_tolstring(L, -1, &len);
    char *ptr;
    if (res_str == NULL || !len) {
        fprintf(stderr, "%s invalid, defaulting to %s\n", keys[6],
                conf_file_path);
    } else {
        ptr = malloc(len + 1);
        if (ptr == NULL) {
            fprintf(stderr, "%s invalid, defaulting to %s\n", keys[6],
                    conf_file_path);
        } else {
            strlcpy(ptr, res_str, len + 1);
        }
    }
    return ptr;
}
