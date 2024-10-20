#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <stddef.h>

#include "main.h"
// I'll try this later
// #define MAX_TOTAL_SIZE 8196  // 10485760

struct lru_table {
    pthread_mutex_t lock;
    long last_time;
    long *time_accessed;
    size_t *data_size;
    char *keys;
    char *data;
};

typedef struct lru_table lru_table;

extern lru_table *create_cache();
extern size_t get_cache(void *cache, char *buf, char *key);
extern void update_cache(void *cache, char *key, char *data, size_t data_size);

#endif // !CACHE_H
