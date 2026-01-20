#include <stdio.h>
#define SMRTPTR_IMPLEMENTATION
#include "../csl-smrtptrs.h"


int main() {
    {
        unique_ptr(int) ptr1 = malloc(sizeof(int));
        *ptr1 = 1;
        printf("ptr1: %d\n", *ptr1);
    }
    {
        shared_ptr(int) ptr2 = malloc(3 * sizeof(int));
        *ptr2 = 2;
        printf("ptr2: %d\n", *ptr2);
        {
            shared_ptr(int) ptr3 = ptr2;
            printf("ptr2: %d\n", *ptr2);
            printf("ptr3: %d\n", *ptr3);
        }
        printf("ptr2: %d\n", *ptr2);
    }
    return 0;
}
