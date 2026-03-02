#include <assert.h>
#include <stdio.h>

typedef struct {
    int x;
    int y;
} point;

#define CSL_ARENAS_IMPLEMENTATION
#include "../csl-allocators.h"

#define defer(dealloc) __attribute__((cleanup(dealloc)))

int main() {
    defer(free_ObjectArena) ObjectArena arena = UNWRAP(init_ObjectArena(10, sizeof(point)), {
        fprintf(stderr, "ERROR: Failed to allocate arena.\n");
        return -1; 
    });
    point* p1 = IFNULL( ObjectArena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
    p1->x = 10;
    p1->y = 15;
    printf("Point p1: x = %d, y = %d\n", p1->x, p1->y);

    point* p2 = IFNULL( ObjectArena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
   
    /* This should have been appended since nothing has been released yet */
    assert(p2 == p1 + 1);

    p2->x = 20;
    p2->y = 25;
    printf("Point p2: x = %d, y = %d\n", p2->x, p2->y);

    /* Release p1 to see if we can back allocate */
    ObjectArena_release(&arena, p1);

    point* p3 = IFNULL( ObjectArena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
    /* This allocation should be in the same place as p1 was */
    assert(p3 == p1);
    p3->x = 20;
    p3->y = 25;
    printf("Point p3: x = %d, y = %d\n", p3->x, p3->y);

    // free_object_arena(&arena);
    return 0;
}
