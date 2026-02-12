/*******************************************************************************
* Name:             csl-containerr.h                                           *
* Description:      Provides methods for wrapping return values in "error"     *
*                   values to have save error handling (rust inspired)         *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
* Standard:         C23 (uses optional GNU extensions)                         *
* Usage:            Steps:                                                     *
*                   1. Call DERIVE_RESULT_DIRECT/DERIVE_RESULT_INDIRECT to     *
*                       implement <T>_result_t for your type (indirect for     *
*                       pointers)                                              *
*                   2. Use RESULT(<T>) to "wrap" your return types, return     *
*                       from function with ERR(<T>, val) or OK(<T>, val)       *
*                   3. Wrap RESULT(<T>) function calls in UNWRAP() and specify *
*                       a handler in the second argument. This can be single   *
*                       or multiline.                                          *
*                       Ex:                                                    *
*                                                                              *
*                       printf("String: %s\n", UNWRAP(String.str(my_string), { *
*                           fprintf(stderr, "Failed to access string data\n"); *
*                           String.del(&my_string);                            *
*                           return 1;                                          *
*                       }));                                                   *
*                                                                              *
*                       or                                                     *
*                                                                              *
*                       printf("%s\n", UNWRAP(String.str(my_string),return 1));*
*                       - Note: Fully inline UNWRAP depends on a GNU compiler  *
*                               extension: statement expressions. This feature *
*                               is only available when compiled with gcc or    *
*                               clang.                                         *
*                               The TRY-CATCH version is ISO C23 and should    *
*                               (untested) work with other compilers. Because  *
*                               of this dependency, UNWRAP is not the inline   *
*                               versionby default. to define it, just compile  *
*                               with the standard set to gnu2x.                *
*                                                                              *
*                               gcc main.c -o main -std=gnu2x                  *
*                                                                              *
*                   Steps for EXPECT usage:                                    *
*                   1. Same as before                                          *
*                   2. Wrap function calls in TRY{ } CATCH{ }, and call with   *
*                       EXPECT(func())                                         *
*                   3. Multiple EXPECT calls can exist in one try block and do *
*                       not need to be implemented on the same type. Multiple  *
*                       try-catches cannot be used in the same scope. This is  *
*                       currently a limitation of the implementation. To use   *
*                       multiple try-catches consecutively, they must be       *
*                       wrapped in an outer scope.                             *
*                   4. BE VERY CAREFUL WITH ANY RESOURCES THAT REQUIRE MANUAL  *
*                       CLEANUP. Because longjmp skips stack frames, any       *
*                       resources requiring manual cleanup that are allocated  *
*                       between the TRY and CATCH are at risk of being leaked  *
*                       when an error occurs. ensure that functions which      *
*                       use the result type perform manual cleanup before      *
*                       returning                                              *
*******************************************************************************/

#ifndef __ERRVAL_H
#define __ERRVAL_H


/* 1. Detect that the user is compiling with GNU extensions *and*
   wants to use C23 features (or later).                                 */
#if defined(__GNUC__) || defined(__clang__)      /* GCC or Clang           */
#   if defined(__STRICT_ANSI__)
        /* The compiler was invoked with -std=cXx -pedantic – no extensions. */
#       define GNU_STATEMENT_EXPR 0
#   else
        /* We are in a GNU‑mode compiler (gcc, clang …).  It will understand
        ({ … }) if the language mode is at least C23 or later.          */
#       define GNU_STATEMENT_EXPR 1
#   endif
#else
        /* Non‑GNU compilers – no support for statement expressions.           */
        #  define GNU_STATEMENT_EXPR 0
#endif

#if !GNU_STATEMENT_EXPR
#include <stdio.h>
#include <setjmp.h>
#endif

/* @type:   result
 * @brief:  The result enum is either an err or ok. The <T>_result_t type is 
 *          a struct which contains the result_t enum type and the template type
 */
#define RESULT(type) type ## _result_t

typedef enum {
    _OK,
    _ERR
} result_t;

/* @brief:  defines the result type for a given base type 
 * @param:  type - the type that result will be derived for
 */
#define DERIVE_RESULT_DIRECT(type) \
    typedef struct {        \
        result_t code;      \
        type value;         \
    } RESULT(type) 

/* @brief:  defines the result type for a given base type 
 * @param:  type - the type that result will be derived for
 * @param:  alias - alias for the type (for example pchar instead of char*)
 */
#define DERIVE_RESULT_INDIRECT(type, alias) \
    typedef struct {        \
        result_t code;      \
        type value;         \
    } RESULT(alias) \


/* @brief:  wraps the value in a result with code OK 
 * @param:  type - the type of the value to be wrapped
 * @parap:  val - the value to be wrapped 
 */
#define OK(type, val) \
    (RESULT(type)){ .code = _OK, .value = (val) }

/* @brief:  wraps the value in a result with code ERR 
 * @param:  type - the type of the value to be wrapped
 * @parap:  val - the value to be wrapped 
 */
#define ERR(type, val) \
    (RESULT(type)){ .code = _ERR, .value = (val) }

#if GNU_STATEMENT_EXPR

/* @brief:  unwraps the value contained in a RESULT() type. Uses the GNU extension
 * statement expressions
 * @param:  result - the return value of the function to be unwrapped 
 */
#define UNWRAP(result, handler) \
    ({ \
        typeof(result) _res = (result); \
        if (_res.code == _ERR) { \
            handler; \
        } \
        _res.value; \
    })

#else
/* @brief:  unwraps the value contained in RESULT() type - ISO C version
 * @param:  result - the return value of the function to be unwrapped 
 * @return: the result of the expression is the value if there is no error.
 *          if there is an error the function jumps to the saved stack frame
 */
#define UNWRAP(result)  ( (result).code == _OK) \
                        ? (result).value \
                        : (longjmp(result_handler, 1), (typeof((result).value))0)

/* @brief:  sets the result handler before running unwrap */
#define TRY { jmp_buf result_handler; \
    if (setjmp(result_handler) == 0)

/* @brief:  syntax sugar for TRY CATCH */
#define CATCH(handler) else { handler } }

#endif

#endif
