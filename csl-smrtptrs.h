/* vim: set ft=c : */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef SMRTPTR_IMPLEMENTATION

/* Used mainly for determining type of pointer to create when cloning */
enum smrtptr_types {
    UNIQUE_PTR,
    WEAK_PTR,
    SHARED_PTR,
    SHARED_PTR_ATOMIC,
};

enum smrtptr_errors {
    SMRTPTR_NOERR,
    SMRTPTR_INVALID_TYPE,
    SMRTPTR_MAKE_RECIEVED_NULL,
    SMRTPTR_NULL_CTRL_BLOCK,
};

[[noreturn]] void smrtptr_log_error(enum smrtptr_errors code) {
    switch (code) {
        case SMRTPTR_MAKE_RECIEVED_NULL: {
            fprintf(stderr, "ERROR: make_smrtptr recieved NULL pointer");
            exit(SMRTPTR_MAKE_RECIEVED_NULL);
        }
        case SMRTPTR_INVALID_TYPE: {
            fprintf(stderr, "ERROR: Invalid smrtptr type. "
                    "must be of type 'enum smrtptr_types'\n");
            exit(SMRTPTR_INVALID_TYPE);
        }
        case SMRTPTR_NULL_CTRL_BLOCK: {
            fprintf(stderr,
                "ERROR: Smart pointer control block was NULL\n");
            exit(SMRTPTR_NULL_CTRL_BLOCK);
        }
        default: {
            fprintf(stderr, "ERROR: Unknown code");
            exit(-1);
        }
    }
}

#ifdef UNIQUE_PTR_TYPE_LIST 

typedef struct {
    void* ptr;
    void (*destructor)(void*);
} _void_unique_ptr; 

#define UNIQUE_PTR_DERIVE(T, free_fn)        \
    typedef struct {                \
        T* ptr;                     \
        void (*destructor)(void*);  \
    } T##_unique_ptr; 

UNIQUE_PTR_TYPE_LIST 
#undef UNIQUE_PTR_DERIVE

#define UNIQUE_PTR_DERIVE(T, free_fn) T##_unique_ptr _is_##T;

/* Generate a union which contains all unique ptr types. These will differ
 * only in the base type they contain */
union unique_ptr_types { 
    UNIQUE_PTR_TYPE_LIST
    _void_unique_ptr generic;   // All types will be processed as this type (void*)
};

UNIQUE_PTR_TYPE_LIST 
#undef UNIQUE_PTR_DERIVE

/* This takes in a void ptr so it can be compatible with all smrtptr types
 * We then cast the ptr to the union which contains all types and */
static void free_unique_ptr(void* smrtptr) {
    union unique_ptr_types _smrtptr = *(union unique_ptr_types*)smrtptr;
    _smrtptr.generic.destructor(_smrtptr.generic.ptr);
}

/* @brief: unique_ptr type macro */
#define unique_ptr(T) __attribute__(( cleanup(free_unique_ptr) )) T##_unique_ptr
/* Calls the internal _make_unique_ptr function and specifies the return type based 
 * on the first parameter */
#define make_unique_ptr(T, alloc, dealloc) _make_unique_ptr(alloc, dealloc)._is_##T

static union unique_ptr_types _make_unique_ptr(void* alloc, void (*dealloc)(void*)) {
    _void_unique_ptr smrtptr = { .ptr = NULL, .destructor = NULL };
    if(alloc != NULL && dealloc != NULL) {
        smrtptr.ptr = alloc;
        smrtptr.destructor = dealloc;
    }
    return (union unique_ptr_types)smrtptr;
}

#endif

#ifdef SHARED_PTR_TYPE_LIST
/* True shared_ptrs like in C++ */

/* The control block that is used for shared and weak pointers for all base types */
typedef struct {
    unsigned int nshared;
    unsigned int nweak;
    void (*destructor)(void*);
} shared_ptr_ctrlblk;

/* @brief:      Defines the necessary structures and unions for a the list of weak and 
 *              shared pointer types
 *                  - T##_shared_ptr: Clonable owning pointer
 *                  - T##_weak_ptr: Clonable non-owning pointer (can be promoted)
 *                  - T##_smrtptr_option: Union containing a weak or shared pointer 
 *                      - Used for generic return from cloning funcions
 * @param:      [T] - the type to derive a shared pointer type for 
 * @param:      [free_fn] (UNUSED): ~function pointer to the function responsible for freeing the memory
 *                      must be of type void (*)(void*)~
 */
#define SHARED_PTR_DERIVE(T, free_fn)   \
    typedef struct {                    \
        T *ptr;                         \
        shared_ptr_ctrlblk *ctrl;       \
    } T##_shared_ptr;                   \
    \
    typedef struct {                    \
        T* _ptr;                        \
        shared_ptr_ctrlblk* ctrl;       \
    } T##_weak_ptr;                     \

SHARED_PTR_TYPE_LIST
#undef SHARED_PTR_DERIVE

