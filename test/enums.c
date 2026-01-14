#include <stdio.h>
#include "../csl-enums.h"

MKENUM(test_enum, 
    ONE = 1,
    TWO,
    TRHEE
);

int main() {
    printf("test_enum: %s\n", test_enum_pretty);    
}
