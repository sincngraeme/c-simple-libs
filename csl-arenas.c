/*** INTERFACE ***/
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>

/* We are relying on 8-bit bytes */
static_assert(CHAR_BIT == 8, "# of bits in byte must be 8 (architecture not supported)\n");

enum AllocationStrategy {
    SCRATCH_ALLOC,
    BLOCK_ALLOC,
    REVERSE_BLOCK_ALLOC,
};

/* Allocations are tracked consecutively by offset */
typedef struct {
    uint8_t* data;
    size_t offset;
    size_t size;
} ScratchArena;

/* Allocations are tracked by a byte next to each block,
 * arena is scanned from start for free space, or from offset
 * to start if REVERSE_BLOCK_ALLOC */
typedef struct {
    uint8_t* data_0init;
    size_t block_size;
    size_t arena_size;
    size_t offset;
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
static void* ScratchArena_alloc(ScratchArena* arena, size_t size) {
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
static void* BlockArena_alloc(BlockArena* arena, size_t size, bool forewards) {
    if( 
        arena == NULL               ||
        arena->data_0init == NULL   ||
        arena->block_size == 0      ||
        arena->arena_size == 0      ||
        size == 0
    ) return NULL;
    // We need an extra byte to indicate if the block has been allocated
    size_t nblocks = arena->arena_size / (arena->block_size + 1);
    if(nblocks == 0) return NULL;
    
    uint8_t* pfree = arena->data_0init;
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
        arena == NULL                                           ||
        ptr == NULL                                             ||
        arena->data_0init == NULL                               ||
        arena->block_size == 0                                  ||
        arena->arena_size == 0                                  ||
        (uint8_t*)ptr > arena->data_0init + arena->arena_size   ||
        (uint8_t*)ptr < arena->data_0init
    ) return false;
    uint8_t* alloc_flag = (uint8_t*)ptr - 1;
    *alloc_flag = 0; 
    return true;
}

static void BlockArena_delete(BlockArena* arena) {
    free(arena->data_0init);
    arena->data_0init = NULL;
}

/*** PUBLIC FUNCTIONS ***/
void* arena_alloc(Arena* arena, size_t size) {
    switch(arena->strategy) {
        case SCRATCH_ALLOC: return ScratchArena_alloc(&arena->scratch, size);
        case BLOCK_ALLOC: return BlockArena_alloc(&arena->block, size, true);
        case REVERSE_BLOCK_ALLOC: return BlockArena_alloc(&arena->block, size, false);
    }
    return NULL;
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
    return false;
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

/* If included as a header only expose the declarations */
#else

/* Generic arena functions which will select the right functions based on 
 * the arena type
 */
void* arena_alloc(Arena* arena, size_t size);
bool  arena_release_ptr(Arena* arena, void* ptr);
void  arena_delete(Arena* arena);

#endif
