#include <stdio.h>
#define CSL_STRING_INTERFACE
#include "../csl-string.c"
#include "../csl-errval.h"

int log_num = 0;
#define LOG_OUTPUT stderr
#define LOG(message) \
    fprintf(LOG_OUTPUT, message); \
    dstring_delete(&buffer); \
    return log_num++;

int main() {
    DString buffer = UNWRAP(dstring_new(" There!"), LOG("Failed to allocate string\n"));
    printf("dstring: %.*s\n", STRFMT(buffer.s));
    UNWRAP(dstring_prepend(&buffer, "Hello"), LOG("Failed to prepend to string\n"));
    printf("dstring: %.*s\n", STRFMT(buffer.s));
    UNWRAP(dstring_append(&buffer, " General Kenobi"), LOG("Failed to append to string\n")); 
    printf("New string: %.*s\n", STRFMT(buffer.s));
    // // Remove something
    printf("Prefix Size: %zu\n", UNWRAP(dstring_strippref(&buffer, 6), {
        LOG("Failed to strip prefix from string\n");
    }));
    printf("New string: %.*s\n", STRFMT(buffer.s));
    printf("Suffix Size: %zu\n", UNWRAP(dstring_stripsuff(&buffer, 6), {
        LOG("Failed to strip suffix from string\n");
    }));
    printf("New string: %.*s\n", STRFMT(buffer.s));
    printf("Slice: %.*s\n", STRFMT(UNWRAP(dstring_get_slice(buffer, 1, 5), LOG("Failed to access string data \n"))));
    printf("New string: %.*s\n", STRFMT(buffer.s));
    // Free
    dstring_delete(&buffer);

    return 0;
}
