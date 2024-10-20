#ifndef HANDLER_H
#define HANDLER_H

#include "cache.h"
#include "main.h"
#include <stddef.h>

size_t handle_get(req request, char **response, lru_table *cache);

#endif // !HANDLER_H
