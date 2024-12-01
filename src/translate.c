#include "translate.h"
#include "main.h"
#include <lua.h>
#include <regex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

const char *mime_type_out[] = {
    "audio/aac",
    "application/x-abiword",
    "image/apng",
    "application/x-freearc",
    "image/avif",
    "video/x-msvideo",
    "application/vnd.amazon.ebook",
    "application/octet-stream",
    "image/bmp",
    "application/x-bzip",
    "application/x-bzip2",
    "application/x-cdf",
    "application/x-csh",
    "text/css",
    "text/csv",
    "application/msword",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/vnd.ms-fontobject",
    "application/epub+zip",
    "application/gzip",
    "image/gif",
    "text/html",
    "text/html",
    "image/vnd.microsoft.icon",
    "text/calendar",
    "application/java-archive",
    "image/jpeg",
    "image/jpeg",
    "text/javascript",
    "application/json",
    "application/ld+json",
    "audio/midi",
    "audio/midi",
    "text/javascript",
    "audio/mpeg",
    "video/mp4",
    "video/mpeg",
    "application/vnd.apple.installer+xml",
    "application/vnd.oasis.opendocument.presentation",
    "application/vnd.oasis.opendocument.spreadsheet",
    "application/vnd.oasis.opendocument.text",
    "audio/ogg",
    "video/ogg",
    "application/ogg",
    "audio/ogg",
    "font/otf",
    "image/png",
    "application/pdf",
    "application/x-httpd-php",
    "application/vnd.ms-powerpoint",
    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "application/vnd.rar",
    "application/rtf",
    "application/x-sh",
    "image/svg+xml",
    "application/x-tar",
    "image/tiff",
    "image/tiff",
    "video/mp2t",
    "font/ttf",
    "text/plain",
    "application/vnd.visio",
    "audio/wav",
    "audio/webm",
    "video/webm",
    "image/webp",
    "font/woff",
    "font/woff2",
    "application/xhtml+xml",
    "application/vnd.ms-excel",
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/xml",
    "application/vnd.mozilla.xul+xml",
    "application/zip",
    "video/3gpp",
    "video/3gpp2",
    "application/x-7z-compressed",
};

const char *mime_type_in[] = {
    "aac",  "abw",  "apng", "arc",    "avif",  "avi",   "azw", "bin",  "bmp",
    "bz",   "bz2",  "cda",  "csh",    "css",   "csv",   "doc", "docx", "eot",
    "epub", "gz",   "gif",  "html",   "htm",   "ico",   "ics", "jar",  "jpeg",
    "jpg",  "js",   "json", "jsonld", "mid",   "midi",  "mjs", "mp3",  "mp4",
    "mpeg", "mpkg", "odp",  "ods",    "odt",   "oga",   "ogv", "ogx",  "opus",
    "otf",  "png",  "pdf",  "php",    "ppt",   "pptx",  "rar", "rtf",  "sh",
    "svg",  "tar",  "tif",  "tiff",   "ts",    "ttf",   "txt", "vsd",  "wav",
    "weba", "webm", "webp", "woff",   "woff2", "xhtml", "xls", "xlsx", "xml",
    "xul",  "zip",  "3gp",  "3g2",    "7z",
};

regex_t forbidden_re;
regex_t mime_type_re;

void compile_regexes() {

    const char forbidden_path[] = "/../";
    const char mime_type[] = "[.]([A-Za-z0-9]*)$";

    int regex_status;
    char buf[100];

    if ((regex_status = regcomp(&forbidden_re, forbidden_path, REG_EXTENDED))) {
        regerror(regex_status, &forbidden_re, buf, 100);
        fprintf(stderr, "%s forbidden regex compilation error: %s\n",
                forbidden_path, buf);
        exit(1);
    }
    if ((regex_status = regcomp(&mime_type_re, mime_type, REG_EXTENDED))) {
        regerror(regex_status, &mime_type_re, buf, 100);
        fprintf(stderr, "%s mime regex compilation error: %s\n", mime_type,
                buf);
        exit(1);
    }
}

hheader split_hheader(char *header_header) {
    hheader h;
    h.method = strtok(header_header, " ");
    h.path = strtok(NULL, " ");
    h.protocol = strtok(NULL, "");
    return h;
}

int build_request(lua_State *L, hheader req_hh, char **request, size_t line_n,
                  size_t *body_size) {
    lua_newtable(L);
    lua_pushstring(L, req_hh.method);
    lua_setfield(L, -2, "method");
    lua_pushstring(L, req_hh.path);
    lua_setfield(L, -2, "path");
    lua_pushstring(L, req_hh.protocol);
    lua_setfield(L, -2, "protocol");
    for (uint i = 1; i < line_n; i++) {
        if (i == line_n - 1) {
            return 0;
        }
        char *key = strtok(request[i], ":");
        char *value = strtok(NULL, "") + sizeof(char);
        if (!strcasecmp(key, "Content-Length")) {
            *body_size = atoi(value);
        }
        lua_pushstring(L, value);
        lua_setfield(L, -2, key);
    }
    return 1;
}

char **split_request(char *request, size_t *n) {
    char **l = realloc(NULL, sizeof(char *));
    if (l == NULL) {
        perror("Error while allocating memory for splitting request\n");
        exit(1);
    }
    l[0] = strtok(request, "\n");
    int len = strlen(l[0]);
    if (l[0][len - 1] == '\r') {
        l[0][len - 1] = '\0';
    }
    for (int i = 1;; i++) {
        char **temp = realloc(l, sizeof(char *) * (i + 1));
        if (temp == NULL) {
            perror("Error while allocating memory for splitting request\n");
            exit(1);
        }
        l = temp;
        l[i] = strtok(NULL, "\n");
        if (l[i] == NULL) {
            fprintf(stderr, "No more \\n found");
            exit(1);
        }
        len = strlen(l[i]);
        if (l[i][len - 1] == '\r') {
            l[i][len - 1] = '\0';
        }
        if (len < 2) {
            l[i] = strtok(NULL, "");
            *n = i + 1;
            return l;
        }
    }
}
