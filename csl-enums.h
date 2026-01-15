#include "csl-recursed.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define PPRINT_BUFSIZE 256

#define _ENUM_ENTRY(arg) arg,
#define _STRUCT_ENTRY(arg) "\t" #arg ",\n"
#define PPRINT_ENUM(name, ...)                  \
    enum name {                                 \
        FOR_EACH(_ENUM_ENTRY, __VA_ARGS__)      \
    };                                          \
    const char* name##_pretty = "enum " #name " {\n" FOR_EACH(_STRUCT_ENTRY, __VA_ARGS__) "}\n"

#define PPRINT_ENTRY(arg) _Generic((arg), PRETTY_PRINT_TYPES, default: "%p"),
#define pprint(...) \
    _pprint(FOR_EACH())

static void _pprint(...) {
    va_list args;
    va_start(args);
}

#define _PRETTY_PRINT_GET_USER_TYPES(arg) __PRETTY_PRINT_GET_USER_TYPES arg
#define __PRETTY_PRINT_GET_USER_TYPES(arg) arg
#define PRETTY_PRINT_TYPES \
        (int, "%d"), \
        (long, "%ld"), \
        (long long, "%lld"), \
        (unsigned, "%u"), \
        (unsigned long, "%lu"), \
        (unsigned long long, "%llu"), \
        (float, "%f"), \
        (double, "%f"), \
        (char *, "%s"), \
        (const char *, "%s"), \
        _PRETTY_PRINT_GET_USER_TYPES(PRETTY_PRINT)
