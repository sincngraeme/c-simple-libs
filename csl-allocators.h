#include <stdlib.h>
#include "csl-errval.h"

#ifdef OBJECT_ALLOCATOR_TYPES

typedef enum {
#define OBJECT_ALLOCATOR_TYPE(T) ARENA_TYPE_##T,
        OBJECT_ALLOCATOR_TYPES
#undef OBJECT_ALLOCATOR_TYPE
} Object_arena_types;

typedef union {
#define OBJECT_ALLOCATOR_TYPE(T) const T* T##_option;
    OBJECT_ALLOCATOR_TYPES
#undef OBJECT_ALLOCATOR_TYPE
        void* _generic;  // Internal
} Object_arena_options;

/* Generic Arena Type */
typedef struct {
    size_t nobjects;
    size_t object_size;
    size_t* index_stack;
    Object_arena_types type;
    /* Contains all types */
    Object_arena_options u;
} Object_arena;

const size_t object_arena_index_stack_max = 100;

DERIVE_RESULT_DIRECT(Object_arena);

static RESULT(Object_arena) _make_object_arena(size_t nobjects, Object_arena_types type) {
    size_t object_size = 0;
    size_t index_stack_size = 0;
    size_t arena_size = 0;
    size_t allocation_size = 0;

    /* Switch on all derived types to ensure we get the right object size */
    switch(type) {
#define OBJECT_ALLOCATOR_TYPE(T)        \
        case ARENA_TYPE_##T:            \
            object_size = sizeof(T);    \
            break;
        OBJECT_ALLOCATOR_TYPES
#undef OBJECT_ALLOCATOR_TYPE
        default: return ERR(Object_arena, (Object_arena){0});
    }

    /* The index stack size is capped at 100 to prevent 
     * unneccessary large allocations (it will get realloced if
     * necessary) */
    index_stack_size = ( (nobjects < object_arena_index_stack_max) 
                        ? nobjects 
                        : object_arena_index_stack_max );
    arena_size = object_size * nobjects;
    allocation_size = arena_size + index_stack_size;

    /* Allocate space for the objects and the index stack */
    void* pobjects = NULL; 
    if((pobjects = malloc(allocation_size)) == NULL) {
        return ERR(Object_arena, (Object_arena){0});
    }
    Object_arena arena = {
        .nobjects = nobjects,
        .object_size = object_size,
        /* Index stack will be located at the end of all the objects */
        .index_stack = pobjects + allocation_size,
        .type = type,
        /* here we use the void pointer version */
        .u._generic = pobjects
    };
    return OK(Object_arena, arena);
}

static RESULT(Object_arena_options) _object_arena_alloc(Object_arena* arena) {
    if(arena == NULL) {
        
    }
}



static void free_object_arena(Object_arena* arena) {
    free(arena->u._generic);
    arena->nobjects = 0;
    arena->object_size = 0;
    arena->index_stack = NULL;
}

#define make_object_arena(T, nobjects) _make_object_arena(nobjects, ARENA_TYPE_##T)

#endif
