#ifndef HANDLER_H
#define HANDLER_H

#include "cache.h"
#include "main.h"
#include <stddef.h>

#define MAX_RESPONE_SIZE 16777216

size_t handle_get(req request, char **response, lru_table *cache);

#endif // !HANDLER_H
