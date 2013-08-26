//
//  RuntimeWiiHeaps.c
//  PSPL
//
//  Created by Jack Andersen on 8/8/13.
//
//

#include <stdio.h>
#include <ogc/system.h>
#include <ogc/lwp_heap.h>
#include <ogc/irq.h>

/* This subsystem is responsible for allocating and maintaining
 * memory-allocation heaps for two key systems of PSPL on the
 * Wii platform.
 * 
 * The Wii has two distinct, physical memory banks 
 * (both physically present on "Hollywood" MCM):
 * * MEM1 - 0x00000000 - 24 MiB - 1T-SRAM (a bit faster)
 * * MEM2 - 0x10000000 - 64 MiB - GDDR3-SDRAM
 * * Total Size: 88 MiB ;)
 * 
 * The two banks are accessible system-wide and both benefit from the 
 * PowerPC's caching system. 
 * 
 * PSPL has a major responsibility in loading data for multimedia
 * subsystems. Therefore, PSPL maintains a *media heap* to provide 
 * enough dedicated space for any retained PSPLC blobs and archived files.
 * By default, the lower 32 MiB of MEM2 is allocated for the media heap.
 * 
 * This mechanism relies on index tables, additionally loaded
 * from the PSPL file. Since the index tables are generally much smaller
 * and loaded in a separate pass, there is a dedicated *indexing heap*.
 * By default, the upper 512 KiB of MEM1 is allocated for the indexing heap.
 * 
 * The heaps are allocated as part of `pspl_runtime_init`
 * 
 * The heap sizes may be overridden by calling `pspl_wii_set_media_heap_size`
 * or `pspl_wii_set_indexing_heap_size` *before* `pspl_runtime_init`
 *
 * Memory blocks are allocated using `pspl_wii_allocate_indexing_block` or
 * `pspl_wii_allocate_media_block` and freed using `pspl_wii_free_indexing_block`
 * or `pspl_wii_free_media_block`
 */

static size_t MEDIA_HEAP_SIZE = 1<<25; // (32 MiB)
static size_t INDEXING_HEAP_SIZE = 1<<19; // (512 KiB)

size_t pspl_wii_set_media_heap_size(size_t size) {
    MEDIA_HEAP_SIZE = size;
    return size;
}

size_t pspl_wii_set_indexing_heap_size(size_t size) {
    INDEXING_HEAP_SIZE = size;
    return size;
}

static heap_cntrl media_heap;
static heap_cntrl indexing_heap;

static uint8_t media_set = 0;
static uint8_t indexing_set = 0;

void _pspl_wii_mem_init() {
    
    
    // MEDIA HEAP
    
    static void* low2;
    static void* new_low2;
    
    // Atomically move low MEM2 arena to allow space for *Media Heap*
    if (!media_set) {
        u32 level = IRQ_Disable();
        low2 = SYS_GetArena2Lo();
        new_low2 = low2 + MEDIA_HEAP_SIZE;
        SYS_SetArena2Lo(new_low2);
        IRQ_Restore(level);
        media_set = 1;
    }
    
    // Make Heap (32-byte paging for pspl-data's alignment expectations)
    __lwp_heap_init(&media_heap, low2, MEDIA_HEAP_SIZE, 32);
    
    
    // INDEXING HEAP
    
    static void* hi1;
    static void* new_hi1;
    
    // Atomically move high MEM1 arena to allow space for *Indexing Heap*
    if (!indexing_set) {
        u32 level = IRQ_Disable();
        hi1 = SYS_GetArena1Hi();
        new_hi1 = hi1 - INDEXING_HEAP_SIZE;
        SYS_SetArena1Hi(new_hi1);
        IRQ_Restore(level);
        indexing_set = 1;
    }
    
    // Make Heap (minimum PowerPC 8-byte paging for (all 32-bit) pspl indexing)
    __lwp_heap_init(&indexing_heap, new_hi1, INDEXING_HEAP_SIZE, 8);
    
    
}


void* pspl_wii_allocate_media_block(size_t size) {
    return __lwp_heap_allocate(&media_heap, size);
}

void* pspl_wii_allocate_indexing_block(size_t size) {
    return __lwp_heap_allocate(&indexing_heap, size);
}


void pspl_wii_free_media_block(void* ptr) {
    __lwp_heap_free(&media_heap, ptr);
}

void pspl_wii_free_indexing_block(void* ptr) {
    __lwp_heap_free(&indexing_heap, ptr);
}
