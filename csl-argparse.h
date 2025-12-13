/*******************************************************************************
* Name:             csl-argparse.h                                             *
* Description:      Dynamic string implementation                              *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
* Implementation:   The dstring type is the dynamic string itself. The struct  *
*                   is a stack resource, which contains pointers to the        *
*                   begining and end of the string.                            *
* Initialization:   Call `DSTRING_INIT(alias)` where alias is whatever name    *
*                   you want to use for the string type interface.             *
*                   - This initializes the dstring_methods struct with all     *
*                       the function pointers                                  *
* Usage:            To call any string functions you simply use the alias you  *
*                   called DSTRING_INIT with, and access the function pointers *
*                   from the struct.                                           *
*                   ex: `String.append(&my_string, "Hello There")`             *
*                   - Note that with the exception of .new all require a       *
*                       self reference as the first argument. For methods      *
*                       that modify the string contents, pass by reference.    *
*                       For methods that do not modify the string contents,    *
*                       pass by value.                                         *
*                   All functions that can produce errors return the RESULT    *
*                   type (RESULT(type)). This expands to type_result_t.        *
*                   For information on how to access returned values from      *
*                   RESULT(type) functions see errval.h                        *
*******************************************************************************/

// X macro for defining positional parameters
// X macro for defining simple flags
// X macro for defining flags with arguments

/* VA Expansion */

/* @brief: parses all the arguments
 * @param: count (argc)
 * @param: string(argv)
 * @returns: 
 */


