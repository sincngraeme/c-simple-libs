/* vim: set ft=c : */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
#error "This library is C-only. Use <memory> for C++"
#endif

#ifdef SMRTPTR_IMPLEMENTATION

/* Used mainly for determining type of pointer to create when cloning */
enum smrtptr_types {
    SMRTPTR_UNIQUE,
    SMRTPTR_WEAK,
    SMRTPTR_STRONG,
    SMRTPTR_STRONG_ATOMIC,
    SMRTPTR_WEAK_ATOMIC,
};

enum smrtptr_errors {
    SMRTPTR_NOERR               = 0b00000,
    SMRTPTR_INVALID_TYPE        = 0b00001,
    SMRTPTR_MAKE_RECIEVED_NULL  = 0b00010,
    SMRTPTR_NULL_CTRL_BLOCK     = 0b00100,
    SMRTPTR_FREE_RECIEVED_NULL  = 0b01000,
    SMRTPTR_NULL_DESTRUCTOR     = 0b10000,
};

static enum smrtptr_errors smrtptr_errno = 0;

/****************************** UNIQUE POINTERS *******************************/// {{{

#ifdef SMRTPTR_UNIQUE_TYPE_LIST 

typedef struct {
    void* ptr;
    void (*destructor)(void*);
} _void_smrtptr_unique; 

#define SMRTPTR_DERIVE_UNIQUE(T, free_fn)        \
    typedef struct {                \
        T* ptr;                     \
        void (*destructor)(void*);  \
    } T##_smrtptr_unique; 

SMRTPTR_UNIQUE_TYPE_LIST 
#undef SMRTPTR_DERIVE_UNIQUE

#define SMRTPTR_DERIVE_UNIQUE(T, free_fn) T##_smrtptr_unique T##_member;

/* Generate a union which contains all unique ptr types. These will differ
 * only in the base type they contain */
union smrtptr_unique_types { 
    SMRTPTR_UNIQUE_TYPE_LIST
    _void_smrtptr_unique generic;   // All types will be processed as this type (void*)
};

#undef SMRTPTR_DERIVE_UNIQUE

/* This takes in a void ptr so it can be compatible with all smrtptr types
 * We then cast the ptr to the union which contains all types and */
static void smrtptr_free_unique(void* smrtptr) {
    if(smrtptr == NULL) {
        smrtptr_errno |= SMRTPTR_FREE_RECIEVED_NULL;
        return;
    }
    union smrtptr_unique_types _smrtptr = *(union smrtptr_unique_types*)smrtptr;
    if(_smrtptr.generic.destructor == NULL) {
        return;
    }
    _smrtptr.generic.destructor(_smrtptr.generic.ptr);
}
/* Returns a new unique pointer to the allocated memory pointed to by alloc, to be freed
 * later with dealloc */
static union smrtptr_unique_types _smrtptr_make_unique(void* alloc, void (*dealloc)(void*)) {
    _void_smrtptr_unique smrtptr = { .ptr = NULL, .destructor = NULL };
    if(alloc != NULL && dealloc != NULL) {
        smrtptr.ptr = alloc;
        smrtptr.destructor = dealloc;
    }
    return (union smrtptr_unique_types)smrtptr;
}

/* Moves the data from a unique pointer to another unique pointer */
__attribute__((unused))
static union smrtptr_unique_types _smrtptr_move_unique(union smrtptr_unique_types* src) {
    _void_smrtptr_unique dest = { .ptr = src->generic.ptr, .destructor = src->generic.destructor };
    src->generic.ptr = NULL;
    src->generic.destructor = NULL;
    return (union smrtptr_unique_types)dest;
}

/* @brief: smrtptr_unique type macro */
#define smrtptr_unique(T) __attribute__(( cleanup(smrtptr_free_unique) )) T##_smrtptr_unique
/* Calls the internal _make_smrtptr_unique function and specifies the return type based 
 * on the first parameter */
#define smrtptr_make_unique(T, alloc, dealloc) _smrtptr_make_unique(alloc, dealloc).T##_member
#define smrtptr_move_unique(T, ptr) _smrtptr_move_unique((union smrtptr_unique_types*)ptr).T##_member;

