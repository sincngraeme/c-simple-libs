#include <stdio.h>
#define SMRTPTR_IMPLEMENTATION

#define INIT_SOLE_PTR
#define ACCESS_PTR_REGISTRY \
    REGISTER_ACCESS_PTR(int, group1)

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
    return 0;
}
