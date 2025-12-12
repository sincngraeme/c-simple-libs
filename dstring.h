/*******************************************************************************
* Name:             dstring.h                                                  *
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
 *      - dstring is a struct on the stack
 *      - String data is stored as on contiguous block on the heap
 */

typedef struct {
    char* start;
    char* end;
} dstring;

// Setup our error value checking
DERIVE_RESULT_INDIRECT(char*, p_char);
DERIVE_RESULT_DIRECT(char);
DERIVE_RESULT_DIRECT(dstring);
DERIVE_RESULT_DIRECT(size_t);

/* This a list of all the methods implemented for dstring */
typedef struct {
    /* @brief:  Allocates a new dynamic string and initializes to a given string literal
     * @param:  const char* str - The string literal to iniitalize to 
     * @return: RESULT(p_dstring) (literally: p_dstring_result_t)
     *          - Result struct containing error enum and a pointer to dynamic string type
     *          - (The new dynamic string) */
    RESULT(dstring) (*new)(const char* str);
    /* @brief:  Gets the current string content of the dynamic string 
     * @param:  dstring self - Dynamic string to get string data from 
     * @return: RESULT(p_char) - string data (null terminated) */
    RESULT(p_char) (*str)(dstring self);
    /* @brief:  Gets a string slice (partial string) from the string
     * @param:  dstring self - Dynamic string to get string data from 
     * @return: RESULT(p_char) - null terminated string data (performs copy) */
    RESULT(p_char) (*slice)(dstring self, unsigned long lower, unsigned long upper);
    /* @brief:  Gets the current size of the dynamic string 
     * @param:  String to get size of
     * @return: RESULT(size_t) - on success returns the size */
    RESULT(size_t) (*size)(dstring self);
    /* @brief:  Gets the current length of the dynamic string (excludes null byte)
     * @param:  String to get size of
     * @return: RESULT(size_t) - on success returns the size */
    RESULT(size_t) (*len)(dstring self);
    /* @brief:  Gets a character at a given index 
     * @param:  dstring self - string to index into
     * @param:  long index - index of the character to return
     * @return: RESULT(char) - on success returns character found at index. */
    RESULT(char) (*index)(dstring self, unsigned long index);
    /* @brief:  Gets a pointer to a character at a given index 
     * @param:  dstring self - string to index into
     * @param:  long index - index of the character to return
     * @return: RESULT(p_char) - on success returns character pointer found at index. */
    RESULT(p_char) (*index_p)(dstring* self, unsigned long index);
    /* @brief:  Appends a string literal to the given dynamic string
     * @param:  dstring* self - reference to the dynamic string
     * @param:  const char* str - string literal to append
     * @return: RESULT(size_t)  - new size of string */
    RESULT(size_t) (*append)(dstring* self, const char* str);
    /* @brief:  Appends the given dynamic string to a string literal
     * @param:  dstring** self - reference to the dynamic string
     * @param:  const char* str - string literal to append to 
     * @return: RESULT(size_t) - new size of string */
    RESULT(size_t) (*prepend)(dstring* self, const char* str);
    /* @brief:  Removes n characters from the beginning of the string
     * @param:  dstring** self - reference to the dynamic string
     * @param:  const char* str - string literal to append to 
     * @return: RESULT(size_t) - new size of string */
    RESULT(p_char) (*strip_prefix)(dstring* self, unsigned long index);
    /* @brief:  Removes n characters from the end of the string
     * @param:  dstring** self - reference to the dynamic string
     * @param:  const char* str - string literal to append to 
     * @return: RESULT(p_char) (char*) - pointer to the begining of the removed prefix */
    RESULT(p_char) (*strip_suffix)(dstring* self, unsigned long index);
    /* @brief:  Deletes a dynamic string (just the start and end pointers, the stack memory will remain)
     * @param:  dstring* self - dstring to delete
     * @return: RESULT(p_char) (char*) - pointer to the begining of the removed suffix */
    void (*del)(dstring* self);
} dstring_methods;
typedef dstring* pdstring;

/* Interface: User calls string init with a given alias and accesses functions through the 
 *              resulting method struct with the name <alias> */
