/* Name:        arenas.h
 * Description: Arena interface
 * By:          Nigel Sinclair
 * Github:      https://github.com/sincngraeme
 */

#ifndef _ALLOCATORS_H
#define _ALLOCATORS_H

#ifdef __unix__
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <sys/mman.h>
#else
#error "Not a UNIX!" 
#endif

#define MORTAR_INTERFACES
#include "utility.c"

/*** INTERFACE ***/
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Generic Container type */
typedef struct Arena {
    u8*   mem;
    usize size;
} Arena;

/* Allocations are tracked consecutively by offset */
typedef struct {
    Arena arena;
    usize offset;
} ScratchAllocatorData;

/* Page Allocations are tracked externally by OS */
typedef struct {
    u8*   mem;
    usize size;
} SysAllocatorData;

/* Generic Allocator Interface */
typedef struct Allocator {
    union { 
        ScratchAllocatorData* scratch;
        SysAllocatorData*     sys;
        void* any;
    } data;
    struct AllocatorVTable {
        void* (*alloc)      (struct Allocator* self, size_t size);
        bool  (*realloc)    (struct Allocator* self, void* data, size_t size);
        bool  (*dealloc)    (struct Allocator* self, void* data);
        void  (*reset)      (struct Allocator* self);
        void  (*delete)     (struct Allocator* self);
    } vtable;
    struct Allocator* fallback;
} Allocator;

typedef void* (AllocatorAllocFn)      (Allocator* self, size_t size);
typedef bool  (AllocatorReallocFn)    (Allocator* self, void* data, size_t size);
typedef bool  (AllocatorDeallocFn)    (Allocator* self, void* data);
typedef void  (AllocatorResetFn)      (Allocator* self);
typedef void  (AllocatorDeleteFn)     (Allocator* self);

/* Generic arena functions which will select the right functions based on 
 * the arena type
 */

/* Allocate within the arena */
void* mem_alloc(Allocator* self, usize size);
/* Deallocate within the arena */
bool  mem_dealloc(Allocator* self, void* ptr);
/* Reallocate within the arena */
bool  mem_realloc(Allocator* self, void* mem, usize size);
/* Reset the entire arena */
void  mem_reset(Allocator* self);
/* Resize the entire arena (requires parent allocator) */
void  mem_resize(Allocator* self, usize size);
/* Delete the entire arena */
void  delete_allocator(Allocator* self);

Allocator SysAllocator_init(SysAllocatorData* config);
void ScratchAllocator_init(
    Allocator* allocator, 
    ScratchAllocatorData* opts, 
    Allocator* fallback
);

#endif
