#include "csl-recursed.h"

#define _ENUM_ENTRY(arg) arg,
#define _STRUCT_ENTRY(arg) "\t" #arg ",\n"
#define MKENUM(name, ...)                       \
    enum name {                                 \
        FOR_EACH(_ENUM_ENTRY, __VA_ARGS__)      \
    };                                          \
    char* name##_pretty = "enum " #name " {\n" FOR_EACH(_STRUCT_ENTRY, __VA_ARGS__) "}\n"


