#include <stdio.h>

#define SMRTPTR_TYPE_LIST \
    SMRTPTR_DERIVE(int);
#include "../csl-smrtptrs.h"

int main() {
    const int* ptr1 = malloc(sizeof(int));
    return 0;
}
