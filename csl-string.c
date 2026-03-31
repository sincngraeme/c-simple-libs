/*******************************************************************************
* Name:             string.c                                                   *
* Description:      lenght based and dynamic string implementation             *
* By:               Nigel Sinclair                                             *
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
*                   All functions that can produce errors return the WRESULT    *
*                   type (WRESULT(type)). This expands to type_result_t.        *
*                   For information on how to access returned values from      *
*                   WRESULT(type) functions see errval.h                        *
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "csl-errval.h"

#define STRING_END UINT64_MAX

/* Dynamic String type: Made up of start and end pointer to string content. 
 *      - size is found with size() method by subtracting the end and start pointers
 *      - dstring is a struct on the stack
 *      - String data is stored as on contiguous block on the heap
 */
typedef struct {
    char* start;
    size_t size;
} String;

typedef struct {
    String s;
    size_t capacity;
} DString;

// Setup our error value checking
DERIVE_WRESULT(char);
DERIVE_WRESULT(DString);
DERIVE_WRESULT(String);
DERIVE_WRESULT(size_t);
DERIVE_WRESULT(bool);

#define STRFMT(S) (S).size, (S).start

void dstring_delete(DString* self);
WRESULT(DString) dstring_new(const char* str);
WRESULT(size_t) dstring_stripsuff(DString* self, unsigned long index);
WRESULT(size_t) dstring_strippref(DString* self, unsigned long index);
WRESULT(size_t) dstring_prepend(DString* self, const char* str);
WRESULT(size_t) dstring_append(DString* self, const char* str);
size_t dstring_get_size(DString self);
WRESULT(String) dstring_get_slice(DString self, size_t lower, size_t upper);

#if !defined(CSL_STRING_INTERFACE)

/* @brief:  Gets a string slice (partial string) from the string
 * @param:  dstring self - Dynamic string to get string data from 
 * @return: WRESULT(p_char) - null terminated string data (performs copy) */
WRESULT(String) dstring_get_slice(DString self, size_t lower, size_t upper) {
    String fail = (String){0};
    if(
        upper > self.s.size  ||
        lower > upper
    ) return WRESULT_ERR(String, fail);
    String success = { .size = (upper - lower), .start = &self.s.start[lower] };
    return WRESULT_OK(String, success);
}

/* @brief:  Gets the current length of the dynamic string
 * @param:  String to get size of
 * @return: WRESULT(size_t) - on success returns the size */
size_t dstring_get_size(DString self) {
    return self.s.size;
}

/* @brief:  Appends a string literal to the given dynamic string
 * @param:  dstring* self - reference to the dynamic string
 * @param:  const char* str - string literal to append
 * @return: WRESULT(size_t)  - new size of string */
WRESULT(size_t) dstring_append(DString* self, const char* str) {
    size_t str_len = strlen(str);
    if(self == NULL) return WRESULT_ERR(size_t, 0);
        // Make temporary variables on stack to store old values
        size_t current_size = self->s.size;
        self->s.start = (char*)realloc(self->s.start, current_size + str_len);
        // Copy the new string to the end
        memcpy(&self->s.start[current_size], str, str_len);
        self->s.size += str_len;
        return WRESULT_OK(size_t, (size_t)(self->s.size));
}

/* @brief:  Prepends the given dynamic string to a string literal
 * @param:  dstring** self - reference to the dynamic string
 * @param:  const char* str - string literal to append to 
 * @return: WRESULT(size_t) - new size of string */
WRESULT(size_t) dstring_prepend(DString* self, const char* str) {
    size_t str_len = strlen(str);
    if(self == NULL) return WRESULT_ERR(size_t, 0);
    size_t current_size = self->s.size;
    size_t new_size = current_size + str_len;
    self->s.start = (char*)realloc(self->s.start, new_size);
    if(self->s.start == NULL) return WRESULT_ERR(size_t, 1);
    // Copy the original data to the end where the prepended string will be
    memmove(&self->s.start[str_len], self->s.start, current_size);
    // Prepend the string
    memmove(self->s.start, str, str_len);
    self->s.size = new_size;
    self->capacity = new_size;
    return WRESULT_OK(size_t, self->s.size);
}

/* @brief:  Removes n characters from the beginning of the string
 * @param:  dstring** self - reference to the dynamic string
 * @param:  const char* str - string literal to append to 
 * @return: WRESULT(size_t) - new size of string */
WRESULT(size_t) dstring_strippref(DString* self, size_t n) {
    if(
        self == NULL                    ||
        n > self->s.size                ||
        self->s.size > self->capacity
    ) WRESULT_ERR(size_t, 0);

    size_t new_size = self->s.size - n;
    memmove(self->s.start, self->s.start + n, new_size);
    self->s.start = (char*)realloc(self->s.start, new_size);
    if(self->s.start == NULL) return WRESULT_ERR(size_t, 0);
    self->s.size = new_size;
    self->capacity = new_size;
    return WRESULT_OK(size_t, new_size);
}

/* @brief:  Removes n characters from the end of the string
 * @param:  dstring** self - reference to the dynamic string
 * @param:  const char* str - string literal to append to 
 * @return: WRESULT(p_char) (char*) - pointer to the begining of the removed prefix */
WRESULT(size_t) dstring_stripsuff(DString* self, size_t n) {
    if(
        self == NULL                    ||
        n > self->s.size                ||
        self->s.size > self->capacity
    ) return WRESULT_ERR(size_t, 0);

    size_t new_size = self->s.size - n;
    self->s.start = (char*)realloc(self->s.start, new_size);
    if(self->s.start == NULL) return WRESULT_ERR(size_t, 0);
    return WRESULT_OK(size_t, new_size);
}

/* @brief:  Allocates a new dynamic string and initializes to a given string literal
 * @param:  const char* str - The string literal to iniitalize to 
 * @return: WRESULT(p_dstring) (literally: p_dstring_result_t)
 *          - Result struct containing error enum and a pointer to dynamic string type
 *          - (The new dynamic string) */
WRESULT(DString) dstring_new(const char* str) {
    DString fail = (DString){0}; 
    if(str == NULL) return WRESULT_ERR(DString, fail);
    size_t str_length = strlen(str);
    size_t alloc_size = str_length * sizeof(char) * 1.5;
    if(str_length > UINT8_MAX || alloc_size > UINT8_MAX) return WRESULT_ERR(DString, fail);
    char* string = (char*)malloc(alloc_size); 
    if(string == NULL) return WRESULT_ERR(DString, fail);
    // Copy the string into the allocated space 
    strcpy(string, str);
    DString final_string = {
        .s = { .size = str_length, .start = string },
        .capacity = alloc_size
    };
    return WRESULT_OK(DString, final_string);
}

/* @brief:  Deletes a dynamic string (just the start and end pointers, the stack memory will remain)
 * @param:  dstring* self - dstring to delete
 * @return: WRESULT(p_char) (char*) - pointer to the begining of the removed suffix */
void dstring_delete(DString* self) {
    free(self->s.start);
}

#else

#if defined(STRING_USE_VTABLE)
#endif

#endif
