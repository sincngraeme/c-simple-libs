#include <stdlib.h>
#include <stdio.h>

#ifdef SMRTPTR_IMPLEMENTATION

unsigned int shared_ptr_nrefs = 0;

void create_shared_ptr() {
    shared_ptr_nrefs++;
}
void free_shared_ptr(void* smrtptr) {
    void** _smrtptr = (void**)smrtptr;
    shared_ptr_nrefs--;
    if(shared_ptr_nrefs == 0) free(*_smrtptr);
}
void free_unique_ptr(void* smrtptr) {
    void** _smrtptr = (void **)smrtptr;
    free(*_smrtptr);
}

#define shared_ptr(T)                           \
    create_shared_ptr();                          \
    __attribute__((cleanup(free_shared_ptr))) T* 

#define unique_ptr(T) __attribute__((cleanup(free_unique_ptr))) T* 

#endif

