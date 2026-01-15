#include <stdio.h>
#include "../csl-enums.h"

PPRINT_ENUM(test_enum, 
    ONE = 1,
    TWO,
    TRHEE
);

#define PRETTY_PRINT

int main() {
    printf("test_enum: %s\n", test_enum_pretty);    
    print("integer: %, char: %, long: %", 10, 'c', 10);
    return 0;
}
