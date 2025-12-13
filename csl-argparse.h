/*******************************************************************************
* Name:             csl-argparse.h                                             *
* Description:      Command line argument parsing library                      *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
* Implementation:                                                              *
* Initialization:                                                              *
* Usage:                                                                       *
*******************************************************************************/

#ifndef _ARGPARSE_H
#define _ARGPARSE_H

#include <stdio.h>
#include "csl-recursed.h"

/* VA Expansion */

/*** Flags (Indicate boolean options) ***/
#define _ARGPARSE_ADD_FLAG(name, flag) name = flag,
#define ARGPARSE_ADD_FLAG() _ARGPARSE_ADD_FLAG
#define ARGPARSE_GET_FLAG(arg) ARGPARSE_ADD_FLAG PARENS arg
/* Constructs the flags enum to store all the boolean options */
#define ARGPARSE_FLAGS(...)                         \
    typedef enum {                                  \
        FOR_EACH(ARGPARSE_GET_FLAG, __VA_ARGS__)    \
    } argparse_flags;                               \

/*** Optional Arguments ***/
typedef struct {
    const char* name;
    const char* value;
} argparse_optarg;

/* Constructs an individual entry in the optional argument table */
#define _ARGPARSE_INIT_OPTARG(_name, alias)            \
    argparse_optarg _name;

#define ARGPARSE_INIT_OPTARG() _ARGPARSE_INIT_OPTARG
#define ARGPARSE_GET_OPTARG1(arg) ARGPARSE_INIT_OPTARG PARENS arg

/* Initializes an individual entry in the optional argument table */
#define _ARGPARSE_ADD_OPTARG(_name, alias)            \
    ._name = (argparse_optarg){ .name = #alias,  .value = NULL },

#define ARGPARSE_ADD_OPTARG() _ARGPARSE_ADD_OPTARG
#define ARGPARSE_GET_OPTARG2(arg) ARGPARSE_ADD_OPTARG PARENS arg

/* Constructs the optional argument struct to store all the 
 * optional arguments */
#define ARGPARSE_OPTARGS(...)                                               \
    typedef struct {                                                        \
        FOR_EACH(ARGPARSE_GET_OPTARG1, __VA_ARGS__)                         \
    } argparse_optargs;                                                     \
    argparse_optargs ARGPARSE_PREFIX##optarg_table = (argparse_optargs) {  \
        FOR_EACH(ARGPARSE_GET_OPTARG2, __VA_ARGS__)                         \
    };

/*** Positional Parameters ***/

#define _ARGPARSE_ADD_POSITIONAL(name, index) name = index,
#define ARGPARSE_ADD_POSITIONAL() _ARGPARSE_ADD_POSITIONAL
#define ARGPARSE_GET_POSITIONAL(arg) ARGPARSE_ADD_POSITIONAL PARENS arg
/* Constructs the flags enum to store all the boolean options */
#define ARGPARSE_POSITIONALS(table_size, ...)                       \
        typedef enum {                                              \
            FOR_EACH(ARGPARSE_GET_POSITIONAL, __VA_ARGS__)          \
        } argparse_positional_indices;                              \
        const char* ARGPARSE_PREFIX##postional_table[table_size];           

/* @brief: parses all the arguments
 * @param: count (argc)
 * @param: string(argv)
 * TODO: parsing
 */
#define ARGPARSE_PARSE(flags_table_size)                    \
    char ARGPARSE_PREFIX##flags[flags_table_size];          \
    do {                                                    \
        int positional_count = 0;                           \
        for(int i = 0; i < argc; i++) {                     \
            if(*argc != '-' && *argc != NULL) {             \
                ARGPARSE_PREFIX##positional[positional_count] = argv;  \
            } else if(*argc == NULL) break;                 \
            else {                                          \
                                                            \
            }                                               \
        }                                                   \
    } while(0)                                              \

#endif
