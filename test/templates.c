#include "../csl-templates.h"
#include <stdio.h>

#define function(T) INFER(function, T, long, int, float, double, char)
#define TEMPLATE(T)                 \
T function_##T(T my_variable) {     \
   my_variable += 1;                \
   return my_variable;              \
}
GENERATE(long, int , float, double, char);
#undef TEMPLATE

int main() {
    printf("Integer: %d\n", function(10));
    printf("Float: %f\n", function(10.0));
    printf("Character: %c\n", function('A'));
    return 0;
}

