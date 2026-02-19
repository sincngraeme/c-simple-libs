#include <assert.h>
#include <stdio.h>

typedef struct {
    int x;
    int y;
} point;

#define CSL_ALLOCATORS_IMPLEMENTATION

#include "../csl-allocators.h"

#define defer(fn) __attribute__((cleanup(fn)))

int main() {
    defer(free_arena) Arena arena = UNWRAP(init_arena(10, sizeof(point)), {
        fprintf(stderr, "ERROR: Failed to allocate arena.\n");
        return -1; 
    });
    point* p1 = UNWRAP( arena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
    p1->x = 10;
    p1->y = 15;
    printf("Point p1 (%p): x = %d, y = %d\n", p1,  p1->x, p1->y);

    point* p2 = UNWRAP( arena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
    p2->x = 20;
    p2->y = 25;
    printf("Point p1 (%p): x = %d, y = %d\n", p1,  p1->x, p1->y);
    printf("Point p2 (%p): x = %d, y = %d\n", p2,  p2->x, p2->y);
    
    if(arena_release(p1, &arena)) {
        fprintf(stderr, "ERROR: Failed to release object in arena.\n");
        return -1;
    };
    point* p3 = UNWRAP( arena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
    assert(p1 == p3);
    p3->x = 30;
    p3->y = 35;
    printf("Point p1 (%p): x = %d, y = %d\n", p1,  p1->x, p1->y);
    printf("Point p2 (%p): x = %d, y = %d\n", p2,  p2->x, p2->y);
    printf("Point p3 (%p): x = %d, y = %d\n", p3, p3->x, p3->y);
    
    if(arena_release(p2, &arena)) {
        fprintf(stderr, "ERROR: Failed to release object in arena.\n");
        return -1;
    };

    point* p4 = UNWRAP( arena_alloc(&arena), {
        fprintf(stderr, "ERROR: Failed to allocate object from arena memory.\n");
        return -1;
    });
    assert(p4 == p2);
    p4->x = 40;
    p4->y = 45;
    printf("Point p1 (%p): x = %d, y = %d\n", p1,  p1->x, p1->y);
    printf("Point p2 (%p): x = %d, y = %d\n", p2,  p2->x, p2->y);
    printf("Point p3 (%p): x = %d, y = %d\n", p3, p3->x, p3->y);
    printf("Point p4 (%p): x = %d, y = %d\n", p4, p4->x, p4->y);

    return 0;
}
