/*******************************************************************************
* Name:             csl-argparse.h                                             *
* Description:      Cursed macro magic to recursively parse VA lists           *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
*******************************************************************************/

#ifndef _RECURSED_H
#define _RECURSED_H

// For some reason this needs expansion
#define PARENS ()

// Defines the recursion depth of the macro expansion (256)
// You don't need more than that
#define DEFER(...)      DEFER256(DEFER256(__VA_ARGS__))
#define DEFER256(...)   DEFER16(DEFER16(__VA_ARGS__))
#define DEFER16(...)    DEFER4(DEFER4(__VA_ARGS__))
#define DEFER4(...)     DEFER2(DEFER2(__VA_ARGS__))
#define DEFER2(...)     DEFER1(DEFER1(__VA_ARGS__))
#define DEFER1(...)     __VA_ARGS__

/* This is the top level one that will acually be called, note that defer passes
 * through the full "stack" of nested macros */
#define FOR_EACH(macro, ...) __VA_OPT__(DEFER(_FOR_EACH(macro, __VA_ARGS__)))
/* This macro removes the first argument of the VA list repeatedly and calls
 * the macro on it */
#define _FOR_EACH(macro, first, ...)                    \
    macro(first)                                        \
    __VA_OPT__(_FOR_EACH_ PARENS (macro, __VA_ARGS__))
/* Another necessary abstraction to prevent the prepocessor from doing the safe thing */
#define _FOR_EACH_() _FOR_EACH

/* Related Utility macros */
#define COUNT_ARGS(T, ...) (sizeof((T[]){__VA_ARGS__})/sizeof(T))

#endif
