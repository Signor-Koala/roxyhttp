#ifndef MAIN_H
#define MAIN_H

#include <stddef.h>
enum req_method {
    NONE,
    GET,
    PUT,
};

struct hheader {
    char *method;
    char *path;
    char *protocol;
};

typedef struct hheader hheader;

extern char conf_port[6];
extern size_t conf_buffer_size;
extern size_t conf_max_filepath;
extern size_t conf_max_response_size;
extern size_t conf_max_cache_entries;
extern size_t conf_max_entry_size;
extern char *conf_file_path;

#endif // !MAIN_H
