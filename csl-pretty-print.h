#include "csl-recursed.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define PPRINT_BUFSIZE 256

#define PRINTP_FMT_ENTRY(arg) , _Generic((arg), PRETTY_PRINT_TYPES, default: "%p")
#define printp(...)                                                 \
    printf(make_format(                                             \
        PPRINT_BUFSIZE,                                             \
        (char[PPRINT_BUFSIZE]){0}                                   \
        FOR_EACH(PRINTP_FMT_ENTRY, __VA_ARGS__), NULL), __VA_ARGS__)

static const char* make_format(unsigned int bufsize, char buffer[bufsize], const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    strcpy(buffer, fmt);
    const char* arg = NULL;
    /* Construct the format string */
    for(unsigned int i = 0; i < bufsize; i++){
        arg = va_arg(args, const char*);
        if(arg == NULL) break;
        strcat(buffer, arg);
    }
    return buffer; 
}

#define PRETTY_PRINT_TYPES \
        int: "%d", \
        long: "%ld", \
        long long: "%lld", \
        unsigned: "%u", \
        unsigned long: "%lu", \
        unsigned long long: "%llu", \
        short: "%hi", \
        unsigned short: "%hu", \
        float: "%f", \
        double: "%f", \
        char : "%c", \
        char *: "%s", \
        const char *: "%s"
