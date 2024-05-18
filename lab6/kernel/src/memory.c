#include "memory.h"
#include "u_list.h"
#include "uart1.h"
#include "exception.h"
#include "dtb.h"

extern char _heap_top;
static char* htop_ptr = &_heap_top;

extern char  _start;
extern char  _end;
extern char  _stack_top;
extern char* CPIO_DEFAULT_START;
extern char* CPIO_DEFAULT_END;
extern char* dtb_ptr;

#ifdef DEBUG
    #define memory_sendline(fmt, args ...) uart_sendline(fmt, ##args)
#else
    #define memory_sendline(fmt, args ...) (void)0
#endif

// ------ Lab2 ------
void* s_allocator(unsigned int size) {
    // -> htop_ptr
    // htop_ptr + 0x02:  heap_block size
    // htop_ptr + 0x10 ~ htop_ptr + 0x10 * k:
    //            { heap_block }
    // -> htop_ptr

    // 0x10 for heap_block header
    char* r = htop_ptr + 0x10;
    // size paddling to multiple of 0x10
    size = 0x10 + size - size % 0x10;
    *(unsigned int*)(r - 0x8) = size;
    htop_ptr += size;
    return r;
}

void s_free(void* ptr) {
    // To do?
}

// ------ Lab4 allocator------
static frame_t*           frame_array; // stores whole physical address frame's statement and index
static list_head_t        frame_freelist[FRAME_INDEX_MAX ]; // store available block for frame(use linklist)
static list_head_t        cache_list[CACHE_INDEX_MAX ]; // store available block for cache(use linklist)

