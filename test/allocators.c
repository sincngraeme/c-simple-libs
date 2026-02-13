#include <stdio.h>

typedef struct {
    int x;
    int y;
} point;

#define OBJECT_ALLOCATOR_TYPES \
    OBJECT_ALLOCATOR_TYPE(point)

#include "../csl-allocators.h"

#define defer(dealloc) __attribute__((cleanup(dealloc)))

int main() {
    defer(free_object_arena) Object_arena arena = UNWRAP(make_object_arena(point, 10), {
        fprintf(stderr, "ERROR: Failed to allocate arena.\n");
        return -1; 
    });
    printf("Hello from "__FILE__);
    // free_object_arena(&arena);
    return 0;
}
