#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "main.h"
#include <regex.h>

extern regex_t http_methods_re[3];
extern regex_t mime_type_re;
extern regex_t forbidden_re;
void compile_regexes();
req request_decode(char *request);

extern const char *mime_type_in[77];
extern const char *mime_type_out[77];

#endif // !TRANSLATE_H
