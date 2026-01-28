#include <stdio.h>
#include "../csl-pretty-print.h"

PPRINT_ENUM(test_enum, 
    ONE = 1,
    TWO,
    TRHEE
);

int main() {
    int x = 10;
    char c = 'e';
    printf("test_enum: %s\n", test_enum_pretty);    
    printp("string", 10, "c", 10, "\n");
    printp(35);
    printp("Hello ", "There!\n", "General Kenobi\n", "Integer: ", x, "\nCharacter: ", c, (char)'\n');
    return 0;
}
