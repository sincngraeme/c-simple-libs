#include <stdio.h>
#define ARENA_HEADER
#include "../csl-arenas.c"
#include "../csl-tests.h"

#define defer(fn) __attribute__((cleanup(fn)))
#define ARENA_SIZE 1024

/* Tests */
void test_scratch_arena();
void test_block_arena();
void test_reverse_block_arena();


int main() {
    CSL_TEST_INIT;

    test_scratch_arena();
    test_block_arena();
    test_reverse_block_arena();

    return 0;
}

void test_scratch_arena() {
    defer(arena_delete) Arena arena = {
        .strategy = SCRATCH_ALLOC,
        .scratch = { .data = malloc(ARENA_SIZE), .size = ARENA_SIZE }
    };
    int* data[10] = {0};
    for(int i = 0; i < 10; i++ ) {
            data[i] = arena_alloc(&arena, sizeof(int));
            *data[i] = i;
    }
    for(int i = 0; i < 10; i++ ) {
            CSL_TEST_ASSERT(*data[i] == i, "Data corrupted.");
            printf("data[%d] = %d\n", i, *data[i]);
    }
}

void test_block_arena() {
    defer(arena_delete) Arena arena = {
        .strategy = BLOCK_ALLOC,
        .block = { 
            .data_0init = malloc(ARENA_SIZE), 
            .arena_size = ARENA_SIZE, 
            .block_size = sizeof(int)
        }
    };
    int* data[10] = {0};
    for(int i = 0; i < 10; i++ ) {
            data[i] = arena_alloc(&arena, 1);
            *data[i] = i;
    }
    for(int i = 0; i < 10; i++ ) {
            CSL_TEST_ASSERT(*data[i] == i, "Data corrupted.");
            printf("data[%d] = %d\n", i, *data[i]);
    }
}

void test_reverse_block_arena() {
    defer(arena_delete) Arena arena = {
        .strategy = BLOCK_ALLOC,
        .block = { 
            .data_0init = malloc(ARENA_SIZE), 
            .arena_size = ARENA_SIZE, 
            .block_size = sizeof(int)
        }
    };
    int* data[10] = {0};
    for(int i = 0; i < 10; i++ ) {
        data[i] = arena_alloc(&arena, 1);
        *data[i] = i;
    }
    for(int i = 0; i < 10; i++ ) {
        CSL_TEST_ASSERT(*data[i] == i, "Data corrupted.");
        printf("data[%d] = %d\n", i, *data[i]);
    }
}
