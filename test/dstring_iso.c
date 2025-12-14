#include <stdio.h>
#include "../errval.h"

int log_num = 0;
#define LOG_OUTPUT stderr
#define LOG(message) \
    fprintf(LOG_OUTPUT, message); \
    return log_num++;

DERIVE_RESULT_DIRECT(int);

RESULT(int) always_fails() {
    return ERR(int, 0);
}
RESULT(int) always_succeeds() {
    return OK(int, 1);
}

int main() {
    TRY {
        printf("Error: %d\n", UNWRAP(always_fails()));
    } CATCH(fprintf(stderr, "Failed\n"););
    TRY {
        printf("Success: %d\n", UNWRAP(always_succeeds()));
    } CATCH(LOG("Failed\n"););
    return 0;
}
