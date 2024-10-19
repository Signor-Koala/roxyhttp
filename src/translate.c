#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

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

regex_t http_methods_re[2];
regex_t forbidden_re;
regex_t mime_type_re;

void compile_regexes() {

  const char *http_methods[2] = {
      "GET ([[:graph:]]*) HTTP/[0-9][.]?[0-9]?",
      "PUT ([[:graph:]]*) HTTP/[0-9][.]?[0-9]?",
  };
  const char forbidden_path[] = "/../";
  const char mime_type[] = "[.]([A-Za-z0-9]*)$";

  int regex_status;
  char buf[100];
  for (uint i = 0; i < sizeof(http_methods) / sizeof(char *); i++) {
    if ((regex_status =
             regcomp(&http_methods_re[i], http_methods[i], REG_EXTENDED))) {
      regerror(regex_status, &http_methods_re[i], buf, 100);
      fprintf(stderr, "%s http regex compilation error: %s\n", http_methods[i],
              buf);
      exit(1);
    }
  }
  if ((regex_status = regcomp(&forbidden_re, forbidden_path, REG_EXTENDED))) {
    regerror(regex_status, &forbidden_re, buf, 100);
    fprintf(stderr, "%s forbidden regex compilation error: %s\n",
            forbidden_path, buf);
    exit(1);
  }
  if ((regex_status = regcomp(&mime_type_re, mime_type, REG_EXTENDED))) {
    regerror(regex_status, &mime_type_re, buf, 100);
    fprintf(stderr, "%s mime regex compilation error: %s\n", mime_type, buf);
    exit(1);
  }
}

req request_decode(char *request) {
  int regex_status;
  regmatch_t rm[2];
  req req_details = {
      NONE,
      NULL,
  };

  for (uint i = 0; i < sizeof(http_methods_re); i++) {
    if (!(regex_status = regexec(&http_methods_re[i], request, 2, rm, 0))) {
      req_details.method = i + 1;

      if (!(rm[1].rm_so == rm[1].rm_eo)) {
        int l = rm[1].rm_eo - rm[1].rm_so + 1;
        req_details.path = malloc(l * sizeof(char));
        strlcpy(req_details.path, request + rm[1].rm_so, l);
      }
      break;
    } else if (regex_status == REG_NOMATCH) {
      printf("NO MATCH\n %s\n", request);
    } else {
      printf("fail %d", regex_status);
    }
  }
  return req_details;
}
