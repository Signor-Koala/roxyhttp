#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "main.h"
#include <regex.h>

extern regex_t mime_type_re;
void compile_regexes();
char **split_request(char *request, size_t req_len, size_t *body_len,
                     size_t *n);
hheader split_hheader(char *header_header);

extern const char *mime_type_in[77];
extern const char *mime_type_out[77];
extern regex_t forbidden_re;

#endif // !TRANSLATE_H
