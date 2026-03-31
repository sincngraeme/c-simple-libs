/*** INTERFACE ***/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t  i32;
typedef int64_t  i64;

#define defer(fn) __attribute__(( cleanup(fn) ))
#define ifnull(maybenull, handler) ({ \
        typeof(maybenull) result = maybenull; \
        if(result == NULL) { \
            handler; \
        } \
        result; \
    })

enum AllocationStrategy {
    SCRATCH_ALLOC,
    BLOCK_ALLOC,
    REVERSE_BLOCK_ALLOC,
};

/* Allocations are tracked consecutively by offset */
typedef struct {
    u8* data;
    u64 offset;
    u64 size;
} ScratchArena;

/* Allocations are tracked by a byte next to each block,
 * arena is scanned from start for free space, or from offset
 * to start if REVERSE_BLOCK_ALLOC */
typedef struct {
    u8* data_0init;
    u64 block_size;
    u64 arena_size;
    u64 offset;
} BlockArena;

typedef struct {
     enum AllocationStrategy strategy;
     union {
         ScratchArena scratch;
         BlockArena block;
     };
} Arena;

#if !defined(ARENA_HEADER) || defined(ARENA_IMPLEMENTATION)
/*** PRIVATE FUNCTIONS ***/

/* Scratch arenas allocate a given number of bytes each time, each pointer 
 * having the same lifetime as the arena. The only way to deallocate is to 
 * reset the arena by setting the offset to 0;
 */
static void* ScratchArena_alloc(ScratchArena* arena, u64 size) {
    if( 
        arena == NULL       ||
        arena->data == NULL ||
        arena->size == 0    ||
        size == 0
    ) return NULL;
    arena->offset += size;
    return (void*)(arena->data + arena->offset);
}

static void ScratchArena_delete(ScratchArena* arena) {
    free(arena->data);
    arena->data = NULL;
    arena->offset = 0;
    arena->size = 0;
}

/* Block arenas are initialized with a block size which they will always
 * allocate in multiples of. Allocation method is first free (from start). 
 */
static void* BlockArena_alloc(BlockArena* arena, u64 size, bool forewards) {
    if( 
        arena == NULL               ||
        arena->data_0init == NULL   ||
        arena->block_size == 0      ||
        arena->arena_size == 0      ||
        size == 0
    ) return NULL;
    // We need an extra byte to indicate if the block has been allocated
    u64 nblocks = arena->arena_size / (arena->block_size + 1);
    if(nblocks == 0) return NULL;
    
    u8* pfree = arena->data_0init;
    if(forewards) {
        /* loop, incrementing pointer by block size plus one byte for the allocation 
         * indicator until pfree == 0 (unallocated block), or the end of the arena is reached */
        while( pfree + arena->block_size + 1 < arena->data_0init + arena->arena_size && 
               *(pfree += arena->block_size + 1));
    } else {
        pfree += arena->offset;
        /* loop, decrementing pointer by block size plus one byte for the allocation 
         * indicator until pfree == 0 (unallocated block), or the start of the arena is reached */
        do {
            /* Check if we have reached the bottom */
            if(pfree - arena->block_size - 1 < arena->data_0init) {
                // Set the pointer back to the end of the allocated space
                pfree += arena->offset;                     
                arena->offset += arena->block_size + 1;
                break;
            }
        } while(*(pfree -= arena->block_size + 1));
    }
    *pfree = 1; 
    
    return pfree + 1;
}

static bool BlockArena_release_ptr(BlockArena* arena, void* ptr) {
    if( 
        arena == NULL                                       ||
        ptr == NULL                                         ||
        arena->data_0init == NULL                           ||
        arena->block_size == 0                              ||
        arena->arena_size == 0                              ||
        (u8*)ptr > arena->data_0init + arena->arena_size    ||
        (u8*)ptr < arena->data_0init
    ) return false;
    u8* alloc_flag = (u8*)ptr - 1;
    *alloc_flag = 0; 
    return true;
}

static void BlockArena_delete(BlockArena* arena) {
    free(arena->data_0init);
    arena->data_0init = NULL;
}

/*** PUBLIC FUNCTIONS ***/
void* arena_alloc(Arena* arena, u64 size) {
    switch(arena->strategy) {
        case SCRATCH_ALLOC: return ScratchArena_alloc(&arena->scratch, size);
        case BLOCK_ALLOC: return BlockArena_alloc(&arena->block, size, true);
        case REVERSE_BLOCK_ALLOC: return BlockArena_alloc(&arena->block, size, false);
    }
}

bool arena_release_ptr(Arena* arena, void* ptr) {
    switch(arena->strategy) {
        case SCRATCH_ALLOC: {
            return false;
        }
        case BLOCK_ALLOC: {
            BlockArena_release_ptr(&arena->block, ptr);
            return true;
        }
        case REVERSE_BLOCK_ALLOC: {
            BlockArena_release_ptr(&arena->block, ptr);
            return true;
        }
    }
}

void arena_delete(Arena* arena) {
    switch(arena->strategy) {
        case SCRATCH_ALLOC: {
            ScratchArena_delete(&arena->scratch);
            break;
        }
        case REVERSE_BLOCK_ALLOC:
        case BLOCK_ALLOC: {
            BlockArena_delete(&arena->block);
            break;
        }
    }
}

#else

/* Generic arena functions which will select the right functions based on 
 * the arena type
 */
void* arena_alloc(Arena* arena, u64 size);
bool  arena_release_ptr(Arena* arena, void* ptr);
void  arena_delete(Arena* arena);

#endif
