#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "main.h"
#include <lua.h>
#include <regex.h>

extern regex_t mime_type_re;
void compile_regexes();
char **split_request(char *request, size_t *n);
hheader split_hheader(char *header_header);
int build_request(lua_State *L, hheader req_hh, char **request, size_t line_n,
                  size_t *body_size);

extern const char *mime_type_in[77];
extern const char *mime_type_out[77];
extern regex_t forbidden_re;

#endif // !TRANSLATE_H
