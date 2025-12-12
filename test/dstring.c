#include <stdio.h>
#include "../dstring.h"
#include "../errval.h"

int log_num = 0;
#define LOG_OUTPUT stderr
#define LOG(message) \
    fprintf(LOG_OUTPUT, message); \
    String.del(&buffer); \
    return log_num++;

DSTRING_INIT(String);

int main() {
    dstring buffer = UNWRAP(String.new("Hello There!"), LOG("Failed to allocate string\n"));
    printf("String: %s\n", UNWRAP(String.str(buffer), LOG("Failed to access string data\n")));
    // Add something
    UNWRAP(String.append(&buffer, " General Kenobi"), LOG("Failed to append to string\n")); 
    printf("New string: %s\n", UNWRAP(String.str(buffer), LOG("Failed to access string data\n")));
    // Remove something
    printf("Prefix: %s\n", UNWRAP(String.strip_prefix(&buffer, 5), {
        LOG("Failed to strip prefix from string\n");
    }));
    printf("Suffix: %s\n", UNWRAP(String.strip_suffix(&buffer, 6), {
        LOG("Failed to strip suffix from string\n");
    }));
    printf("Slice: %s\n", UNWRAP(String.slice(buffer, 2, 5), LOG("Failed to access string data \n")));
    printf("New string: %s\n", UNWRAP(String.str(buffer), LOG("Failed to access string data\n")));
    // Free

    String.del(&buffer);
    return 0;
}