#endif // }}}

/******************************* SHARED POINTERS *******************************/// {{{

#ifdef SHARED_PTR_TYPE_LIST

/* The control block that is used for strong and weak pointers for all base types */
typedef struct {
    size_t nstrong;
    size_t nweak;
    void (*destructor)(void*);
} shared_ptr_ctrlblk;

/* @brief:      Defines the necessary structures and unions for a the list of weak and 
 *              strong pointer types
 *                  - T##_smrtptr_strong: Clonable owning pointer
 *                  - T##_smrtptr_weak: Clonable non-owning pointer (can be promoted)
 *                  - T##_smrtptr_option: Union containing a weak or strong pointer 
 *                      - Used for generic return from cloning funcions
 * @param:      [T] - the type to derive a strong pointer type for 
 * @param:      [free_fn] (UNUSED): ~function pointer to the function responsible for freeing the memory
 *                      must be of type void (*)(void*)~
 */
#define SHARED_PTR_DERIVE(T, free_fn)   \
    typedef struct {                    \
        T *ptr;                         \
        shared_ptr_ctrlblk *ctrl;       \
    } T##_smrtptr_strong;                   \
    \
    typedef struct {                    \
        T* _ptr;                        \
        shared_ptr_ctrlblk* ctrl;       \
    } T##_smrtptr_weak;                     \

SHARED_PTR_TYPE_LIST
#undef SHARED_PTR_DERIVE

/* Used internally for generic ptr types */
typedef struct {
    void* ptr;
    shared_ptr_ctrlblk* ctrl;
} _generic_shared_ptr;

#define SHARED_PTR_DERIVE(T, unused) T##_smrtptr_strong _is_##T;
union smrtptr_strong_types {
    SHARED_PTR_TYPE_LIST
    _generic_shared_ptr generic;
};
#undef SHARED_PTR_DERIVE

#define SHARED_PTR_DERIVE(T, unused) T##_smrtptr_weak _is_##T;
union smrtptr_weak_types {
    SHARED_PTR_TYPE_LIST
    _generic_shared_ptr generic;
};
#undef SHARED_PTR_DERIVE

typedef union {
    union smrtptr_strong_types IS_SMRTPTR_STRONG;
    union smrtptr_weak_types IS_SMRTPTR_WEAK;
} shared_ptr_option;

/* @brief:      Makes a strong pointer from an allocated void pointer and stores the specified deallocator 
 *              in the control block for deletion when fully out of scope 
 * @param:      void* alloc:    the pointer to *freshly* allocated memory 
 * @param:      void (*dealloc)(void*): function pointer that will be called when the pointer goes
 *              out of scope and gets freed 
 */
[[nodiscard]] static union smrtptr_strong_types _smrtptr_make_strong(void *alloc, void (*dealloc)(void*)) {
    if (alloc == NULL) {
        smrtptr_errno |= SMRTPTR_MAKE_RECIEVED_NULL;
        return (union smrtptr_strong_types){0};
    }
    shared_ptr_ctrlblk *temp_ctrl = malloc(sizeof(shared_ptr_ctrlblk));
    *temp_ctrl = (shared_ptr_ctrlblk){
        .nstrong = 1,
        .nweak = 1,
        .destructor = dealloc
    };
    _generic_shared_ptr generic_ptr = {
        .ptr = alloc,
        .ctrl = temp_ctrl,
    };
    return (union smrtptr_strong_types)generic_ptr;
}

/* @brief:      clones a strong pointer into a weak or strong pointer 
 * @param:      */
[[nodiscard]] static shared_ptr_option _smrtptr_clone_strong(
    union smrtptr_strong_types ptr,
    enum smrtptr_types type
) {
    if(ptr.generic.ctrl == NULL) {
        smrtptr_errno |= SMRTPTR_NULL_CTRL_BLOCK;
        return (shared_ptr_option){0};
    }
    switch (type) {
        case SMRTPTR_STRONG: {
            ptr.generic.ctrl->nstrong++;
            break;
        }
        case SMRTPTR_WEAK: {
            ptr.generic.ctrl->nweak++;
            break;
        }
        default: {
            smrtptr_errno |= SMRTPTR_INVALID_TYPE;
            return (shared_ptr_option){0};
        }
    }
    return (shared_ptr_option)ptr;
}

