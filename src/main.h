#ifndef MAIN_H
#define MAIN_H

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

#define BUFFER_SIZE 2048
#define MAX_FILEPATH 256

#endif // !MAIN_H
