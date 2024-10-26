#include "handlers.h"
#include "main.h"
#include <lauxlib.h>
#include <lua.h>
#include <regex.h>
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

const char *confname = "config.lua";
const char *override_table_file = "overrides.lua";

static struct {
    size_t n;
    regex_t *patterns;
    char **override;
} override_table;

char *get_config(lua_State *L) {

    if (luaL_loadfile(L, confname) || lua_pcall(L, 0, 0, 0))
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

int init_overrides(lua_State *L) {

    if (luaL_loadfile(L, override_table_file)) {
        fprintf(stderr, "Cannot load override table file");
        return 1;
    }
    if (lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "Error running override table file");
        return 2;
    }
    lua_getglobal(L, "Overrides");
    if (!lua_istable(L, -1)) {
        fprintf(stderr, "'overrides' is not a table\n");
        return 3;
    }

    override_table.n = 0;

    lua_pushnil(L);
    size_t pathn, funcn;
    char **paths = NULL;
    char **funcs = NULL;
    while (lua_next(L, -2) != 0) {
        lua_pushvalue(L, -2);

        override_table.n++;
        char **p = realloc(paths, override_table.n * sizeof(char *));
        char **f = realloc(funcs, override_table.n * sizeof(char *));
        if (p != NULL && f != NULL) {
            paths = p;
            funcs = f;
        } else {
            fprintf(stderr, "Error allocating space for override table\n");
            exit(1);
        }

        const char *key = lua_tolstring(L, -1, &pathn);
        const char *value = lua_tolstring(L, -2, &funcn);

        paths[override_table.n - 1] = malloc((pathn + 1) * sizeof(char));
        funcs[override_table.n - 1] = malloc((funcn + 1) * sizeof(char));

        strlcpy(paths[override_table.n - 1], key, pathn + 1);
        strlcpy(funcs[override_table.n - 1], value, funcn + 1);
        lua_pop(L, 2);
    }

    override_table.patterns = malloc(override_table.n * sizeof(regex_t));
    override_table.override = funcs;
    int regex_status = 0;
    char buf[100];
    for (uint i = 0; i < override_table.n; i++) {
        if ((regex_status = regcomp(&override_table.patterns[i], paths[i],
                                    REG_EXTENDED))) {
            regerror(regex_status, &override_table.patterns[i], buf, 100);
            fprintf(stderr, "%s override regex compilation error: %s\n",
                    paths[i], buf);
            exit(1);
        }
    }
    free(paths);
    return 0;
}

size_t override_exec(lua_State *L, req request, char **response,
                     char *handler_name) {
    lua_getglobal(L, handler_name);

    lua_newtable(L);
    lua_pushstring(L, request.path);
    lua_setfield(L, -2, "path");

    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
        fprintf(stderr, "Error running function %s\n %s\n", handler_name,
                lua_tostring(L, -1));

    size_t len, header_len, res_len;
    const char *res = lua_tolstring(L, -1, &len);

    if (len > conf_max_response_size) {
        fprintf(stderr, "Override response above maximum response size\n");
        exit(1);
    }

    header_len = add_200_header(response);
    res_len = header_len + len;

    if (res_len > conf_buffer_size) {
        char *r = realloc(*response, res_len + 1);
        if (r == NULL) {
            fprintf(stderr, "Error allocating memory for response\n");
            exit(1);
        } else {
            *response = r;
        }
    }
    fprintf(stderr, "%s\n", *response);
    memcpy(&(*response)[header_len - 1], res, res_len);
    fprintf(stderr, "%s\n", *response);
    (*response)[res_len] = '\0';

    lua_pop(L, 1);

    return res_len;
}

size_t override_handler(lua_State *L, req request, char **response) {
    int status;
    for (uint i = 0; i < override_table.n; i++) {
        status = regexec(&override_table.patterns[i], request.path, 0, NULL, 0);
        if (!status) {
            return override_exec(L, request, response,
                                 override_table.override[i]);
        }
    }
    return 0;
}
