#include "default_handlers.h"
#include <fcntl.h>
#include <regex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cache.h"
#include "main.h"
#include "translate.h"

static const char header_200[] = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: %s\r\n"
                                 "\r\n";

static const char header_200_http[] = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/html\r\n"
                                      "\r\n";

static const char header_404[] = "HTTP/1.1 404 Not Found\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "\r\n"
                                 "404 Not Found";

static const char header_403[] = "HTTP/1.1 403 Forbidden\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "\r\n"
                                 "403 Forbidden";

static const char header_500[] = "HTTP/1.1 500 Internal Server Error\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "\r\n"
                                 "502 Internal Server Error";

size_t handle_get(hheader req, char **response, lru_table *cache) {
    char filepath[conf_max_filepath];
    strlcpy(filepath, conf_file_path, conf_max_filepath);
    *response = malloc(conf_buffer_size);

    if (!strcmp(req.path, "/")) {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else if (!(regexec(&forbidden_re, req.path, 0, NULL, 0))) {
        strlcpy(*response, header_403, conf_buffer_size);
        fprintf(stderr, "FORBIDDEN");
        return strlen(header_403);
    } else {
        strlcat(filepath, req.path, sizeof(filepath));
    }

    struct stat st;
    if (stat(filepath, &st)) {
        strlcpy(*response, header_404, conf_buffer_size);
        return strlen(header_404);
    } else {
        if (conf_max_response_size < st.st_size + sizeof(header_200)) {
            fprintf(stderr, "REQUESTED PAGE TOO LARGE\n");
            strlcpy(*response, header_500, conf_buffer_size);
            return strlen(header_500);
        }

        char header_ok[conf_buffer_size];
        regmatch_t rm[2];
        if (!(regexec(&mime_type_re, filepath, 2, rm, 0))) {
            for (uint i = 0; i < sizeof(mime_type_in); i++) {
                if (!strcmp(&filepath[rm[1].rm_so], mime_type_in[i])) {
                    sprintf(header_ok, header_200, mime_type_out[i]);
                    break;
                }
            }
        }

        size_t header_size = strlcpy(*response, header_ok, conf_buffer_size);

        size_t response_size = header_size + st.st_size;
        if (response_size > conf_buffer_size)
            *response = realloc(*response, st.st_size + header_size + 1);

        if (get_cache(cache, *response, filepath)) {
            return response_size;
        }

        int fptr = open(filepath, O_RDONLY);
        for (uint i = 0; i < ((st.st_size) / conf_buffer_size) + 1; i++) {
            read(fptr, &(*response)[header_size + i * conf_buffer_size],
                 conf_buffer_size);
        }
        (*response)[response_size] = '\0';

        update_cache(cache, filepath, *response, response_size + 1);

        return response_size;
    }
}

size_t add_200_header(char **response) {
    size_t size_res = sizeof(header_200_http);
    *response = malloc(conf_buffer_size);

    strlcpy(*response, header_200_http, conf_buffer_size);
    return size_res;
}
