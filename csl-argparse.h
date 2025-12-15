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
#include <string.h>
#include "csl-recursed.h"

#define ARGPARSE_STRINGIFY(arg) #arg,

/* VA Expansion */

/*** Flags (Indicate boolean options) ***/
#define _ARGPARSE_INIT_FLAG(name, flag) name = flag,
#define ARGPARSE_INIT_FLAG() _ARGPARSE_INIT_FLAG
#define ARGPARSE_GET_FLAG1(arg) ARGPARSE_INIT_FLAG PARENS arg
/* Construct the flags table */
#define _ARGPARSE_ADD_FLAG(name, flag) flag,
#define ARGPARSE_ADD_FLAG() _ARGPARSE_ADD_FLAG
#define ARGPARSE_GET_FLAG2(arg) ARGPARSE_ADD_FLAG PARENS arg
/* Constructs the flags enum to store all the boolean options */
#define ARGPARSE_FLAGS(...)                                             \
    typedef enum {                                                      \
        FOR_EACH(ARGPARSE_GET_FLAG1, __VA_ARGS__)                        \
    } argparse_flags;                                                   \
    static const int argparse_flags_table_size =                               \
        COUNT_ARGS(char*, FOR_EACH(ARGPARSE_STRINGIFY, __VA_ARGS__));   \
    static char argparse_flags_table[] = {                                     \
        FOR_EACH(ARGPARSE_GET_FLAG2, __VA_ARGS__)                       \
    };

/*** Optional Arguments ***/

/* Constructs an individual entry in the optional argument table */
#define _ARGPARSE_INIT_OPTARG(name, alias) name,

#define ARGPARSE_INIT_OPTARG() _ARGPARSE_INIT_OPTARG
#define ARGPARSE_GET_OPTARG1(arg) ARGPARSE_INIT_OPTARG PARENS arg

/* Populates the optarg table */
#define _ARGPARSE_ADD_OPTARG(name, alias) { alias, NULL },

#define ARGPARSE_ADD_OPTARG() _ARGPARSE_ADD_OPTARG
#define ARGPARSE_GET_OPTARG2(arg) ARGPARSE_ADD_OPTARG PARENS arg

/* Constructs the optional argument struct to store all the
 * optional arguments */
#define ARGPARSE_OPTARGS(...)                                               \
    typedef enum {                                                          \
        FOR_EACH(ARGPARSE_GET_OPTARG1, __VA_ARGS__)                         \
    } argparse_optargs;                                                     \
    static const int argparse_optarg_table_size =                                  \
        COUNT_ARGS(char*, FOR_EACH(ARGPARSE_STRINGIFY, __VA_ARGS__));       \
    static char* argparse_optarg_table[][2] = {                                    \
        FOR_EACH(ARGPARSE_GET_OPTARG2, __VA_ARGS__)                         \
    };

/*** Positional Parameters ***/

#define ARGPARSE_INIT_POSITIONAL(name) name,
#define ARGPARSE_ADD_POSITIONAL(name) NULL,
/* Constructs the flags enum to store all the boolean options */
#define ARGPARSE_POSITIONALS(...)                                               \
    typedef enum {                                                              \
        FOR_EACH(ARGPARSE_INIT_POSITIONAL, __VA_ARGS__)                         \
    } argparse_positional;                                                      \
    static const int argparse_postional_table_size =                            \
        COUNT_ARGS(char*, FOR_EACH(ARGPARSE_STRINGIFY, __VA_ARGS__));           \
    static char* argparse_postional_table[] = {                                 \
        FOR_EACH(ARGPARSE_ADD_POSITIONAL, __VA_ARGS__)                          \
    };



/* @brief: parses all the arguments
 * @param: count (argc)
 * @param: string(argv)
 * TODO: parsing
 * Takes in VA list made up of sublists for optionals, optargs and positionals.
 */
#define ARGPARSE_GET_FLAGS_TABLE(first, second, third)  \
    ARGPARSE_FLAGS second

#define ARGPARSE_GET_OPTARGS_TABLE(first, second, third)    \
    ARGPARSE_OPTARGS third