#define DSTRING_INIT(alias) \
    dstring_methods alias = (dstring_methods){ \
        .new = dstring_new, \
        .str = get_string, \
        .slice = get_slice, \
        .size = get_size, \
        .len = get_length, \
        .index = get_character_at_index, \
        .index_p = get_character_at_index_p, \
        .append = append, \
        .prepend = prepend, \
        .strip_prefix = strip_prefix, \
        .strip_suffix = strip_suffix, \
        .del = dstring_delete, \
    }

static inline RESULT(p_char) get_string(dstring self) {
    if(self.start == nullptr || self.end == nullptr) return ERR(p_char, nullptr);
    if(*self.end == '\0' && (uint64_t)(self.end - self.start) < DSTRING_MAX_SIZE )  {
        return OK(p_char, self.start);
    } else {
        return ERR(p_char, nullptr);
    }
}

static inline RESULT(p_char) get_slice(dstring self, unsigned long lower, unsigned long upper) {
    if(self.start == nullptr || self.end == nullptr) {
        return ERR(p_char, (char*)1);
    } else if ((self.end - self.start) < 0 
        || (uint64_t)(self.end - self.start) > DSTRING_MAX_SIZE 
        || *self.end != '\0') 
    {
        return ERR(p_char, (char*)2);
    } else if (upper < lower){
        return ERR(p_char, (char*)3);
    } else if ((self.start + lower) < self.start || (self.start + lower) >= self.end){
        return ERR(p_char, (char*)4);
    } else if ((self.start + upper) < self.start || (self.start + upper) >= self.end){
        return ERR(p_char, (char*)5);
    } else {
        unsigned long int slice_size = upper - lower + 1;
        char* slice = (char*)malloc(slice_size * sizeof(char));
        memcpy(slice, self.start + lower, slice_size);
        slice[slice_size] = '\0';
        return OK(p_char, slice);
    }
}

static inline RESULT(size_t) get_size(dstring self) {
    if(self.start == nullptr || self.end == nullptr) {
        return ERR(size_t, 0);
    } else if((self.end - self.start) < 0) {
        return ERR(size_t, 0);
    } else {
        return OK(size_t, (size_t)(self.end - self.start));
    }
}

static inline RESULT(size_t) get_length(dstring self) {
    return OK(size_t, UNWRAP(get_size(self), {
            return ERR(size_t, 0);
    }) - 1);
}
static RESULT(char) get_character_at_index(dstring self, unsigned long index) {
    if(self.start == nullptr || self.end == nullptr) {
        return ERR(char, 0);
    } else if ((self.end - self.start) < 0 || *self.end != '\0') {
        return ERR(char, 1);
    } else if ((self.start + index) < self.start || (self.start + index) >= self.end){
        return ERR(char, 2);
    } else {
        return OK(char, *( self.start + index ));
    }
}

static RESULT(p_char) get_character_at_index_p(dstring* self, unsigned long index) {
    if(self->start == nullptr || self->end == nullptr) {
        return ERR(p_char, (char*)0);
    } else if ((self->end - self->start) < 0 || *self->end != '\0') {
        return ERR(p_char, (char*)1);
    } else if ((self->start + index) < self->start || (self->start + index) >= (char*)self->end){
        return ERR(p_char, (char*)2);
    } else {
        return OK(p_char, ( self->start + index ));
    }
}

static RESULT(size_t) append(dstring* self, const char* str) {
    size_t str_size = strlen(str) + 1;
    if(self == nullptr || self->start == nullptr || self->end == nullptr) {
        return ERR(size_t, 0);
    } else if((self->end - self->start) < 0 || *self->end != '\0') {
        return ERR(size_t, 0);
    } else {
        // Make temporary variables on stack to store old values
        size_t current_size = UNWRAP(get_size(*self), {
            fprintf(stderr,  "Failed to get string size in method: Append");
            return ERR(size_t, 0);
        });
        char current_str[current_size];
        memcpy(current_str, self->start, current_size); // Copy the old string out
        self->start = (char*)realloc(self->start, current_size + str_size - 1);
        self->end = self->start + current_size + str_size - 1;
        // Copy the old string back in 
        memcpy(self->start, current_str, current_size);
        // Copy the new string to the end
        memcpy(self->start + current_size - 1, str, str_size);
        return OK(size_t, (size_t)(self->end - self->start));
    }
}

