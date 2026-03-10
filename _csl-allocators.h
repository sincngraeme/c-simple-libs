#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include "csl-errval.h"

#if defined(CSL_ARENAS_IMPLEMENTATION)

typedef struct {
    size_t nobjects;
    size_t object_size;
    size_t* index_stack;
    size_t index_stack_size;
    /* uint8_t ensures pointer arithmetic is in bytes */
    uint8_t* objects;           
} ObjectArena;

/* We are relying on 8-bit bytes */
static_assert(CHAR_BIT == 8, "# of bits in byte must be 8 (architecture not supported)\n");

/* For error wrapping */
DERIVE_WRESULT(ObjectArena);

/* Initializes an arena for <nobjects> number of objects of fixed <object_size> size */
static WRESULT(ObjectArena) init_ObjectArena(size_t nobjects, size_t object_size) {
    size_t index_stack_size = ( nobjects < 100 ? nobjects : 100 ) * sizeof(size_t); 
    size_t arena_size = nobjects * object_size; 
    /* We allocate space for both the arena and the index stack */
    size_t allocation_size = arena_size + index_stack_size; 

    void* pobjects = malloc(allocation_size);
    if(pobjects == NULL){
        return WRESULT_ERR( ObjectArena, (ObjectArena){0} );
    };

    ObjectArena arena = {
        .nobjects = nobjects,
        .object_size = object_size,
        .objects = pobjects,
        .index_stack = pobjects + arena_size,
        .index_stack_size = index_stack_size,
    };
    *arena.index_stack = 0; // Starting at the block of memory
    return WRESULT_OK( ObjectArena, arena );
}

/* Returns a pointer to an object in the allocated arena space */
static void* ObjectArena_alloc(ObjectArena* arena) {
    if(
        arena->nobjects         == 0    ||
        arena->object_size      == 0    ||
        arena->objects          == NULL ||
        arena->index_stack      == NULL ||
        arena->index_stack_size == 0    ||
        // Index stack pointer must be after the end of the objects
        arena->index_stack > (size_t*)arena->objects + (arena->nobjects * arena->object_size) ||
        // Index stack value cannot be greater than the number of allocated blocks
        *arena->index_stack > arena->nobjects
    ) return NULL; 
    void* pobject = NULL;
    /* Check if the stack is empty, if it is then we must use the top value on the stack
     * for the next allocation.
     * Otherwise, we just use index stored at the bottom of the stack */
    if((uint8_t*)arena->index_stack == (uint8_t*)arena->objects + (arena->nobjects * arena->object_size)) {
        /* value pointed to by index stack is the index of the next 
         * block to be allocated, in this case the index stack is empty 
         * so we can just use the top value and increment the index */
        pobject = arena->objects + arena->object_size * (*arena->index_stack)++;
    } else {
        /* In this case we are backfilling into the space that was allocated, 
         * then released. Therefore, we must get the value at the top of the 
         * index stack and decrement the index stack pointer */
        pobject = arena->objects + *(arena->index_stack--) * arena->object_size ;
    }
    return pobject;
}

__attribute__((unused))
static bool ObjectArena_release(ObjectArena* arena, void* object) {
    if(
        arena == NULL               ||
        object == NULL              ||
        arena->nobjects == 0        ||
        arena->object_size == 0     ||
        arena->objects == NULL      ||
        arena->index_stack == NULL  ||
        object < (void*)arena->objects
    ) return false;
    /* Index stack is located after the objects */
    void* index_stack_bottom = (uint8_t*)arena->objects + (arena->nobjects * arena->object_size);
    if(object > index_stack_bottom) return false;

    size_t object_index = (size_t)((uint8_t*)object - arena->objects) / arena->object_size;
    /* Value stored at the bottom of the stack must be the index of the last object since we only
     * push to the stack when we release out of order */
    size_t index_of_last = *(size_t*)index_stack_bottom; 
    if(object_index < index_of_last) {
        /* Push to the stack since we are releasing out of order */
        *(++arena->index_stack) = object_index;
    } else {
        /* we do not need to push to the stack since the object is being appended */
        *arena->index_stack = object_index;
    }
    return true; 
}

static void free_ObjectArena(ObjectArena* arena) {
    free(arena->objects);
    arena->nobjects = 0;
    arena->object_size = 0;
    arena->index_stack = NULL;
}

#endif
