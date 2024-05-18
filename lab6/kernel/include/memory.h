#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "u_list.h"

/* Lab2 */
void* s_allocator(unsigned int size);
void  s_free(void* ptr);

/* Lab4 */
#define BUDDY_MEMORY_BASE       PHYS_TO_VIRT(0x0)     // 0x10000000 - 0x20000000 (SPEC) -> Advanced #3 for all memory region
#define BUDDY_MEMORY_PAGE_COUNT 0x3C000 // let BUDDY_MEMORY use 0x0 ~ 0x3C000000 (SPEC on course website)
#define PAGESIZE    0x1000     // 4KB
#define MAX_PAGES   0x10000    // 65536 (Entries), PAGESIZE * MAX_PAGES = 0x10000000 (SPEC)

typedef enum {
    FRAME_STATUS_FREE = -2,  // Indicates the frame is free
    FRAME_STATUS_ALLOCATED,  // Indicates the frame is allocated

    FRAME_INDEX_0 = 0,      // 4KB
    FRAME_INDEX_1,          // 8KB
    FRAME_INDEX_2,          // 16KB
    FRAME_INDEX_3,          // 32KB
    FRAME_INDEX_4,          // 64KB
    FRAME_INDEX_5,          // 128KB
    FRAME_INDEX_6,          // 256KB
    FRAME_INDEX_FINAL = 7,  // 512KB
    FRAME_INDEX_MAX = 8     // Maximum index count
} frame_value_type;

typedef enum {
    CACHE_STATUS_NONE = -1,  // Indicates the cache is not used

    CACHE_INDEX_0 = 0,      // 32B
    CACHE_INDEX_1,          // 64B
    CACHE_INDEX_2,          // 128B
    CACHE_INDEX_3,          // 256B
    CACHE_INDEX_4,          // 512B
    CACHE_INDEX_5,          // 1KB
    CACHE_INDEX_FINAL = 6,  // 2KB
    CACHE_INDEX_MAX = 7     // Maximum index count
} cache_value_type;

typedef struct frame
{
    struct list_head listhead; // store freelist
    int val;                   // store order
    int used;
    int cache_order;
    unsigned int idx;
} frame_t;

void init_allocator();
frame_t *split_frame(frame_t *frame);
frame_t *get_buddy(frame_t *frame);
frame_t *coalesce(frame_t *frame_ptr);

void show_page_info();
void show_cache_info();

//buddy system
void* page_malloc(unsigned int size);
void  page_free(void *ptr);
void  convert_page_to_caches(int order);
void* cache_malloc(unsigned int size);
void  cache_free(void* ptr);

void* kmalloc(unsigned int size);
void  kfree(void *ptr);
void  memory_reserve(unsigned long long start, unsigned long long end);

#endif /* _MEMORY_H_ */