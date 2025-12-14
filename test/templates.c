#include "../csl-templates.h"
#include <stdio.h>

// #define function(T, U) INFER(function, T, long, int, float, double, char)
#define TEMPLATE(T, U)                 \
U function_##T##_to_##U(T my_variable) {     \
   my_variable += 1;                \
   return my_variable;              \
}
MULTIGEN((int , float), (int, double));
#undef TEMPLATE

#define function(T) INFER(function, T, int, double)
#define TEMPLATE(T)                                 \
T function_##T(T my_variable) {        \
   my_variable += 1;                                \
   return my_variable;                              \
}
GENERATE(int, double);
#undef TEMPLATE

int main() {
    char c = 'c';
    printf("Integer: %f\n", function_int_to_float(10));
    printf("Double: %f\n", function_int_to_double(10));
    // printf("Integer: %d\n", function_int(10, c));
    // printf("Double: %f\n", function_double(10, c));
    printf("Int: %f\n", function(10.0));
    return 0;
}