static void smrtptr_free_strong(void* ptr) {
    _generic_shared_ptr* _ptr = (_generic_shared_ptr*)ptr;
    if(_ptr->ctrl == NULL) {
        smrtptr_errno |= SMRTPTR_NULL_CTRL_BLOCK;
        return;
    }
    if(--_ptr->ctrl->nstrong == 0) {
        _ptr->ctrl->destructor(_ptr->ptr);
        if(--_ptr->ctrl->nweak == 0) free(_ptr->ctrl);
    }
}

/* Weak pointers (are implemented for any type that strong ptr is implemented for) */
[[nodiscard]] shared_ptr_option _smrtptr_clone_weak(
    union smrtptr_weak_types ptr,
    enum smrtptr_types type
) {
    switch(type){
        case SMRTPTR_WEAK: {
            ptr.generic.ctrl->nweak++;
            return (shared_ptr_option)ptr;
        }
        case SMRTPTR_STRONG: {
            ptr.generic.ctrl->nstrong++;
            return (shared_ptr_option)ptr;
        }
        default: {
            smrtptr_errno |= SMRTPTR_INVALID_TYPE;
            return (shared_ptr_option){0};
        }
    }
}

static void smrtptr_free_weak(void* ptr) {
    /* This type points to void and has no strong or weak association
     * this allows for any type to be implemented */
    _generic_shared_ptr* _ptr = ptr;
    /* we don't check the strong count because nweak only becomes 0
     * once all strong pointers are gone due to strong pointers implicitly
     * having a weak reference */
    if(--_ptr->ctrl->nweak == 0) free(_ptr->ctrl);
}

#undef SHARED_PTR_DERIVE

#define smrtptr_strong(T) __attribute__(( cleanup(smrtptr_free_strong) )) T##_smrtptr_strong
#define smrtptr_weak(T) __attribute__(( cleanup(smrtptr_free_weak) )) T##_smrtptr_weak
#define smrtptr_make_strong(T, alloc, dealloc)  _smrtptr_make_strong(alloc, dealloc)._is_##T 
#define smrtptr_clone_strong(T, ptr, type) _smrtptr_clone_strong((union smrtptr_strong_types)ptr, type).IS_##type._is_##T
#define smrtptr_clone_weak(T, ptr, type) _smrtptr_clone_weak((union smrtptr_weak_types)ptr, type).IS_##type._is_##T
#define deref_smrtptr(smrtptr) *smrtptr.ptr
#define ref_smrtptr(smrtptr) smrtptr.ptr
#define is_ptr_alive(smrtptr) !!(( smrtptr ).ctrl->nstrong)
#endif // }}}

/***************************** ATOMIC SHARED PTRS *****************************/// {{{

#ifdef SMRTPTR_SHARED_ATOMIC_TYPE_LIST
/* Thread safe implementation of shared and weak pointers using atomic reference counting */
#include <stdatomic.h>

/* The control block that is used for strong and weak pointers for all base types */
typedef struct {
    atomic_size_t nstrong;
    atomic_size_t nweak;
    void (*destructor)(void*);
} atomic_shared_ptr_ctrlblk;

/* @brief:      Defines the necessary structures and unions for a the list of weak and 
 *              strong pointer types
 *                  - T##_smrtptr_strong: Clonable owning pointer
 *                  - T##_smrtptr_weak: Clonable non-owning pointer (can be promoted)
 *                  - T##_smrtptr_option: Union containing a weak or strong pointer 
 *                      - Used for generic return from cloning funcions
 * @param:      [T] - the type to derive a strong pointer type for 
 * @param:      [free_fn] (UNUSED): ~function pointer to the function responsible for freeing the memory
 *                      must be of type void (*)(void*)~
 */