#define ARGPARSE_GET_POSITIONALS_TABLE(first, ...)  \
        ARGPARSE_POSITIONALS first

typedef enum {
    MISSING_POSITIONAL,
    MISSPLACED_POSITIONAL,
    INVALID_OPTARG_FORMAT,
    INVALID_OPTARG,
    INVALID_FLAG
} argparse_errors;


typedef struct argparse_interface {
    char*           flags;
    int             nflags;
    char***         optargs;
    int             noptargs;
    char**          positionals;
    int             npositionals;
    void          (*parse)(struct argparse_interface* data, int argc, char** argv);
    argparse_errors parse_errors;
} argparse_interface;

#define ARGPARSE_PARSE(...)                                                 \
    ARGPARSE_GET_POSITIONALS_TABLE(__VA_ARGS__)                             \
    ARGPARSE_GET_FLAGS_TABLE(__VA_ARGS__)                                   \
    ARGPARSE_GET_OPTARGS_TABLE(__VA_ARGS__)                                 \
    argparse_interface argparse = {                                         \
        .parse = argparse_parse,                                            \
        .nflags = argparse_flags_table_size,                                \
        .noptargs = argparse_optarg_table_size,                             \
        .npositionals = argparse_postional_table_size,                      \
    };                                                                      \
    argparse.parse(&argparse, argc, argv);

#define ARGPARSE_ERROR_LOG fprintf(stderr, "Failed to parse.\n")

/* Loops through the optional arguments and checks for a match */
static int argparse_lookup_optarg(argparse_interface* data, char* arg, int stop_index) {
    int found_index = -1;
    for(
        int i = 0; 
        i < data->noptargs && !(found_index = strncmp(data->optargs[i][1], arg, stop_index)); 
        i++
    );
    return found_index;
}

static int argparse_test_optarg(char* arg) {
    int index = 0;
    while(*arg != '\0'){
        if(*arg++ == '=') return index;
        index++;
    }
    return -1;
}

static int argparse_lookup_flag(argparse_interface* data, char arg) {
    int found_index = -1;
    for(int i = 0; i < data->nflags && (found_index = data->flags[i]) != arg; i++);
    return found_index;
}

static void argparse_parse(argparse_interface* data, int argc, char** argv) {
    for(int i = 0; i < argc; i++) {
        /* Positionals */
        if(i < data->npositionals) {
            if(*argv[i] == '-' || argv[i] == NULL) {
                ARGPARSE_ERROR_LOG;
                data->parse_errors = MISSING_POSITIONAL;
                return;
            } else {
                data->positionals[i] = argv[i];
                continue;
            }
        /* Optionals */
        } else if(*argv[i] == '-' && *argv[i + 1] == '-') { // TODO: optionals with no =
            int contents_index = argparse_test_optarg(argv[i] + 2);
            if(contents_index > -1) {
                ARGPARSE_ERROR_LOG;
                data->parse_errors = INVALID_OPTARG_FORMAT;
                return;
            }
            int optarg_index = argparse_lookup_optarg(data, argv[i] + 2, contents_index);
            if(optarg_index > -1) {
                // Skip the dashes, then skip to character after the =
                data->optargs[optarg_index][2] = argv[i] + 2 + contents_index + 1;
                continue;
            } else {
                ARGPARSE_ERROR_LOG;
                data->parse_errors = INVALID_OPTARG;
                return;
            }
        /* Flags */
        } else if(*argv[i] == '-' && *argv[i + 1] != '-') {
            for(int letter = 0; argv[i + 1][letter] != '\0'; letter++) {
                int flag_index = argparse_lookup_flag(data, argv[i + 1][letter]);
                if(flag_index > -1) {
                   data->flags[flag_index] = argv[i + 1][letter]; 
                } else {
                    ARGPARSE_ERROR_LOG;
                    data->parse_errors = INVALID_FLAG;
                }
            }
            continue;
        } else {
            ARGPARSE_ERROR_LOG;
            data->parse_errors = MISSPLACED_POSITIONAL;
            return;
        }
    }
}

#endif