void init_allocator()
{
    frame_array = s_allocator(BUDDY_MEMORY_PAGE_COUNT * sizeof(frame_t));

    // init frame_array 0, 64, 128, ..., 0x3c000
    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    {
        if (i % (1 << FRAME_INDEX_FINAL) == 0)
        {
            frame_array[i].val = FRAME_INDEX_FINAL;
            frame_array[i].used = FRAME_STATUS_FREE;
        }
    }

    //init frame freelist 1, 2, 3, 4, 5, 6
    for (int i = FRAME_INDEX_0; i <= FRAME_INDEX_FINAL; i++)
    {
        INIT_LIST_HEAD(&frame_freelist[i]);
    }

    //init cache list 1, 2, 3, 4, 5, 6
    for (int i = CACHE_INDEX_0; i<= CACHE_INDEX_FINAL; i++)
    {
        INIT_LIST_HEAD(&cache_list[i]);
    }

    for (int i = 0; i < BUDDY_MEMORY_PAGE_COUNT; i++)
    {
        //init listhead for each frame
        INIT_LIST_HEAD(&frame_array[i].listhead);
        frame_array[i].idx = i;
        frame_array[i].cache_order = CACHE_STATUS_NONE;

        //add init frame into freelist
        if (i % (1 << FRAME_INDEX_FINAL) == 0)
        {
            list_add(&frame_array[i].listhead, &frame_freelist[FRAME_INDEX_FINAL]);
        }
    }
    uart_sendline("        physical address : 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE*(frame_array[0].idx)));
    uart_sendline("        physical address : 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE*(frame_array[1].idx)));
    /* Below are the memory regions that have to be reserved
    Spin tables for multicore boot (0x0000 - 0x1000)
    Devicetree (Optional, if you have implement it)
    Kernel image in the physical memory
    Simple allocator (startup allocator)(Stack + Heap)
    Initramfs
    */
    memory_sendline("\r\n* Startup Allocation *\r\n");
    memory_sendline("buddy system: usable memory region: 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE, BUDDY_MEMORY_BASE + BUDDY_MEMORY_PAGE_COUNT * PAGESIZE);
    dtb_find_and_store_reserved_memory(); // find spin tables in dtb
    memory_reserve((unsigned long long)&_start, (unsigned long long)&_end); // kernel
    memory_reserve((unsigned long long)&_heap_top, (unsigned long long)&_stack_top);  // heap & stack -> simple allocator
    memory_reserve((unsigned long long)CPIO_DEFAULT_START, (unsigned long long)CPIO_DEFAULT_END); //Initramfs
}

//smallest 4K
void* page_malloc(unsigned int size)
{
    memory_sendline("    [+] Allocate page - size : %d(0x%x)\r\n", size, size);
    memory_sendline("        Before\r\n");
    show_page_info();

    // 用val來儲存size在所有index裡面的級距
    int val;
    for (int i = FRAME_INDEX_0; i <= FRAME_INDEX_FINAL; i++)
    {

        // PAGESIZE << 0 = 0x1000
        // PAGESIZE << 1 = 0x2000
        // PAGESIZE << 2 = 0x4000
        //...
        if (size <= (PAGESIZE << i))
        {
            val = i;
            memory_sendline("        block size = 0x%x\n", PAGESIZE << i);
            break;
        }

        if ( i == FRAME_INDEX_FINAL)
        {
            memory_sendline("[!] request size exceeded for page_malloc!!!!\r\n");
            return (void*)0;
        }

    }

    // find the smallest larger frame in freelist
    int target_val;
    for (target_val = val; target_val <= FRAME_INDEX_FINAL; target_val++)
    {
        // freelist does not have 2**i order frame, going for next order
        if (!list_empty(&frame_freelist[target_val]))
            break;
    }

    if (target_val > FRAME_INDEX_FINAL)
    {
        memory_sendline("[!] No available frame in freelist, page_malloc ERROR!!!!\r\n");
        return (void*)0;
    }
    // get the target available frame from freelist
    frame_t *target_frame_ptr = (frame_t*)frame_freelist[target_val].next;
    list_del_entry((struct list_head *)target_frame_ptr);

    // Release redundant memory block to separate into pieces
    // ex: target = 5, val = 3
    // 00001 -> 00110
    for (int j = target_val; j > val; j--) 
    {
        //uart_sendline("        split 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE*(target_frame_ptr->idx)), BUDDY_MEMORY_BASE + ((PAGESIZE*(target_frame_ptr->idx))+(PAGESIZE<<target_frame_ptr->val)));
        split_frame(target_frame_ptr);
    }
    // 把目標frame改為ALLOCATED
    target_frame_ptr->used = FRAME_STATUS_ALLOCATED;
    memory_sendline("        physical address : 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE*(target_frame_ptr->idx)));
    memory_sendline("        The number of times to allocate a page : %d\r\n", target_val - val);
    memory_sendline("        After\r\n");
    show_page_info();

    return (void *) BUDDY_MEMORY_BASE + (PAGESIZE * (target_frame_ptr->idx));
}

void page_free(void* ptr)
{
    //取得要free的frame index
    frame_t *target_frame_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12]; // MAX_PAGES * 64bit -> 0x1000 * 0x10000000
    memory_sendline("    [+] Free page: 0x%x, val = %d\r\n",ptr, target_frame_ptr->val);
    memory_sendline("        Before\r\n");
    show_page_info();
    target_frame_ptr->used = FRAME_STATUS_FREE;
    frame_t* temp;
    while ((temp = coalesce(target_frame_ptr)) != (frame_t *)-1)target_frame_ptr = temp;
    list_add(&target_frame_ptr->listhead, &frame_freelist[target_frame_ptr->val]);
    memory_sendline("        After\r\n");
    show_page_info();
}

frame_t* split_frame(frame_t *frame)
{
    // 先將自身的val - 1 , 然後接著找鄰近的buddy 
    frame->val -= 1;
    //memory_sendline("            to 0x%x ~ 0x%x", BUDDY_MEMORY_BASE + (PAGESIZE*(frame->idx)), BUDDY_MEMORY_BASE + ((PAGESIZE*(frame->idx))+(PAGESIZE<<frame->val)));
    frame_t *buddyptr = get_buddy(frame);
    buddyptr->val = frame->val;
    //memory_sendline(" and 0x%x ~ 0x%x\n", BUDDY_MEMORY_BASE + (PAGESIZE*(buddyptr->idx)), BUDDY_MEMORY_BASE + ((PAGESIZE*(buddyptr->idx))+(PAGESIZE<<buddyptr->val)));
    list_add(&buddyptr->listhead, &frame_freelist[buddyptr->val]);
    return frame;
}

frame_t* get_buddy(frame_t *frame)
{
    // XOR(INDEX, order)
    // ex. frame->idx ^ (1 << frame->val) -> 63 ^ (1<<4) = 47
    return &frame_array[frame->idx ^ (1 << frame->val)];
}

frame_t* coalesce(frame_t *frame_ptr)
{
    frame_t *buddy = get_buddy(frame_ptr);
    // 當前已是最大值6
    if (frame_ptr->val == FRAME_INDEX_FINAL){
        //uart_sendline("    coalesce FAIL : current frame have max frame index\n");
        return (frame_t*)-1;
    }

    // buddy的val不同
    if (frame_ptr->val != buddy->val){
        //uart_sendline("    coalesce FAIL : buddy val different\n");
        return (frame_t*)-1;
    }

    // buddy是已使用的狀態
    if (buddy->used == FRAME_STATUS_ALLOCATED){
        //uart_sendline("    coalesce FAIL : buddy has been allocated\n");
        return (frame_t*)-1;
    }

    list_del_entry((struct list_head *)buddy);
    frame_ptr->val += 1;
    //memory_sendline("    coalesce SUCESS : merging 0x%x, 0x%x, -> val = %d\r\n", frame_ptr->idx, buddy->idx, frame_ptr->val);
    return buddy < frame_ptr ? buddy : frame_ptr;
}

void show_page_info(){
    unsigned int exp2 = 1;
    memory_sendline("        ----------------- [  Number of Available Page Blocks  ] -----------------\r\n        | ");
    for (int i = FRAME_INDEX_0; i <= FRAME_INDEX_FINAL; i++)
    {
        memory_sendline("%4dKB(%1d) ", 4*exp2, i);
        exp2 *= 2;
    }
    memory_sendline("|\r\n        | ");
    for (int i = FRAME_INDEX_0; i <= FRAME_INDEX_FINAL; i++)
        memory_sendline("     %4d ", list_size(&frame_freelist[i]));
    memory_sendline("|\r\n");
}

void show_cache_info()
{
    unsigned int exp2 = 1;
    memory_sendline("    -- [  Number of Available Cache Blocks ] --\r\n    | ");
    for (int i = CACHE_INDEX_0; i <= CACHE_INDEX_FINAL; i++)
    {
        memory_sendline("%4dB(%1d) ", 32*exp2, i);
        exp2 *= 2;
    }
    memory_sendline("|\r\n    | ");
    for (int i = CACHE_INDEX_0; i <= CACHE_INDEX_FINAL; i++)
        memory_sendline("   %5d ", list_size(&cache_list[i]));
    memory_sendline("|\r\n");
}

void convert_page_to_caches(int order)
{
    // make caches from a smallest-size page (0x1000)
    char *page = page_malloc(PAGESIZE);
    // 取得此page的index
    frame_t *pageframe_ptr = &frame_array[((unsigned long long)page - BUDDY_MEMORY_BASE) >> 12];
    pageframe_ptr->cache_order = order;

    // split page into a lot of caches and push them into cache_list
    int cachesize = (32 << order);
    for (int i = 0; i < PAGESIZE; i += cachesize)
    {
        // 使一個page被切為多個cachesize大小的片段，並加入到cache_list
        list_head_t *c = (list_head_t *)(page + i);
        list_add(c, &cache_list[order]);
    }
}

void* cache_malloc(unsigned int size)
{
    memory_sendline("[+] Allocate cache - size : %d(0x%x)\r\n", size, size);
    memory_sendline("    Before\r\n");
    show_cache_info();
    int order;
    // 32 << 0 = 0x20
    // 32 << 1 = 0x40
    // 32 << 2 = 0x80
    //...
    for (int i = CACHE_INDEX_0; i <= CACHE_INDEX_FINAL; i++)
    {
        if (size <= (32 << i)) { order = i; break; }
    }
    // 若沒有可用的cache_list 就去page切一塊來用
    if (list_empty(&cache_list[order]))
    {
        convert_page_to_caches(order);
    }
    // 此時已確定有cahe_list可用
    list_head_t* r = cache_list[order].next;
    list_del_entry(r);
    memory_sendline("    physical address : 0x%x\n", r);
    memory_sendline("    After\r\n");
    show_cache_info();
    return r;
}

void cache_free(void *ptr)
{
    list_head_t *c = (list_head_t *)ptr;
    frame_t *pageframe_ptr = &frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12];
    memory_sendline("[+] Free cache: 0x%x, val = %d\r\n",ptr, pageframe_ptr->cache_order);
    memory_sendline("    Before\r\n");
    show_cache_info();
    list_add(c, &cache_list[pageframe_ptr->cache_order]);
    memory_sendline("    After\r\n");
    show_cache_info();
}

void *kmalloc(unsigned int size)
{
    memory_sendline("\n\n");
    memory_sendline("================================\r\n");
    memory_sendline("[+] Request kmalloc size: %d\r\n", size);
    memory_sendline("================================\r\n");
    //For page
    if (size > (32 << CACHE_INDEX_FINAL))
    {
        void *r = page_malloc(size);
        return r;
    }
    void *r = cache_malloc(size);
    return r;
}

void kfree(void *ptr)
{
    memory_sendline("\n\n");
    memory_sendline("==========================\r\n");
    memory_sendline("[+] Request kfree 0x%x\r\n", ptr);
    memory_sendline("==========================\r\n");
    // For page
    // 確保ptr是以page為單位
    if ((unsigned long long)ptr % PAGESIZE == 0 && frame_array[((unsigned long long)ptr - BUDDY_MEMORY_BASE) >> 12].cache_order == CACHE_STATUS_NONE)
    {
        page_free(ptr);
        return;
    }
    cache_free(ptr);
}

void memory_reserve(unsigned long long start, unsigned long long end)
{
    start -= start % PAGESIZE; // floor (align 0x1000)
    end = end % PAGESIZE ? end + PAGESIZE - (end % PAGESIZE) : end; // ceiling (align 0x1000)

    uart_sendline("Reserved Memory: ");
    uart_sendline("start 0x%x ~ ", start);
    uart_sendline("end 0x%x\r\n",end);

    // delete page from freelist
    // 從INDEX6開始往下檢查
    for (int order = FRAME_INDEX_FINAL; order >= 0; order--)
    {
        list_head_t *pos;
        // 依序檢查free_list內的frame page
        list_for_each(pos, &frame_freelist[order])
        {
            // 取得此frame的pyhsical address的起始和結尾
            unsigned long long pagestart = ((frame_t *)pos)->idx * PAGESIZE + BUDDY_MEMORY_BASE;
            unsigned long long pageend = pagestart + (PAGESIZE << order);
    
            if (start <= pagestart && end >= pageend) // 整個page都在reserved memory內，從freelist刪掉
            {
                ((frame_t *)pos)->used = FRAME_STATUS_ALLOCATED;
                memory_sendline("    [!] Reserved page in 0x%x - 0x%x\n", pagestart, pageend);
                memory_sendline("        Before\n");
                show_page_info();
                list_del_entry(pos);
                memory_sendline("        Remove usable block for reserved memory: order %d\r\n", order);
                memory_sendline("        After\n");
                show_page_info();
            }
            else if (start >= pageend || end <= pagestart) // 沒有重疊的memory
            {
                continue;
            }
            else // 部分重疊
            {
                list_del_entry(pos);
                list_head_t *temppos = pos -> prev;
                // 1. 先把自身的val - 1並加入到freelist
                // 2. 用split_frame內的get_buddy(切一半且val - 1)，並把切完的加到freelist
                list_add(&split_frame((frame_t *)pos)->listhead, &frame_freelist[order - 1]);
                pos = temppos;
            }
        }
    }
}