#define SMRTPTR_DERIVE_SHARED_ATOMIC(T, free_fn)   \
    typedef struct {                    \
        T *ptr;                         \
        atomic_shared_ptr_ctrlblk *ctrl;       \
    } T##_smrtptr_strong_atomic;        \
    \
    typedef struct {                    \
        T* _ptr;                        \
        atomic_shared_ptr_ctrlblk* ctrl;       \
    } T##_smrtptr_weak_atomic;                     \

SMRTPTR_SHARED_ATOMIC_TYPE_LIST
#undef SMRTPTR_DERIVE_SHARED_ATOMIC

/* Used internally for generic ptr types */
typedef struct {
    void* ptr;
    atomic_shared_ptr_ctrlblk* ctrl;
} _generic_atomic_shared_ptr;

#define SMRTPTR_DERIVE_SHARED_ATOMIC(T, unused) T##_smrtptr_strong_atomic _is_##T;
union smrtptr_strong_atomic_types {
    SMRTPTR_SHARED_ATOMIC_TYPE_LIST
    _generic_atomic_shared_ptr generic;
};
#undef SMRTPTR_DERIVE_SHARED_ATOMIC

#define SMRTPTR_DERIVE_SHARED_ATOMIC(T, unused) T##_smrtptr_weak_atomic _is_##T;
union smrtptr_weak_atomic_types {
    SMRTPTR_SHARED_ATOMIC_TYPE_LIST
    _generic_atomic_shared_ptr generic;
};
#undef SMRTPTR_DERIVE_SHARED_ATOMIC

typedef union {
    union smrtptr_strong_atomic_types IS_SMRTPTR_STRONG_ATOMIC;
    union smrtptr_weak_atomic_types IS_SMRTPTR_WEAK_ATOMIC;
} atomic_shared_ptr_option;

/* @brief:      Makes a strong pointer from an allocated void pointer and deallocates using the
 *              specified deallocator when fully out of scope 
 * @param:      void* alloc:    the pointer to *freshly* allocated memory 
 * @param:      void (*dealloc)(void*): function pointer that will be called when the pointer goes
 *              out of scope and gets freed 
 */
[[nodiscard]] __attribute__((unused))
static union smrtptr_strong_atomic_types _smrtptr_make_strong_atomic(void *alloc, void (*dealloc)(void*)) {
    if (alloc == NULL) {
        smrtptr_errno |= SMRTPTR_MAKE_RECIEVED_NULL;
        return (union smrtptr_strong_atomic_types){0};
    }
    atomic_shared_ptr_ctrlblk* temp_ctrl = malloc(sizeof(shared_ptr_ctrlblk));
    atomic_init(&temp_ctrl->nstrong, 1);
    atomic_init(&temp_ctrl->nweak, 1);
    temp_ctrl->destructor = dealloc;
    _generic_atomic_shared_ptr generic_ptr = {
        .ptr = alloc,
        .ctrl = temp_ctrl,
    };
    return (union smrtptr_strong_atomic_types)generic_ptr;
}

/* @brief:      clones a strong atomic pointer into a weak or strong atomic pointer 
 * @param:      union smrtptr_strong_atomic_types ptr: Union containing all the smrtptr types
 *              implemented for atomic reference counting 
 * @param:      enum smrtptr_types type: the type of smrtptr (WEAK or STRONG)
 */
[[nodiscard]] __attribute__((unused)) 
static atomic_shared_ptr_option _smrtptr_clone_strong_atomic(
    union smrtptr_strong_atomic_types ptr,
    enum smrtptr_types type
) {
    if(ptr.generic.ctrl == NULL) {
        smrtptr_errno |= SMRTPTR_NULL_CTRL_BLOCK;
        return (atomic_shared_ptr_option){0};
    }
    switch (type) {
        /* ++ and -- operators are not suitable for atomic reference counting */
        case SMRTPTR_STRONG_ATOMIC: {
            atomic_fetch_add_explicit(&ptr.generic.ctrl->nstrong, 1, memory_order_relaxed);
            break;
        }
        case SMRTPTR_WEAK_ATOMIC: {
            atomic_fetch_add_explicit(&ptr.generic.ctrl->nweak, 1, memory_order_relaxed);
            break;
        }
        default: {
            smrtptr_errno |= SMRTPTR_INVALID_TYPE;
            return (atomic_shared_ptr_option){0};
        }
    }
    return (atomic_shared_ptr_option)ptr;
}