static RESULT(size_t) prepend(dstring* self, const char* str) {
    size_t str_size = strlen(str) + 1;
    if(self == nullptr || self->start == nullptr || self->end == nullptr) {
        return ERR(size_t, 0);
    } else if((self->end - self->start) < 0 || *self->end != '\0') {
        return ERR(size_t, 0);
    } else {
        // Make temporary variables on stack to store old values
        size_t current_size = UNWRAP(get_size(*self), {
            fprintf(stderr,  "Failed to get string size in method: Append");
            return ERR(size_t, 0);
        });
        char current_str[current_size];
        memcpy(current_str, self->start, current_size); // Copy the old string out
        self->start = (char*)realloc(self->start, current_size + str_size - 1);
        self->end = self->start + current_size + str_size - 1;
        // Copy the new string in
        memcpy(self->start, str, str_size);
        // Copy the old string back to the end
        memcpy(self->start + str_size - 1, current_str, current_size);
        return OK(size_t, (size_t)(self->end - self->start));
    }
}

static RESULT(p_char) strip_prefix(dstring* self, unsigned long index) {
    if(self == nullptr || self->start == nullptr || self->end == nullptr) {
        return ERR(p_char, nullptr);
    } else if((self->end - self->start) < 0 || *self->end != '\0') {
        return ERR(p_char, nullptr);
    } else if ((self->start + index) < self->start || (self->start + index) >= self->end){
        return ERR(p_char, nullptr);
    } else {
        // Make temporary variables on stack to store old values
        size_t current_size = UNWRAP(get_size(*self), {
            fprintf(stderr,  "Failed to get string size in method: Append");
            return ERR(p_char, nullptr);
        });
        long new_size = current_size - index;
        char current_str[current_size];
        // Copy the old string in 
        memcpy(current_str, self->start, current_size);
        // The prefix is as long as the split index (plus null byte)
        char* prefix = (char*)malloc((index + 1) * sizeof(char)); 
        memcpy(prefix, self->start, index); // Copy the old string out
        // ensure null terminated 
        prefix[index + 1] = '\0'; 
        // Resize for the smaller string
        self->start = (char*)realloc(self->start, new_size);
        // Copy the part of old string we want to keep back in
        memcpy(self->start, current_str + index, new_size);
        self->end = self->start + new_size;
        // Ensure null terminated
        *self->end = '\0';
        return OK(p_char, prefix);
    }
}

static RESULT(p_char) strip_suffix(dstring* self, unsigned long index) {
    if(self == nullptr || self->start == nullptr || self->end == nullptr) {
        return ERR(p_char, nullptr);
    } else if((self->end - self->start) < 0 || *self->end != '\0') {
        return ERR(p_char, nullptr);
    } else if ((self->start + index) < self->start || (self->start + index) >= self->end){
        return ERR(p_char, nullptr);
    } else {
        // Make temporary variables on stack to store old values
        size_t current_size = UNWRAP(get_size(*self), {
            return ERR(p_char, nullptr);
        });
        long new_size = current_size - index - 1;
        char current_str[current_size];
        // Copy the old string in 
        memcpy(current_str, self->start, current_size);
        // The suffix is as long as the split index (plus null byte)
        char* suffix = (char*)malloc((index + 1) * sizeof(char)); 
        memcpy(suffix, self->end - index - 1, index); // Copy the old string out
        // ensure null terminated 
        suffix[index + 1] = '\0'; 
        // Resize for the smaller string
        self->start = (char*)realloc(self->start, new_size);
        // Copy the part of old string we want to keep back in
        memcpy(self->start, current_str, new_size);
        self->end = self->start + new_size;
        *self->end = '\0'; 
        return OK(p_char, suffix);
    }
}

static inline RESULT(dstring) dstring_new(const char* str) {
    dstring fail = (dstring){ .start = nullptr,.end = nullptr }; // Value to return on fail
    if(str == nullptr) return ERR(dstring, fail);
    uint64_t str_size = strlen(str) + 1;
    // Allocate space for both the string and string literal
    char* string = (char*)malloc(str_size * sizeof(char)); 
    if(string == nullptr) return ERR(dstring, fail);
    // Copy the string into the allocated space 
    strcpy(string, str);

    dstring final_string = {
        .start = string,
        .end = string + str_size // End of the actual string data
    };
    return OK(dstring, final_string);
}

void dstring_delete(dstring* self) {
    free(self->start);
}
