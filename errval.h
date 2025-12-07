/*******************************************************************************
* Name:             errval.h                                                   *
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
*                   3. Wrap function calls in TRY{ } CATCH{ }, and call with   *
*                       UNWRAP(func())                                         *
*                   4. UNDER NO CIRCUMSTANCES SHOULD YOU USE VLA'S OR ALLOCA   *
*                      in your function (you probably shouldn't be anyway)     *
*                       - UNWRAP calls longjmp() which does may skip stack     *
*                         frames. stack-local memory is undefined, scalars     *
*                         and pointers to heap-allocated memory are safe       *
*                                                                              *
*******************************************************************************/
#ifndef __ERRVAL_H
#define __ERRVAL_H

#include <stdio.h>
#include <setjmp.h>


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
    } RESULT(type); 

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

#ifdef GNU

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
#endif

// #else
/* @brief:  unwraps the value contained in RESULT() type - ISO C version
 * @param:  result - the return value of the function to be unwrapped 
 * @return: the result of the expression is the value if there is no error.
 *          if there is an error the function jumps to the saved stack frame
 */
#define EXPECT(result)  ( (result).code == _OK) \
                        ? (result).value \
                        : (longjmp(result_handler, 1), (typeof((result).value))0)

/* @brief:  sets the result handler before running unwrap */
#define TRY jmp_buf result_handler; \
    if (setjmp(result_handler) == 0)

/* @brief:  syntax sugar for TRY CATCH */
#define CATCH else

#endif