__attribute__((unused)) 
static void smrtptr_free_strong_atomic(void* ptr) {
    _generic_atomic_shared_ptr* _ptr = (_generic_atomic_shared_ptr*)ptr;
    if(_ptr->ctrl == NULL) {
        smrtptr_errno |= SMRTPTR_NULL_CTRL_BLOCK;
        return;
    }
    /* atomic_fetch_sub_explicit returns the old value. If we subtracted 1 and old value is
     * 1 then count must be 0 */
    if(atomic_fetch_sub_explicit(&_ptr->ctrl->nstrong, 1, memory_order_release) == 1) {
        atomic_thread_fence(memory_order_acquire);
        _ptr->ctrl->destructor(_ptr->ptr);
        if(atomic_fetch_sub_explicit(&_ptr->ctrl->nweak, 1, memory_order_release) == 1) {
            atomic_thread_fence(memory_order_acquire);
            free(_ptr->ctrl);
        }
    }
}

/* Weak pointers (are implemented for any type that strong ptr is implemented for) */
[[nodiscard]] atomic_shared_ptr_option _smrtptr_clone_weak_atomic(
    union smrtptr_weak_atomic_types ptr,
    enum smrtptr_types type
) {
    switch(type){
        case SMRTPTR_WEAK_ATOMIC: {
            ptr.generic.ctrl->nweak++;
            return (atomic_shared_ptr_option)ptr;
        }
        case SMRTPTR_STRONG_ATOMIC: {
             ptr.generic.ctrl->nstrong++;
             return (atomic_shared_ptr_option)ptr;
        }
        default: { 
            smrtptr_errno |= SMRTPTR_INVALID_TYPE;
            return (atomic_shared_ptr_option){0};
        }
    }
}

__attribute__((unused)) static void smrtptr_free_weak_atomic(void* ptr) {
    _generic_atomic_shared_ptr* _ptr = ptr;
    if(atomic_fetch_sub_explicit(&_ptr->ctrl->nweak, 1, memory_order_release) == 1) {
        atomic_thread_fence(memory_order_acquire);
        free(_ptr->ctrl);
    }
}

__attribute__((unused))
static union smrtptr_strong_atomic_types _smrtptr_lock_weak_atomic(
    union smrtptr_weak_atomic_types ptr
) {
    size_t nrefs;
    do {
        nrefs = atomic_load_explicit(&ptr.generic.ctrl->nstrong, memory_order_acquire); 
        if(nrefs == 0) {
            /* Is dead */
            ptr.generic.ctrl = NULL;
            ptr.generic.ptr = NULL;
            break;
        }
    } while(!atomic_compare_exchange_weak_explicit(
        &ptr.generic.ctrl->nstrong,
        &nrefs,
        nrefs + 1,
        memory_order_acq_rel,
        memory_order_acquire
    ));
    return ((atomic_shared_ptr_option)ptr).IS_SMRTPTR_STRONG_ATOMIC;
}

#define smrtptr_strong_atomic(T) __attribute__(( cleanup(smrtptr_free_strong_atomic) )) T##_smrtptr_strong_atomic
#define smrtptr_weak_atomic(T) __attribute__(( cleanup(smrtptr_free_weak_atomic) )) T##_smrtptr_weak_atomic
#define smrtptr_make_strong_atomic(T, alloc, dealloc)  _smrtptr_make_strong_atomic(alloc, dealloc)._is_##T 
#define smrtptr_clone_strong_atomic(T, ptr, type) _smrtptr_clone_strong_atomic((union smrtptr_strong_atomic_types)ptr, type).IS_##type._is_##T
#define smrtptr_clone_weak_atomic(T, ptr, type) _smrtptr_clone_weak_atomic((union smrtptr_weak_atomic_types)ptr, type).IS_##type._is_##T
#define smrtptr_lock_weak_atomic(T, strong, weak) \
    (strong = _smrtptr_lock_weak_atomic((union smrtptr_weak_atomic_types)weak)._is_##T).ctrl != NULL

#endif // }}}

#endif
