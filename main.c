#include <stdio.h>
#include "dstring.h"
#include "errval.h"

#ifndef GNU
#define GNU
#endif

int log_num = 0;
#define LOG_OUTPUT stderr
#define LOG(message) \
    fprintf(LOG_OUTPUT, message); \
    return log_num++;

DSTRING_INIT(String);

int main() {
    dstring buffer = UNWRAP(String.new("Hello There!"), LOG("Failed to allocate string\n"));
    printf("String: %s\n", UNWRAP(String.str(buffer), LOG("Failed to access string data\n")));

    // Free
    String.del(&buffer);
    return 0;
}
