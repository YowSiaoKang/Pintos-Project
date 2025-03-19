#include "filesys/cache.h"
#include "devices/timer.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include <string.h>
#include <stdio.h>

static struct buffer_head cache[CACHE_SIZE];
static struct lock cache_lock;         // Global cache lock.
static size_t clock_hand = 0;          // Clock hand for eviction policy.
static struct lock clock_lock;         // Lock for clock hand.

static void cache_flush_entry(struct buffer_head *entry);

/* Initialize the buffer cache. */
void
cache_init(void) {
    lock_init(&cache_lock);
    lock_init(&clock_lock);
    int i;
    for (i = 0; i < CACHE_SIZE; i++) {
        cache[i].valid = false;
        cache[i].dirty = false;
        cache[i].accessed = false;
        cache[i].sector = (block_sector_t) -1;
        lock_init(&cache[i].lock);
        sema_init(&cache[i].read_sema, 1);
        cache[i].reader_cnt = 0;
    }
}

/* Find a cache entry for the given sector. Returns NULL if not found. */
struct buffer_head *
cache_lookup(block_sector_t sector) {
    int i;
    for (i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].valid && cache[i].sector == sector) {
            return &cache[i];
        }
    }
    return NULL;
}

/* Select a cache entry to evict using the clock algorithm. */
struct buffer_head *cache_evict(void) {
    static int clock_hand = 0; 

    while (true) {
        struct buffer_head *entry = &cache[clock_hand];
        clock_hand = (clock_hand + 1) % 64;

        if (!entry->valid) {
            return entry;
        }

        if (!entry->accessed) {
            if (entry->dirty) {
                
                block_write(fs_device, entry->sector, entry->data);
                entry->dirty = false;
            }
            entry->valid = false; 
            return entry;
        }

        // Reset the accessed flag for the next round
        entry->accessed = false;
    }
}



/* Read data from the cache or disk into the buffer. */
void
cache_read(block_sector_t sector, void *buffer, off_t offset, off_t size) {
    ASSERT(offset + size <= BLOCK_SECTOR_SIZE);
    lock_acquire(&cache_lock);
    struct buffer_head *entry = cache_lookup(sector);

    if (entry == NULL) {
        entry = cache_evict();
        ASSERT(entry != NULL);
        lock_acquire(&entry->lock);
        entry->sector = sector;
        entry->valid = true;
        entry->dirty = false;
        entry->accessed = true;
        lock_release(&cache_lock);
        /* Read from disk. */
        block_read(fs_device, sector, entry->data);
    } else {
        lock_acquire(&entry->lock);
        entry->accessed = true;
        lock_release(&cache_lock);
    }

    /* Read data from cache entry to buffer. */
    memcpy(buffer, entry->data + offset, size);
    lock_release(&entry->lock);
}

/* Write data to the cache, marking it dirty. */
void
cache_write(block_sector_t sector, const void *buffer, off_t offset, off_t size) {
    ASSERT(offset + size <= BLOCK_SECTOR_SIZE);
    lock_acquire(&cache_lock);
    struct buffer_head *entry = cache_lookup(sector);

    if (entry == NULL) {
        entry = cache_evict();
        ASSERT(entry != NULL);
        lock_acquire(&entry->lock);
        entry->sector = sector;
        entry->valid = true;
        entry->dirty = true;
        entry->accessed = true;
        lock_release(&cache_lock);
        /* Read from disk to ensure the cache entry contains up-to-date data. */
        block_read(fs_device, sector, entry->data);
    } else {
        lock_acquire(&entry->lock);
        entry->accessed = true;
        lock_release(&cache_lock);
    }

    /* Write data to cache entry from buffer. */
    memcpy(entry->data + offset, buffer, size);
    entry->dirty = true;
    lock_release(&entry->lock);
}

/* Flush a specific cache entry to disk if it's dirty. */
static void
cache_flush_entry(struct buffer_head *entry) {
    ASSERT(lock_held_by_current_thread(&entry->lock));
    if (entry->dirty) {
        block_write(fs_device, entry->sector, entry->data);
        entry->dirty = false;
    }
}

/* Flush all cache entries to disk. */
void
cache_flush(void) {
    int i;
    for (i = 0; i < CACHE_SIZE; i++) {
        lock_acquire(&cache[i].lock);
        if (cache[i].valid && cache[i].dirty) {
            cache_flush_entry(&cache[i]);
        }
        lock_release(&cache[i].lock);
    }
}

/* Destroy the cache, flushing all entries. */
void
cache_destroy(void) {
    cache_flush();
}
