#include <stdlib.h>
#include "csl-errval.h"

#ifdef CSL_ALLOCATORS_IMPLEMENTATION

typedef struct {
    size_t nobjects;
    size_t object_size;
    size_t* index_stack;
    size_t index_stack_size;
    void* objects;
} Arena;

typedef void* Arena_object; // Unfortunately necessary for the type mangling to work

/* For error wrapping */
DERIVE_RESULT_DIRECT(Arena);
DERIVE_RESULT_DIRECT(Arena_object);

/* Initializes an arena for <nobjects> number of objects of fixed <object_size> size */
static RESULT(Arena) init_arena(size_t nobjects, size_t object_size) {
    size_t index_stack_size = sizeof(size_t) * ( nobjects < 100 ? nobjects : 100 ); 
    size_t arena_size = nobjects * object_size; 
    /* We allocate space for both the arena and the index stack */
    size_t allocation_size = arena_size + index_stack_size; 

    void* pobjects = IFNULL(malloc(allocation_size), {
        return ERR( Arena, (Arena){0} );
    });

    Arena arena = {
        .nobjects = nobjects,
        .object_size = object_size,
        .objects = pobjects,
        .index_stack = pobjects + arena_size,
        .index_stack_size = index_stack_size,
    };
    *arena.index_stack = 0; // Starting at the block of memory
    return OK( Arena, arena );
}

/* Returns a pointer to an object in the allocated arena space */
__attribute__((unused)) // TEMP: just for debugging
static RESULT(Arena_object) arena_alloc(Arena* arena) {
    if(
        arena->nobjects         == 0    ||
        arena->object_size      == 0    ||
        arena->objects          == NULL ||
        arena->index_stack      == NULL ||
        arena->index_stack_size == 0    ||
        // Index stack pointer must be after the end of the objects
        (void*)arena->index_stack < arena->objects + (arena->nobjects * arena->object_size) ||
        // Index stack value cannot be greater than the number of allocated blocks
        *arena->index_stack > arena->nobjects
    ) return ERR(Arena_object, NULL); 
    void* pobject = NULL;
    size_t* stack_bottom = arena->objects + (arena->nobjects * arena->object_size);
    /* Check if the stack is empty (we are appending) */
    if(*arena->index_stack < *stack_bottom) {
        /* In this case we are backfilling into the space that was allocated, 
         * then released. Therefore, we must get the value at the top of the 
         * index stack and decrement the index stack pointer */
        pobject = arena->objects + *(arena->index_stack--);
        return OK(Arena_object, pobject);
    } else {
        /* value pointed to by index stack is the index of the next 
         * block to be allocated, in this case the index stack is empty 
         * so we can just use the top value and increment the index */
        pobject = arena->objects + arena->object_size * (*arena->index_stack)++;
        return OK(Arena_object, pobject);
    }
}

/* Hands out a block of memory that was allocated with arena_init */
[[nodiscard]] __attribute__((unused)) static int arena_release(Arena_object object, Arena* arena) {
    if(
        object                  == NULL ||
        arena->nobjects         == 0    ||
        arena->object_size      == 0    ||
        arena->objects          == NULL ||
        arena->index_stack      == NULL ||
        arena->index_stack_size == 0    ||
        // Index stack pointer must be after the end of the objects
        (void*)arena->index_stack < arena->objects + (arena->nobjects * arena->object_size) ||
        // Index stack pointer must be before the end of the allocated space
        (void*)arena->index_stack > ( arena->objects + (arena->nobjects * arena->object_size) + arena->index_stack_size * sizeof(size_t) ) ||
        // Index stack value cannot be greater than the number of allocated blocks
        *arena->index_stack > arena->nobjects ||
        // Object must be in bounds of this arena's object space
        object < arena->objects ||
        object > arena->objects + (arena->nobjects * arena->object_size)
    ) return 1; 
    /* TODO: Handle index stack overflow */
    size_t* stack_bottom = arena->objects + (arena->nobjects * arena->object_size);
    /* Find the index of the given object */
    size_t obj_index = (size_t)( ( object - arena->objects ) / arena->object_size );
    if(obj_index < *stack_bottom) {
        /* The object that is being released is not at the end so we must push
         * its index onto the stack so the memory can be used again */
        *(++arena->index_stack) = obj_index;
    } else {
        /* The index of the object is at the end of the objects, so we just decrement the 
         * value at the bottom of the stack (corresponds to the end of the objects) */
        (*stack_bottom)--;
    }
    return 0;
}

static void free_arena(Arena* arena) {
    free(arena->objects);
    arena->nobjects = 0;
    arena->object_size = 0;
    arena->index_stack = NULL;
}

#endif
