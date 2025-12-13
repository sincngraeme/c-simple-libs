/*******************************************************************************
* Name:             csl-recursed.h                                             *
* Description:      Cursed macro magic to give rust style match expressions    *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
* Usage:            Call match as either the result of an expression or as a   *
*                   statment, supplying they value to check and key-value      *
*                   pairs:                                                     *
*                                                                              *
*                       char* result = MATCH(x, "Default"                      *
*                           (1, "First"),                                      *
*                           (2, "Second"),                                     *
*                           (3, "Third"),                                      *
*                           (4, "Fourth"),                                     *
*                       );                                                     *
*                                                                              *
*                   Note that the default case is required and must be the     *
*                   second argument. (I know it's weird but its a limmitation  *
*                   in variadic macros don't @ me)                             *
*******************************************************************************/

#include "csl-recursed.h"

#define _MATCH_ARMS(lhs, rhs) lhs ? rhs:
// #define _MATCH_ARMS(lhs, ...) lhs, __VA_OPT__(,)
#define MATCH_ARMS() _MATCH_ARMS
/* Very similar to FOR_EACH from csl-recursed.h */

/* @brief: This is the top level one that will acually be called, note that defer passes
 *          through the full "stack" of nested macros 
 * @param: x - value to compare to
 * @param: ... - list of key-value pairs: keys are compared with x, and the matching key's value
 *              becomes the result of the expression, and any function there will be evaluated 
 */
#define MATCH(x, last, ...) (__VA_OPT__(DEFER(_MATCH(x, __VA_ARGS__)))last)
/* This macro manages the expansion of the arguments to handle the last one */
#define CATCH_LAST_MATCH(...) __VA_ARGS__ 
// #define CATCH_LAST_MATCH(...) _CATCH_LAST_MATCH(last, __VA_ARGS__) __VA_OPT()
/* This macro removes the first argument of the VA list repeatedly and calls
 * the macro on it */
#define _MATCH(x, first, ...)                    \
    x == MATCH_ARMS PARENS first \
    __VA_OPT__(_MATCH_ PARENS (x, __VA_ARGS__ )) 
    // CATCH_LAST_MATCH(_MATCH_ PARENS (x, __VA_ARGS__)) __VA_OPT_
/* Another necessary abstraction to prevent the prepocessor from doing the safe thing */
#define _MATCH_() _MATCH
