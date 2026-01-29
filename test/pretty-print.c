#include "../csl-pretty-print.h"

int main() {
    int x = 10;
    char c = 'e';
    printp("string", 10, "c", 10, "\n");
    printp(35);
    printp("Hello ", "There!\n", "General Kenobi\n", "Integer: ", x, "\nCharacter: ", c, (char)'\n');
    return 0;
}
