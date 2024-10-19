#include "cache.h"
#include "main.h"
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

lru_table *create_cache() {
  size_t cache_size =
      sizeof(lru_table) + MAX_CACHE_ENTRIES * (MAX_ENTRY_SIZE + MAX_FILEPATH);
  int cachefd = shm_open("roxyhttp_cache", O_CREAT | O_RDWR | O_TRUNC, 0666);
  if (cachefd < 0) {
    perror("failure on shm_open on des_mutex");
    exit(1);
  }
  if (ftruncate(cachefd, cache_size) == -1) {
    perror("Error on ftruncate to sizeof pthread_cond_t\n");
    exit(-1);
  }
  lru_table *cache = mmap(NULL, cache_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED_VALIDATE, cachefd, 0);
  if (cache == MAP_FAILED) {
    perror("Error on mmap on mutex\n");
    exit(1);
  }

  cache->last_time = 0;
  for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
    cache->time_accessed[i] = 0;
    cache->keys[i] = ((char *)cache + sizeof(lru_table) + i * MAX_FILEPATH);
    cache->data[i] = ((char *)cache + sizeof(lru_table) +
                      MAX_CACHE_ENTRIES * MAX_FILEPATH + i * MAX_ENTRY_SIZE);
  }
  return cache;
}

size_t get_cache(void *cache, char *buf, char *key) {
  lru_table *table = (lru_table *)cache;
  pthread_mutex_lock(&(table->lock));

  for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
    if (table->time_accessed[i] == 0) {
      pthread_mutex_unlock(&(table->lock));
      return 0;
    }
    if (!strcmp(table->keys[i], key)) {
      table->time_accessed[i] = table->last_time + 1;
      memcpy(buf, table->data[i], table->data_size[i]);
      pthread_mutex_unlock(&(table->lock));
      return table->data_size[i];
    }
  }
  pthread_mutex_unlock(&(table->lock));
  return 0;
}

void update_cache(void *cache, char *key, char *data, size_t data_size) {
  if (data_size > MAX_ENTRY_SIZE)
    return;
  lru_table *table = (lru_table *)cache;
  pthread_mutex_lock(&(table->lock));
  long max = table->time_accessed[0];
  long min = max;
  long min_i = 0;
  for (int i = 1; i < MAX_CACHE_ENTRIES; i++) {
    if (min > table->time_accessed[i])
      min_i = i;
  }
  table->last_time++;
  table->time_accessed[min_i] = max + 1;
  memcpy(table->data[min_i], data, data_size);
  table->data_size[min_i] = data_size;
  strlcpy(table->keys[min_i], key, MAX_FILEPATH);
  pthread_mutex_unlock(&(table->lock));
}
