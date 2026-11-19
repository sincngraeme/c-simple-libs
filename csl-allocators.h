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

/*** INTERFACE ***/
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;


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

/* Fixed block sized Allocations are tracked by offsets stored in free blocks */
typedef struct {
    Arena arena;
    usize first_free;
} BlockAllocatorData;

/* Generic Allocator type 
 * (To signal that an allocator data struct must be passed)*/
typedef void AllocatorData;

/* Generic Allocator Interface */
typedef struct Allocator {
    union { 
        ScratchAllocatorData* scratch;
        SysAllocatorData*     sys;
        void* any;
    } data;
    struct AllocatorVTable {
        void* (*alloc)      (AllocatorData* self, size_t size);
        bool  (*realloc)    (AllocatorData* self, void* data, size_t size);
        bool  (*dealloc)    (AllocatorData* self, void* data);
        void  (*reset)      (AllocatorData* self);
        void  (*destroy)    (struct Allocator* self);
    } vtable;
    struct Allocator* parent;
} Allocator;

typedef void* (AllocatorAllocFn)    (AllocatorData* self, size_t size);
typedef bool  (AllocatorReallocFn)  (AllocatorData* self, void* data, size_t size);
typedef bool  (AllocatorDeallocFn)  (AllocatorData* self, void* data);
typedef void  (AllocatorResetFn)    (AllocatorData* self);
typedef void  (AllocatorDestroyFn)  (Allocator* self);

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
void  destroy_allocator(Allocator* self);

Allocator SysAllocator_init(SysAllocatorData* config);
void ScratchAllocator_init(
    Allocator* allocator, 
    ScratchAllocatorData* opts, 
    Allocator* parent
);

#endif
