#include <stdio.h>
#include <stdlib.h>

#define SMRTPTR_IMPLEMENTATION

#define UNIQUE_PTR_TYPE_LIST \
    UNIQUE_PTR_DERIVE(int, free) \
    UNIQUE_PTR_DERIVE(FILE, close_file)

#define SHARED_PTR_TYPE_LIST \
    SHARED_PTR_DERIVE(int, free)

void close_file(void* void_fp) {
    FILE* fp = void_fp;
    if(fclose(fp) == EOF) {
        fprintf(stderr, "ERROR: Failed to close file\n");
        exit(EOF);
    }
}

#include "../csl-smrtptrs.h"

int main() {
    {
        unique_ptr(int) ptr1 = make_unique_ptr(int, malloc(sizeof(int)), free);
        deref_smrtptr(ptr1) = 1;
        printf("ptr1: %d\n", *ptr1.ptr);
    }
    {
        // Shared ptr
        shared_ptr(int) ptr2 = make_shared_ptr(int, malloc(sizeof(int)), free);
        deref_smrtptr(ptr2) = 10;
        printf("ptr6: %d\n", deref_smrtptr(ptr2));
        {
            shared_ptr(int) ptr3 = clone_shared_ptr(int, ptr2, SHARED_PTR);
            { /* Create a weak_ptr */
                weak_ptr(int) ptr4 = clone_shared_ptr(int, ptr3, WEAK_PTR);
                weak_ptr(int) ptr5 = clone_weak_ptr(int, ptr4, WEAK_PTR);
                { shared_ptr(int) ptr6;
                    if( is_ptr_dead(ptr6 = clone_weak_ptr(int, ptr4, SHARED_PTR)) ) {
                        printf("ptr8: %d\n", deref_smrtptr(ptr6)); /* Reading */
                    }
                }
            }
            printf("ptr7: %d\n", deref_smrtptr(ptr2));
        }
    }
    {
        // File IO
        unique_ptr(FILE) fp = make_unique_ptr(FILE, fopen("test.txt", "a"), close_file);
        if(fp.ptr == NULL) {
            fprintf(stderr, "ERROR: Failed to open file\n");
            exit(-1);
        } else {
            printf("File successfully opened!\n");
            if(fputs("Hello there!\n", fp.ptr) == 0) {
                fprintf(stderr, "ERROR: Failed to write to file\n");
                return -1;
            };
            printf("File successfully written to!\n");
        }
    }
    {
        // File IO
        unique_ptr(FILE) fp = make_unique_ptr(FILE, fopen("test.txt", "r"), close_file);
        char buf[256] = {0};
        if(fp.ptr == NULL) {
            fprintf(stderr, "ERROR: Failed to open file\n");
            exit(-1);
        } else {
            printf("File successfully opened!\n");
            for(int i = 0; fgets(buf, 256, fp.ptr) != NULL; i++) printf("%d | %s", i, buf);
            printf("File successfully read from!\n");
        }
    }
    printf("Hello There!\n");
    return 0;
}