/* Used internally for generic ptr types */
typedef struct {
    void* ptr;
    shared_ptr_ctrlblk* ctrl;
} _generic_shared_ptr;

#define SHARED_PTR_DERIVE(T, unused) T##_shared_ptr _is_##T;
union shared_ptr_types {
    SHARED_PTR_TYPE_LIST
    _generic_shared_ptr generic;
};
#undef SHARED_PTR_DERIVE

#define SHARED_PTR_DERIVE(T, unused) T##_weak_ptr _is_##T;
union weak_ptr_types {
    SHARED_PTR_TYPE_LIST
    _generic_shared_ptr generic;
};
#undef SHARED_PTR_DERIVE

typedef union {
    union shared_ptr_types IS_SHARED_PTR;
    union weak_ptr_types IS_WEAK_PTR;
} shared_ptr_option;

/* @brief:      Makes a shared pointer from an allocated void pointer and deallocates using the
 *              specified deallocator when fully out of scope 
 * @param:      void* alloc:    the pointer to *freshly* allocated memory 
 * @param:      void (*dealloc)(void*): function pointer that will be called when the pointer goes
 *              out of scope and gets freed 
 */
[[nodiscard]] static union shared_ptr_types _make_shared_ptr(void *alloc, void (*dealloc)(void*)) {
    if (alloc == NULL) {
        smrtptr_log_error(SMRTPTR_MAKE_RECIEVED_NULL);
    }
    shared_ptr_ctrlblk *temp_ctrl = malloc(sizeof(shared_ptr_ctrlblk));
    *temp_ctrl = (shared_ptr_ctrlblk){
        .nshared = 1,
        .nweak = 0,
        .destructor = dealloc
    };
    _generic_shared_ptr generic_ptr = {
        .ptr = alloc,
        .ctrl = temp_ctrl,
    };
    return (union shared_ptr_types)generic_ptr;
}

/* @brief:      clones a shared pointer into a weak or shared pointer 
 * @param:      */
[[nodiscard]] static shared_ptr_option _clone_shared_ptr(
    union shared_ptr_types ptr,
    enum smrtptr_types type
) {
    if(ptr.generic.ctrl == NULL) smrtptr_log_error(SMRTPTR_NULL_CTRL_BLOCK);
    switch (type) {
        case SHARED_PTR: {
            ptr.generic.ctrl->nshared++;
            break;
        }
        case WEAK_PTR: {
            ptr.generic.ctrl->nweak++;
            break;
        }
        default: {
            smrtptr_log_error(SMRTPTR_INVALID_TYPE);
        }
    }
    return (shared_ptr_option)ptr;
}

static void free_shared_ptr(void* ptr) {
    _generic_shared_ptr* _ptr = (_generic_shared_ptr*)ptr;
    if(_ptr->ctrl == NULL) smrtptr_log_error(SMRTPTR_NULL_CTRL_BLOCK);
    _ptr->ctrl->nshared--;
    if(_ptr->ctrl->nshared == 0) {
        _ptr->ctrl->destructor(_ptr->ptr);
        if(_ptr->ctrl->nweak == 0) free(_ptr->ctrl);
    }
}

/* Weak pointers (are implemented for any type that shared ptr is implemented for) */
[[nodiscard]] shared_ptr_option _clone_weak_ptr(
    union weak_ptr_types ptr,
    enum smrtptr_types type
) {
    switch(type){
        case WEAK_PTR: {
            ptr.generic.ctrl->nweak++;
            return (shared_ptr_option)ptr;
        }
        case SHARED_PTR: {
             if(ptr.generic.ctrl->nshared > 0) {
                 ptr.generic.ctrl->nshared++;
                 return (shared_ptr_option)ptr;
             }
             return (shared_ptr_option){0};
        }
        default: smrtptr_log_error(SMRTPTR_INVALID_TYPE);
    }
}

static void free_weak_ptr(void* ptr) {
    _generic_shared_ptr* _ptr = ptr;
    _ptr->ctrl->nweak--;
    if(_ptr->ctrl->nshared == 0 && _ptr->ctrl->nweak == 0) free(_ptr->ctrl);
}

#undef SHARED_PTR_DERIVE

#define shared_ptr(T) __attribute__(( cleanup(free_shared_ptr) )) T##_shared_ptr
#define weak_ptr(T) __attribute__(( cleanup(free_weak_ptr) )) T##_weak_ptr
#define make_shared_ptr(T, alloc, dealloc)  _make_shared_ptr(alloc, dealloc)._is_##T 
#define clone_shared_ptr(T, ptr, type) _clone_shared_ptr((union shared_ptr_types)ptr, type).IS_##type._is_##T
#define clone_weak_ptr(T, ptr, type) _clone_weak_ptr((union weak_ptr_types)ptr, type).IS_##type._is_##T
#define deref_smrtptr(smrtptr) *smrtptr.ptr
#define ref_smrtptr(smrtptr) smrtptr.ptr
#define is_ptr_dead(smrtptr) !!(( smrtptr ).ctrl->nshared)
#endif
#endif
