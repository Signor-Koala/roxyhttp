#include "default_handlers.h"
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
const char *handler_table_file = "handlers.lua";
const char *middleware_table_file = "middleware.lua";

static struct {
    size_t n;
    regex_t *patterns;
    char **handler;
} handler_table;

static struct {
    size_t n;
    regex_t *patterns;
    char **middleware;
} middleware_table;

// NOTE: For Debugging
static void stackDump(lua_State *L) {
    int i;
    int top = lua_gettop(L); /* depth of the stack */
    fprintf(stderr, "----------------------------------------\n");
    for (i = 1; i <= top; i++) { /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {
        case LUA_TSTRING: { /* strings */
            printf("'%s'", lua_tostring(L, i));
            break;
        }
        case LUA_TBOOLEAN: { /* Booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
        }
        case LUA_TNUMBER: { /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
        }
        default: { /* other values */
            printf("%s", lua_typename(L, t));
            break;
        }
        }
        printf("\n"); /* put a separator */
    }
    printf("\n"); /* end the listing */
    fprintf(stderr, "----------------------------------------\n");
}

// TODO: Make this nicer to maintain
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
    lua_settop(L, 0);
    return ptr;
}

int init_handlers(lua_State *L) {

    if (luaL_loadfile(L, handler_table_file)) {
        fprintf(stderr, "Cannot load handler table file\n");
        lua_settop(L, 0);
        return 1;
    }
    if (lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "Error running handler table file\n");
        lua_settop(L, 0);
        return 2;
    }
    lua_getglobal(L, "Handlers");
    if (!lua_istable(L, -1)) {
        fprintf(stderr, "'Handlers' is not a table\n");
        lua_settop(L, 0);
        return 3;
    }

    handler_table.n = 0;

    lua_pushnil(L);
    size_t pathn, funcn;
    char **paths = NULL;
    char **funcs = NULL;
    while (lua_next(L, -2) != 0) {
        lua_pushvalue(L, -2);

        handler_table.n++;
        char **p = realloc(paths, handler_table.n * sizeof(char *));
        char **f = realloc(funcs, handler_table.n * sizeof(char *));
        if (p != NULL && f != NULL) {
            paths = p;
            funcs = f;
        } else {
            fprintf(stderr, "Error allocating space for handler table\n");
            lua_settop(L, 0);
            exit(1);
        }

        const char *key = lua_tolstring(L, -1, &pathn);
        const char *value = lua_tolstring(L, -2, &funcn);

        paths[handler_table.n - 1] = malloc((pathn + 1) * sizeof(char));
        funcs[handler_table.n - 1] = malloc((funcn + 1) * sizeof(char));

        strlcpy(paths[handler_table.n - 1], key, pathn + 1);
        strlcpy(funcs[handler_table.n - 1], value, funcn + 1);
        lua_pop(L, 2);
    }
    lua_settop(L, 0);

    handler_table.patterns = malloc(handler_table.n * sizeof(regex_t));
    handler_table.handler = funcs;
    int regex_status = 0;
    char buf[100];
    for (uint i = 0; i < handler_table.n; i++) {
        if ((regex_status =
                 regcomp(&handler_table.patterns[i], paths[i], REG_EXTENDED))) {
            regerror(regex_status, &handler_table.patterns[i], buf, 100);
            fprintf(stderr, "%s override regex compilation error: %s\n",
                    paths[i], buf);
            exit(1);
        }
    }
    free(paths);

    // Middleware
    if (luaL_loadfile(L, middleware_table_file)) {
        fprintf(stderr, "Cannot load middleware table file\n");
        lua_settop(L, 0);
        return 1;
    }
    if (lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "Error running middleware table file\n");
        lua_settop(L, 0);
        return 2;
    }
    lua_getglobal(L, "Middleware");
    if (!lua_istable(L, -1)) {
        fprintf(stderr, "'Middleware' is not a table\n");
        lua_settop(L, 0);
        return 3;
    }

    middleware_table.n = 0;

    lua_pushnil(L);
    paths = NULL;
    funcs = NULL;
    while (lua_next(L, -2) != 0) {
        lua_pushvalue(L, -2);

        middleware_table.n++;
        char **p = realloc(paths, middleware_table.n * sizeof(char *));
        char **f = realloc(funcs, middleware_table.n * sizeof(char *));
        if (p != NULL && f != NULL) {
            paths = p;
            funcs = f;
        } else {
            fprintf(stderr, "Error allocating space for override table\n");
            exit(1);
        }

        const char *key = lua_tolstring(L, -1, &pathn);
        const char *value = lua_tolstring(L, -2, &funcn);

        paths[middleware_table.n - 1] = malloc((pathn + 1) * sizeof(char));
        funcs[middleware_table.n - 1] = malloc((funcn + 1) * sizeof(char));

        strlcpy(paths[middleware_table.n - 1], key, pathn + 1);
        strlcpy(funcs[middleware_table.n - 1], value, funcn + 1);
        lua_pop(L, 2);
    }

    middleware_table.patterns = malloc(middleware_table.n * sizeof(regex_t));
    middleware_table.middleware = funcs;
    regex_status = 0;
    for (uint i = 0; i < middleware_table.n; i++) {
        if ((regex_status = regcomp(&middleware_table.patterns[i], paths[i],
                                    REG_EXTENDED))) {
            regerror(regex_status, &middleware_table.patterns[i], buf, 100);
            fprintf(stderr, "%s override regex compilation error: %s\n",
                    paths[i], buf);
            exit(1);
        }
    }
    free(paths);

    lua_settop(L, 0);
    return 0;
}

size_t build_response(lua_State *L, char **response) {
    size_t res_len;
    if (lua_istable(L, -1)) {
        // TODO: Turn table into response
    } else if (lua_isstring(L, -1)) {
        size_t len, header_len;
        const char *res = lua_tolstring(L, -1, &len);

        if (len > conf_max_response_size) {
            fprintf(stderr, "Override response above maximum response size\n");
            exit(1);
        }

        header_len = add_200_header_html(response);
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
        memcpy(&(*response)[header_len - 1], res, res_len);
        (*response)[res_len] = '\0';
    } else {
        fprintf(stderr, "Unsupported output\n");
        exit(1);
    }
    lua_pop(L, 1);

    return res_len;
}

size_t exec_handler(lua_State *L, hheader req_hh, char **response,
                    char *handler_name) {
    int status;
    for (uint i = 0; i < middleware_table.n; i++) {
        status =
            regexec(&middleware_table.patterns[i], req_hh.path, 0, NULL, 0);
        if (!status) {
            lua_getglobal(L, middleware_table.middleware[i]);
            lua_insert(L, -2);
            if (lua_pcall(L, 1, 1, 0)) {
                fprintf(stderr, "Error running function %s\n %s\n",
                        middleware_table.middleware[i], lua_tostring(L, -1));
                lua_settop(L, 0);
                return INTERNAL_ERROR;
            }
        }
    }
    lua_getglobal(L, handler_name);
    lua_insert(L, -2);

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        fprintf(stderr, "Error running function %s\n %s\n", handler_name,
                lua_tostring(L, -1));
        lua_settop(L, 0);
        return INTERNAL_ERROR;
    }

    size_t res_len = build_response(L, response);

    return res_len;
}

size_t get_handler_response(lua_State *L, hheader req_hh, char **response) {
    int status;
    for (uint i = 0; i < handler_table.n; i++) {
        status = regexec(&handler_table.patterns[i], req_hh.path, 0, NULL, 0);
        if (!status) {
            return exec_handler(L, req_hh, response, handler_table.handler[i]);
        }
    }
    return 0;
}
