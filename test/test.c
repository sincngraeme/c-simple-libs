#include "../csl-tests.h"

int add(int x, int y) {
    return x + y;
}

void test_add() {
    CSL_TEST_ASSERT(add(10, 7) == 17, "Failed to add 10 and 7");
    CSL_TEST_ASSERT(add(11, 7) == 17, "Failed to add 11 and 7");
}

int main() {
    CSL_TEST_INIT;
    test_add();
}
