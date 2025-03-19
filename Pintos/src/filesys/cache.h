#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/block.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include <stdbool.h>
#include <stdint.h>

/* Number of cache entries. */
#define CACHE_SIZE 64

/* A cache entry. */
struct buffer_head {
    block_sector_t sector;                   // Disk sector number.
    bool valid;                              // Valid bit.
    bool dirty;                              // Dirty bit.
    bool accessed;                           // Accessed bit for eviction.
    uint8_t data[BLOCK_SECTOR_SIZE];         // Data buffer.
    struct lock lock;                        // Lock for this cache entry.
    struct semaphore read_sema;              // Semaphore for read synchronization.
    int reader_cnt;                          // Number of readers.
};


void cache_init(void);
struct buffer_head *cache_lookup(block_sector_t sector);
struct buffer_head *cache_evict(void);
void cache_read(block_sector_t sector, void *buffer, off_t offset, off_t size);
void cache_write(block_sector_t sector, const void *buffer, off_t offset, off_t size);
void cache_flush(void);
void cache_destroy(void);

#endif