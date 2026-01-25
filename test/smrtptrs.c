#include <stdio.h>
#include <stdlib.h>
#define SMRTPTR_IMPLEMENTATION

#define INIT_SOLE_PTR
#define INIT_SHARED_PTR
#define ACCESS_PTR_REGISTRY \
    REGISTER_ACCESS_PTR(int, group1)

void close_file(void* void_fp) {
    FILE* fp = void_fp;
    if(fclose(fp) == EOF) {
        fprintf(stderr, "ERROR: Failed to close file\n");
        exit(EOF);
    }
}

#define UNIQUE_PTR_TYPE_LIST \
    UNIQUE_PTR_DERIVE(int, free) \
    UNIQUE_PTR_DERIVE(FILE, close_file)


#define SHARED_PTR_TYPE_LIST \
    SHARED_PTR_DERIVE(int, free)

#include "../csl-smrtptrs.h"

int main() {
    {
        unique_ptr(int) ptr1 = malloc(sizeof(int));
        *ptr1 = 1;
        printf("ptr1: %d\n", *ptr1);
    }
    {
        sole_ptr(int) ptr2 = malloc(sizeof(int));
        *ptr2 = 2;
        printf("ptr2: %d\n", *ptr2);
        {
            sole_ptr(int) ptr3 = ptr2;
            printf("ptr2: %d\n", *ptr2);
            printf("ptr3: %d\n", *ptr3);
        }
        printf("ptr2: %d\n", *ptr2);
    }
    {
        access_ptr(int, group1) ptr4 = malloc(sizeof(int));
        *ptr4 = 4;
        printf("ptr4: %d\n", *ptr4);
        {
            access_ptr(int, group1) ptr5 = ptr4;
            printf("ptr4: %d\n", *ptr4);
            printf("ptr5: %d\n", *ptr5);
        }
        printf("ptr4: %d\n", *ptr4);
    }
    {
        // Shared ptr
        shared_ptr(int) ptr6 = make_shared_ptr(int, malloc(sizeof(int)));
        deref_shared_ptr(ptr6) = 10;
        printf("ptr6: %d\n", deref_shared_ptr(ptr6));
        {
            shared_ptr(int) ptr7 = clone_shared_ptr(ptr6, SHARED_PTR);
            printf("ptr7: %d\n", deref_shared_ptr(ptr6));
        }
    }
    {
        // File IO
        unique_ptr(FILE) fp = fopen("test.txt", "a");
        if(fp == NULL) {
            fprintf(stderr, "ERROR: Failed to open file\n");
            exit(-1);
        } else {
            printf("File successfully opened!\n");
            if(fputs("Hello there!\n", fp) == 0) {
                fprintf(stderr, "ERROR: Failed to write to file\n");
                return -1;
            };
            printf("File successfully written to!\n");
        }
    }
    {
        // File IO
        unique_ptr(FILE) fp = fopen("test.txt", "r");
        char buf[256] = {0};
        if(fp == NULL) {
            fprintf(stderr, "ERROR: Failed to open file\n");
            exit(-1);
        } else {
            printf("File successfully opened!\n");
            for(int i = 0; fgets(buf, 256, fp) != NULL; i++) printf("%d | %s", i, buf);
            printf("File successfully read from!\n");
        }
    }
    printf("Hello There!\n");
    return 0;
}
