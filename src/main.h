#ifndef MAIN_H
#define MAIN_H

#include <stddef.h>
enum req_method {
    NONE,
    GET,
    PUT,
};

struct req {
    enum req_method method;
    char *path;
    // Add the rest here later
};

typedef struct req req;

extern char conf_port[6];
extern size_t conf_buffer_size;
extern size_t conf_max_filepath;
extern size_t conf_max_response_size;
extern size_t conf_max_cache_entries;
extern size_t conf_max_entry_size;
extern char *conf_file_path;

#endif // !MAIN_H
