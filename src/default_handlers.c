#include <fcntl.h>
#include <lua.h>
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

static const char header_200_html[] = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/html\r\n"
                                      "\r\n";

static const char header_500_text[] = "HTTP/1.1 500 Internal Server Error\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "Content-Length: 25\r\n"
                                      "\r\n"
                                      "500 Internal Server Error";

// TODO: Allow user to change this
char *error_page_path = "error_pages";

size_t handle_get(hheader req, char **response, lru_table *cache) {
    char filepath[conf_max_filepath];
    strlcpy(filepath, conf_file_path, conf_max_filepath);
    *response = malloc(conf_buffer_size);

    if (!strcmp(req.path, "/")) {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else if (!(regexec(&forbidden_re, req.path, 0, NULL, 0))) {
        // `..` found in path requested
        return FORBIDDEN;
    } else {
        strlcat(filepath, req.path, sizeof(filepath));
    }

    struct stat st;
    if (stat(filepath, &st)) {
        return FILE_NOT_FOUND;
    } else {
        if (conf_max_response_size < st.st_size + sizeof(header_200)) {
            // REQUESTED PAGE TOO LARGE
            return INTERNAL_ERROR;
        }

        char header_ok[conf_buffer_size];
        regmatch_t rm[2];

        /* Checking the file type to set the mime type header */
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

// TODO: Add user handling of errors
size_t handle_error(lua_State *_L, enum ERROR_STATUS status, char **response) {
    char filepath[conf_max_filepath];
    strlcpy(filepath, error_page_path, conf_max_filepath);
    int file_found = 0;
    size_t response_size = 0;

    // Each case is required to check the file's existance
    // Each case also sets the appropriate header
    switch (status) {
    case FORBIDDEN:
        // 400
        break;
    case FILE_NOT_FOUND:
        // 404
        break;
    case METHOD_NOT_ALLOWED:
        // 405
        break;
    case INTERNAL_ERROR:
        // 500
        break;
    case HTTP_PROTO_NOT_IMP:
        // 505
        break;
    case REQUEST_TOO_LONG:
        // 413
        break;
    case INVALID_REQUEST:
        // 400
        break;
    case OK:
        fprintf(stderr, "SHOULDN'T BE HERE\n");
        exit(0);
        break;
    default:
        fprintf(stderr, "ERROR NOT IMPLEMENTED");
        exit(0);
        break;
    }

    if (file_found) {
        // Load file in response
    } else {
        response_size = sizeof(header_500_text);
        *response = realloc(*response, response_size);
        strlcpy(*response, header_500_text, response_size);
    }
    return response_size;
}

size_t add_200_header_html(char **response) {
    size_t size_res = sizeof(header_200_html);
    *response = malloc(conf_buffer_size);

    strlcpy(*response, header_200_html, conf_buffer_size);
    return size_res;
}
