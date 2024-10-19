#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <stddef.h>
#define MAX_CACHE_ENTRIES 4 // 100
#define MAX_ENTRY_SIZE 4096 // 1048576
// I'll try this later
// #define MAX_TOTAL_SIZE 8196  // 10485760

struct lru_table {
  pthread_mutex_t lock;
  long last_time;
  long time_accessed[MAX_CACHE_ENTRIES];
  size_t data_size[MAX_CACHE_ENTRIES];
  char *keys[MAX_CACHE_ENTRIES];
  char *data[MAX_CACHE_ENTRIES];
};

typedef struct lru_table lru_table;

extern lru_table *create_cache();
extern size_t get_cache(void *cache, char *buf, char *key);
extern void update_cache(void *cache, char *key, char *data, size_t data_size);

#endif // !CACHE_H
