/*******************************************************************************
* Name:             dstring.h                                                  *
* Description:      Dynamic string implementation                              *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
*******************************************************************************/

// TODO: X-macro for deriving types - initializationI
#ifndef GNU
#define GNU
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "errval.h"

#define DSTRING_MAX_SIZE UINT64_MAX

/* Dynamic String type: Made up of start and end pointer to string content. 
 *      - size is found with size() method by subtracting the end and start pointers
 *      - Data is stored as on contiguous block on the heap:
 *
 *      | part      | size              |
 *      | --------- | ----------------- |
 *      | dstring   | 2 * sizeof(char*) |<~~~~~~~ This is made up of start and end
 *      | str       | start - end       |
 */

typedef struct {
    char* start;
    char* end;
} dstring;

// Setup our error value checking
DERIVE_RESULT_INDIRECT(dstring*, p_dstring);
DERIVE_RESULT_INDIRECT(char*, p_char);
DERIVE_RESULT_DIRECT(size_t);

/* This a list of all the methods implemented for dstring */
typedef struct dstring_methods {
    RESULT(p_dstring) (*new)(const char* str);
    RESULT(p_char) (*str)(dstring self);
    RESULT(size_t) (*size)(dstring self);
    RESULT(size_t) (*append)(dstring** self, const char* str);
} dstring_methods;
typedef dstring* pdstring;

/* Interface: User calls string init with a given alias and accesses functions through the 
 *              resulting method struct with the name <alias> */
#define DSTRING_INIT(alias) \
    dstring_methods alias = (dstring_methods){ \
        .new = dstring_new, \
        .str = get_string, \
        .size = get_size, \
        .append = append, \
    }

/* @brief:  Gets the current string content of the dynamic string 
 * @param:  dstring self - Dynamic string to get string data from 
 * @return: RESULT(p_char) - string data (null terminated)
 */
static inline RESULT(p_char) get_string(dstring self) {
    if(self.start == nullptr || self.end == nullptr) return ERR(p_char, nullptr);
    if(*self.end == '\0' && (uint64_t)(self.end - self.start) < DSTRING_MAX_SIZE )  {
        return OK(p_char, self.start);
    } else {
        return ERR(p_char, nullptr);
    }
}
/* @brief:  Gets the current size of the dynamic string 
 * @param:  String to get size of
 * @return: RESULT(size_t) - on success returns the size 
 */
static inline RESULT(size_t) get_size(dstring self) {
    if(self.start == nullptr || self.end == nullptr) {
        return ERR(size_t, 0);
    } else if((self.end - self.start) < 0) {
        return ERR(size_t, 0);
    } else {
        return OK(size_t, (size_t)(self.end - self.start));
    }
}
/* @brief:  Gets a character at a given index 
 * @param:  long index - index of the character to return
 * @return: RESULT(char) - on success returns character found at index.
 *          failure indicates out of bounds
 */

/* @brief:  Appends a string literal to the given dynamic string
 * @param:  dstring** self - reference to the dynamic string
 * @param:  const char* str - string literal to append
 */
static RESULT(size_t) append(dstring** self, const char* str) {
    size_t str_size = strlen(str) + 1;
    if((*self)->start == nullptr || (*self)->end == nullptr) {
        return ERR(size_t, 0);
    } else if(((*self)->end - (*self)->start) < 0 || *(*self)->end != '\0') {
        return ERR(size_t, 0);
    } else {
        // Make temporary variables on stack to store old values
        size_t current_size = UNWRAP(get_size(**self), {
            fprintf(stderr,  "Failed to get string size in method: Append");
            return ERR(size_t, 0);
        });
        char* current_str = (*self)->start;
        *self = (dstring*)realloc(*self, sizeof(dstring) + current_size + str_size - 1);
        (*self)->start = (char*)(*self + 1);
        (*self)->end = (*self)->start + current_size + str_size - 1;
        // Copy the old string back in 
        memcpy((*self)->start, current_str, current_size);
        // Copy the new string to the end
        memcpy((*self)->start + current_size - 1, str, str_size);
        return OK(size_t, (size_t)((*self)->end - (*self)->start));
    }
}
/* @brief:  Allocates a new dynamic string and initializes to a given string literal
 * @param:  const char* str - The string literal to iniitalize to 
 * @return: RESULT(p_dstring) (literally: p_dstring_result_t)
 *          - Result struct containing error enum and a pointer to dynamic string type
 *          - (The new dynamic string)
 */
static inline RESULT(p_dstring) dstring_new(const char* str) {
    if(str == nullptr) return ERR(p_dstring, nullptr);
    uint64_t str_size = strlen(str) + 1;
    // Allocate space for both the string and string literal
    dstring* string = (dstring*)malloc(sizeof(dstring) + str_size); 
    if(string == nullptr) return ERR(p_dstring, nullptr);
    string->start = (char*)(string + 1); // End of the dstring part
    string->end = string->start + str_size; // End of the actual string data
    // Copy the string into the allocated space 
    strcpy(string->start, str);
    return OK(p_dstring, string);
}
