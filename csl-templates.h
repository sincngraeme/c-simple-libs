#include "csl-recursed.h"

/* Single templated type */
#define GENERATE(...) FOR_EACH(TEMPLATE, __VA_ARGS__)

/* Multiple templated types*/
#define _TEMPLATE_() TEMPLATE
#define _TEMPLATE(arg) _TEMPLATE_ PARENS arg
#define MULTIGEN(...) FOR_EACH(_TEMPLATE, __VA_ARGS__)

/* Single iteration will build the correct function name for the type*/
/* Type Inference */
#define INFER(name, T, ...)         \
    _Generic((T),                   \
        TYPEGEN(name, __VA_ARGS__)  \
    )(T)                            

/* This is the top level one that will acually be called, note that defer passes
 * through the full "stack" of nested macros */
#define TYPEGEN(name, ...) __VA_OPT__(DEFER(_TYPEGEN(name, __VA_ARGS__)))
/* This macro removes the first argument of the VA list repeatedly and calls
 * the macro on it */
#define _TYPEGEN(name, first, ...)                          \
    first: name##_##first __VA_OPT__(,)                     \
    __VA_OPT__(_TYPEGEN_ PARENS (name, __VA_ARGS__))
/* Another necessary abstraction to prevent the prepocessor from doing the safe thing */
#define _TYPEGEN_() _TYPEGEN
