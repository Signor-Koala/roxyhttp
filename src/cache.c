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
        sizeof(lru_table) +
        conf_max_cache_entries *
            (conf_max_entry_size + conf_max_filepath * sizeof(char) +
             sizeof(size_t) + sizeof(long));
    int cachefd = shm_open("roxyhttp_cache", O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (cachefd < 0) {
        perror("failure on shm_open on des_mutex");
        exit(1);
    }
    if (ftruncate(cachefd, cache_size) == -1) {
        perror("Error on ftruncate to sizeof pthread_cond_t\n");
        exit(-1);
    }
    void *cache = mmap(NULL, cache_size, PROT_READ | PROT_WRITE,
                       MAP_SHARED_VALIDATE, cachefd, 0);
    lru_table *table = cache;
    if (cache == MAP_FAILED) {
        perror("Error on mmap on mutex\n");
        exit(1);
    }

    table->last_time = 0;
    table->time_accessed = (long *)(cache + sizeof(lru_table));
    table->data_size = (size_t *)(cache + sizeof(lru_table) +
                                  conf_max_cache_entries * sizeof(long));
    table->keys =
        (char *)(cache + sizeof(lru_table) +
                 conf_max_cache_entries * (sizeof(long) + sizeof(size_t)));
    table->data =
        (char *)(cache + sizeof(lru_table) +
                 conf_max_cache_entries *
                     (sizeof(long) + sizeof(size_t) + conf_max_filepath));
    for (uint i = 0; i < conf_max_cache_entries; i++)
        table->time_accessed[i] = 0;
    return cache;
}

size_t get_cache(void *cache, char *buf, char *key) {
    lru_table *table = (lru_table *)cache;
    pthread_mutex_lock(&(table->lock));

    for (uint i = 0; i < conf_max_cache_entries; i++) {
        if (table->time_accessed[i] == 0) {
            pthread_mutex_unlock(&(table->lock));
            return 0;
        }
        if (!strcmp(&table->keys[conf_max_filepath * i], key)) {
            table->time_accessed[i] = table->last_time + 1;
            memcpy(buf, &table->data[conf_max_entry_size * i],
                   table->data_size[i]);
            pthread_mutex_unlock(&(table->lock));
            return table->data_size[i];
        }
    }
    pthread_mutex_unlock(&(table->lock));
    return 0;
}

void update_cache(void *cache, char *key, char *data, size_t data_size) {
    size_t cache_size =
        sizeof(lru_table) +
        conf_max_cache_entries * (conf_max_entry_size + conf_max_filepath +
                                  sizeof(size_t) + sizeof(long));
    if (data_size > conf_max_entry_size)
        return;
    lru_table *table = (lru_table *)cache;
    pthread_mutex_lock(&(table->lock));
    long max = table->last_time;
    long min = max;
    long min_i = 0;
    for (uint i = 1; i < conf_max_cache_entries; i++) {
        if (min > table->time_accessed[i]) {
            min_i = i;
            min = table->time_accessed[i];
        }
    }
    table->last_time++;
    table->time_accessed[min_i] = max + 1;

    memcpy((&table->data[conf_max_entry_size * min_i]), data, data_size);
    table->data_size[min_i] = data_size;
    strlcpy(&table->keys[conf_max_filepath * min_i], key, conf_max_filepath);
    pthread_mutex_unlock(&(table->lock));
}
