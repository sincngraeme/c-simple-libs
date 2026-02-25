/* vim: set ft=c: */
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    size_t nrefs;
    void (*destructor)(void*);
} smrtptr_ctrl_t;

typedef struct {
    void* data;
    smrtptr_ctrl_t* _ctrl;
} smrtptr_t;

static void* init_smrtptr(smrtptr_t* ptr, void (*destructor)(void*)) {
    ptr->_ctrl = (smrtptr_ctrl_t*)malloc(sizeof(smrtptr_ctrl_t));
    *ptr->_ctrl = (smrtptr_ctrl_t){
        .nrefs = 1,
        .destructor = destructor
    };
    return ptr;
}

static void free_smrtptr(void* ptr) {
    smrtptr_t* smrtptr = *(smrtptr_t**)ptr;
    if(--smrtptr->_ctrl->nrefs == 0) {
        smrtptr->_ctrl->destructor(smrtptr->data);
        free(smrtptr->_ctrl);
        printf("Freed: 0x%p\n", (void*)smrtptr);
    }
}

#define defer(fn_ptr) __attribute__((cleanup(fn_ptr)))
#define smrtptr(T) defer(free_smrtptr) T* const
