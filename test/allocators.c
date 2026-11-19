#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int x;
    int y;
} point;

#define CSL_ARENAS_IMPLEMENTATION
#define defer(dealloc) __attribute__((cleanup(dealloc)))

#include "../csl-allocators.h"
#include "../csl-tests.h"

SysAllocatorData test_sysdata = {0};
Allocator test_sys = {0};
void* test_page = NULL;

ScratchAllocatorData test_scratchdata = {0};
Allocator test_scratch = {0};
char* test_string = NULL;

/*** SysAllocator ***/
static CSL_TEST(SysAllocator_init) {
    test_sys = SysAllocator_init(&test_sysdata);
    bool result = true;
    result = result && CSL_TEST_ASSERT(test_sys.data.sys == &test_sysdata, "System allocator improperly initialized");
    result = result && CSL_TEST_ASSERT(test_sys.data.sys != NULL, "System allocator improperly initialized");
    result = result && CSL_TEST_ASSERT(test_sys.parent == NULL, "Fallback Allocator not allowed for SysAllocator");
    result = result && CSL_TEST_ASSERT(test_sys.vtable.alloc != NULL, "Allocation function not set");
    result = result && CSL_TEST_ASSERT(test_sys.vtable.dealloc != NULL, "Deallocation function not set");
    result = result && CSL_TEST_ASSERT(test_sys.vtable.realloc != NULL, "Reallocation function not set");
    result = result && CSL_TEST_ASSERT(test_sys.vtable.destroy == NULL, "Cleanup function set (not implemented for SysAlloc)");
    result = result && CSL_TEST_ASSERT(test_sys.vtable.reset == NULL, "Reset function set (not implemented for SysAlloc)");
    return result;
}

static CSL_TEST(SysAllocator_alloc) {
    bool result = true;
    test_page = mem_alloc(&test_sys, 1024);
    result = result && CSL_TEST_ASSERT(test_page != NULL, "System allocator failed");
    return result;
}

static CSL_TEST(SysAllocator_dealloc) {
    bool result = true;
    result = result && CSL_TEST_ASSERT(mem_dealloc(&test_sys, test_page), "System deallocator failed");
    return result;
}

static CSL_TEST(SysAllocator_realloc) {
    bool result = true;
    result = result && CSL_TEST_ASSERT(mem_realloc(&test_sys, test_page, 256), "System reallocator failed");
    return result;
}
/*** ScratchAllocator ***/ 
static CSL_TEST(ScratchAllocator_init) {
    test_scratchdata.arena = (Arena){ 
        .mem = test_page,
        .size = 256
    };
    ScratchAllocator_init(&test_scratch, &test_scratchdata, &test_sys);
    bool result = true;
    result = result && CSL_TEST_ASSERT(test_scratch.data.scratch == &test_scratchdata, "Scratch allocator improperly initialized");
    result = result && CSL_TEST_ASSERT(test_scratch.data.scratch != NULL, "Scratch allocator improperly initialized");
    result = result && CSL_TEST_ASSERT(test_scratch.vtable.alloc != NULL, "Allocation function not set");
    result = result && CSL_TEST_ASSERT(test_scratch.vtable.dealloc == NULL, "Deallocation function set (Not implemented for ScratchAllocator)");
    result = result && CSL_TEST_ASSERT(test_scratch.vtable.realloc == NULL, "Reallocation function set (Not implemented for ScratchAllocator)");
    result = result && CSL_TEST_ASSERT(test_scratch.vtable.destroy != NULL, "Deletion function not set");
    result = result && CSL_TEST_ASSERT(test_scratch.vtable.reset != NULL, "Reset function not set");
    return result;
}

static CSL_TEST(ScratchAllocator_alloc) {
    bool result = true;
    const char string1[] = "Hello There!\n";
    const char string2[] = "General Kenobi.\n";
    usize string1_len = strlen(string1);
    usize string2_len = strlen(string2);
    char* sptr = NULL;

    test_string = mem_alloc(&test_scratch, string1_len);
    result = result && CSL_TEST_ASSERT(test_string != NULL, "Scratch allocator failed");
    memcpy(test_string, string1, string1_len);

    sptr = mem_alloc(&test_scratch, string2_len + 1);
    result = result && CSL_TEST_ASSERT(sptr != NULL, "Scratch allocator failed");
    memcpy(sptr, string2, string2_len + 1);

    char buf[ string1_len + string2_len + 1 ];
    strcpy(buf, string1);
    strcat(buf, string2);
    printf("test_string: %s\n", test_string);
    printf("buf: %s\n", buf);
    CSL_TEST_ASSERT(memcmp(buf, test_string, sizeof(buf)) == 0, "String data corrupted");
    return result;
}

static CSL_TEST(allocators) {
    if(!CSL_RUN_TEST(SysAllocator_init) 
    || !CSL_RUN_TEST(SysAllocator_alloc) 
    || !CSL_RUN_TEST(SysAllocator_realloc) 
    ) goto fail;

    if(!CSL_TEST_ASSERT(test_sys.data.sys != NULL, "Allocator data NULL")) goto fail; 
    if(!CSL_TEST_ASSERT(test_sys.data.sys->size == 256, "Page size incorrect")) goto fail; 

    if(!CSL_RUN_TEST(ScratchAllocator_init) 
    || !CSL_RUN_TEST(ScratchAllocator_alloc) 
    ) goto fail;

    destroy_allocator(&test_scratch);
    if(!CSL_RUN_TEST(SysAllocator_dealloc)) goto fail;

    return true;
fail:
    return false;
}

int main() {
    CSL_TEST_INIT;
    CSL_RUN_TEST(allocators);
}
