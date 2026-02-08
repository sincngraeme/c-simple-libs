/* vim: set ft=c : */

/********************************************************************************
* Name:         csl-smrtptrs.h                                                  *
* Description:  provides several smart pointer types for automation of memory   *
*               frees and/or object lifetime                                    *
* By:           Nigel Sinclair                                                  *
* Github:       https://github.com/sincngraeme                                  *
********************************************************************************/

/*
 * TODO: Move functions for strong pointers
 * TODO: Move functions for weak pointers
 * TODO: Move functions for strong atomic pointers
 * TODO: Move functions for weak atomic pointers
 * TODO: relay pointer (single-owner, shared-access)
 */

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
    SMRTPTR_RELAY,
    SMRTPTR_RELAY_ATOMIC,
};

enum smrtptr_errors {
    SMRTPTR_NOERR               = 0b0000000000000,
    SMRTPTR_INVALID_TYPE        = 0b0000000000001,
    SMRTPTR_MAKE_RECIEVED_NULL  = 0b0000000000010,
    SMRTPTR_NULL_CTRL_BLOCK     = 0b0000000000100,
    SMRTPTR_FREE_RECIEVED_NULL  = 0b0000000001000,
    SMRTPTR_NULL_DESTRUCTOR     = 0b0000000010000,
    SMRTPTR_IS_ALREADY_DEAD     = 0b0000000100000,
    SMRTPTR_PASS_RECIEVED_NULL  = 0b0000001000000,
    SMRTPTR_SRC_DOES_NOT_OWN    = 0b0000010000000,
    SMRTPTR_DEST_HAS_OWNERSHIP  = 0b0000100000000,
    SMRTPTR_DEST_UNINITIALIZED  = 0b0001000000000,
    SMRTPTR_DEST_PTR_NE_SRC     = 0b0010000000000,
    SMRTPTR_NULL_REFCOUNT       = 0b0100000000000,
    SMRTPTR_MALLOC_FAILED       = 0b1000000000000,
};

static enum smrtptr_errors smrtptr_errno = 0;

/* For optionally removing attributes */
#ifndef smrtptr_attribute
#define smrtptr_attribute(attr) __attribute__((attr))
#endif

#define deref_smrtptr(smrtptr) *smrtptr.ptr
#define ref_smrtptr(smrtptr) smrtptr.ptr


/****************************** UNIQUE POINTERS *******************************/// {{{

#ifdef SMRTPTR_UNIQUE_TYPE_LIST

typedef struct {
    void* ptr;
    void (*destructor)(void*);
} _generic_smrtptr_unique;

#define SMRTPTR_DERIVE_UNIQUE(T)        \
    typedef struct {                \
        T* ptr;                     \
        void (*destructor)(void*);  \
    } T##_smrtptr_unique;

SMRTPTR_UNIQUE_TYPE_LIST
#undef SMRTPTR_DERIVE_UNIQUE

#define SMRTPTR_DERIVE_UNIQUE(T) T##_smrtptr_unique T##_member;

/* Generate a union which contains all unique ptr types. These will differ
 * only in the base type they contain */
union smrtptr_unique_types {
    SMRTPTR_UNIQUE_TYPE_LIST
    _generic_smrtptr_unique generic;   // All types will be processed as this type (void*)
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
    _generic_smrtptr_unique smrtptr = { .ptr = NULL, .destructor = NULL };
    if(alloc != NULL && dealloc != NULL) {
        smrtptr.ptr = alloc;
        smrtptr.destructor = dealloc;
    }
    return (union smrtptr_unique_types)smrtptr;
}

/* Moves the data from a unique pointer to another unique pointer */
smrtptr_attribute(unused)
static union smrtptr_unique_types _smrtptr_move_unique(union smrtptr_unique_types* src) {
    _generic_smrtptr_unique dest = { .ptr = src->generic.ptr, .destructor = src->generic.destructor };
    src->generic.ptr = NULL;
    src->generic.destructor = NULL;
    return (union smrtptr_unique_types)dest;
}

/* @brief: smrtptr_unique type macro */
#define smrtptr_unique(T) smrtptr_attribute( cleanup(smrtptr_free_unique) ) T##_smrtptr_unique
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
#define SHARED_PTR_DERIVE(T)   \
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

#define SHARED_PTR_DERIVE(T) T##_smrtptr_strong T##_field;
union smrtptr_strong_types {
    SHARED_PTR_TYPE_LIST
    _generic_shared_ptr generic;
};
#undef SHARED_PTR_DERIVE

#define SHARED_PTR_DERIVE(T) T##_smrtptr_weak T##_field;
union smrtptr_weak_types {
    SHARED_PTR_TYPE_LIST
    _generic_shared_ptr generic;
};
#undef SHARED_PTR_DERIVE

typedef union {
    union smrtptr_strong_types SMRTPTR_STRONG_FIELD;
    union smrtptr_weak_types SMRTPTR_WEAK_FIELD;
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

/* @brief:      copies a strong pointer into a weak or strong pointer
 * @param:      */
[[nodiscard]] static shared_ptr_option _smrtptr_copy_strong(
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
[[nodiscard]] shared_ptr_option _smrtptr_copy_weak(
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

#define smrtptr_strong(T) smrtptr_attribute( cleanup(smrtptr_free_strong) ) T##_smrtptr_strong
#define smrtptr_weak(T) smrtptr_attribute( cleanup(smrtptr_free_weak) ) T##_smrtptr_weak
#define smrtptr_make_strong(T, alloc, dealloc)  \
    _smrtptr_make_strong(alloc, dealloc).T##_field
#define smrtptr_copy_strong(T, ptr, type) \
    _smrtptr_copy_strong((union smrtptr_strong_types)ptr, type).type##_FIELD.T##_field
#define smrtptr_copy_weak(T, ptr, type) \
    _smrtptr_copy_weak((union smrtptr_weak_types)ptr, type).type##_FIELD.T##_field
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
#define SMRTPTR_DERIVE_SHARED_ATOMIC(T)   \
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

#define SMRTPTR_DERIVE_SHARED_ATOMIC(T) T##_smrtptr_strong_atomic T##_field;
union smrtptr_strong_atomic_types {
    SMRTPTR_SHARED_ATOMIC_TYPE_LIST
    _generic_atomic_shared_ptr generic;
};
#undef SMRTPTR_DERIVE_SHARED_ATOMIC

#define SMRTPTR_DERIVE_SHARED_ATOMIC(T) T##_smrtptr_weak_atomic T##_field;
union smrtptr_weak_atomic_types {
    SMRTPTR_SHARED_ATOMIC_TYPE_LIST
    _generic_atomic_shared_ptr generic;
};
#undef SMRTPTR_DERIVE_SHARED_ATOMIC

typedef union {
    union smrtptr_strong_atomic_types SMRTPTR_STRONG_ATOMIC_FIELD;
    union smrtptr_weak_atomic_types SMRTPTR_WEAK_ATOMIC_FIELD;
} atomic_shared_ptr_option;

/* @brief:      Makes a strong pointer from an allocated void pointer and deallocates using the
 *              specified deallocator when fully out of scope
 * @param:      void* alloc:    the pointer to *freshly* allocated memory
 * @param:      void (*dealloc)(void*): function pointer that will be called when the pointer goes
 *              out of scope and gets freed
 */
[[nodiscard]] smrtptr_attribute(unused)
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

/* @brief:      copys a strong atomic pointer into a weak or strong atomic pointer
 * @param:      union smrtptr_strong_atomic_types ptr: Union containing all the smrtptr types
 *              implemented for atomic reference counting
 * @param:      enum smrtptr_types type: the type of smrtptr (WEAK or STRONG)
 */
[[nodiscard]] smrtptr_attribute(unused)
static atomic_shared_ptr_option _smrtptr_copy_strong_atomic(
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

smrtptr_attribute(unused)
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
[[nodiscard]] atomic_shared_ptr_option _smrtptr_copy_weak_atomic(
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

smrtptr_attribute(unused) static void smrtptr_free_weak_atomic(void* ptr) {
    _generic_atomic_shared_ptr* _ptr = ptr;
    if(atomic_fetch_sub_explicit(&_ptr->ctrl->nweak, 1, memory_order_release) == 1) {
        atomic_thread_fence(memory_order_acquire);
        free(_ptr->ctrl);
    }
}

smrtptr_attribute(unused)
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
    return ((atomic_shared_ptr_option)ptr).SMRTPTR_STRONG_ATOMIC_FIELD;
}

#define smrtptr_strong_atomic(T) \
    smrtptr_attribute( cleanup(smrtptr_free_strong_atomic) ) T##_smrtptr_strong_atomic
#define smrtptr_weak_atomic(T) \
    smrtptr_attribute( cleanup(smrtptr_free_weak_atomic) ) T##_smrtptr_weak_atomic
#define smrtptr_make_strong_atomic(T, alloc, dealloc) \
    _smrtptr_make_strong_atomic(alloc, dealloc).T##_field
#define smrtptr_copy_strong_atomic(T, ptr, type) \
    _smrtptr_copy_strong_atomic((union smrtptr_strong_atomic_types)ptr, type).type##_FIELD.T##_field
#define smrtptr_copy_weak_atomic(T, ptr, type) \
    _smrtptr_copy_weak_atomic((union smrtptr_weak_atomic_types)ptr, type).type##_FIELD.T##_field
#define smrtptr_lock_weak_atomic(T, strong, weak) \
    (strong = _smrtptr_lock_weak_atomic((union smrtptr_weak_atomic_types)weak).T##_field).ctrl != NULL

#endif // }}}

/******************************* RELAY POINTERS ********************************/// {{{

#ifdef SMRTPTR_RELAY_TYPE_LIST


typedef struct {
    void* ptr;
    void (*destructor)(void*);
    size_t* nrefs;
} _generic_smrtptr_relay;

#define SMRTPTR_DERIVE_RELAY(T)     \
    typedef struct {                \
        T* ptr;                     \
        void (*destructor)(void*);  \
        size_t* nrefs;              \
    } T##_smrtptr_relay;

SMRTPTR_RELAY_TYPE_LIST
#undef SMRTPTR_DERIVE_RELAY

#define SMRTPTR_DERIVE_RELAY(T) T##_smrtptr_relay T##_field;

/* Generate a union which contains all relay ptr types. These will differ
 * only in the base type they contain */
union smrtptr_relay_types {
    SMRTPTR_RELAY_TYPE_LIST
    _generic_smrtptr_relay generic;   // All types will be processed as this type (void*)
};

#undef SMRTPTR_DERIVE_RELAY

/* This takes in a void ptr so it can be compatible with all smrtptr types
 * We then cast the ptr to the union which contains all types and */
static void smrtptr_free_relay(void* smrtptr) {
    if(smrtptr == NULL) {
        smrtptr_errno |= SMRTPTR_FREE_RECIEVED_NULL;
        return;
    }
    union smrtptr_relay_types _smrtptr = *(union smrtptr_relay_types*)smrtptr;
    if(_smrtptr.generic.nrefs == NULL) {
        smrtptr_errno |= SMRTPTR_NULL_REFCOUNT;
        return;
    }
    /* If the destructor is NULL we do not have ownership */
    if(_smrtptr.generic.destructor != NULL) {
        /* If we do have ownership then we free the ptr */
        _smrtptr.generic.destructor(_smrtptr.generic.ptr);
    }
    /* decrease the ref count, if it is then 0 then we free the refcount */
    if(--(*_smrtptr.generic.nrefs) == 0) {
        free(_smrtptr.generic.nrefs);
    }
}
/* Returns a new relay pointer to the allocated memory pointed to by alloc, to be freed
 * later with dealloc */
[[nodiscard]]
static union smrtptr_relay_types _smrtptr_make_relay(void* alloc, void (*dealloc)(void*)) {
    _generic_smrtptr_relay smrtptr = {0};
    if(alloc != NULL && dealloc != NULL) {
        smrtptr.ptr = alloc;
        smrtptr.destructor = dealloc;
        // We want this to be common to all instances
        if((smrtptr.nrefs = malloc(sizeof(size_t))) == NULL) {
            smrtptr_errno |= SMRTPTR_MALLOC_FAILED;
            return (union smrtptr_relay_types){0};
        }
        *smrtptr.nrefs = 1;
    }
    return (union smrtptr_relay_types)smrtptr;
}

/* Passes ownership from owning relay ptr to non-owning relay pointer. dest and src must point to the
 * same data and dest must be initialize via smrtptr_copy_relay() first. Does not change the refcount. */
[[nodiscard]] smrtptr_attribute(unused)
static enum smrtptr_errors _smrtptr_pass_relay(union smrtptr_relay_types* dest, union smrtptr_relay_types* src) {
    /* The destination must be initialized, and must not have ownership of
     * any data. The source must be initialized and have ownersip of its
     * data */
    if(src == NULL || dest == NULL) return SMRTPTR_PASS_RECIEVED_NULL;
    if(src->generic.nrefs == NULL) return SMRTPTR_NULL_REFCOUNT | SMRTPTR_DEST_UNINITIALIZED;
    if(src->generic.destructor == NULL) return SMRTPTR_SRC_DOES_NOT_OWN;
    if(dest->generic.destructor != NULL) return SMRTPTR_DEST_HAS_OWNERSHIP;
    /* Passing ownership to a pointer to different data throws off ref counts and
     * and results in UB */
    if(dest->generic.ptr != src->generic.ptr) return SMRTPTR_DEST_PTR_NE_SRC;

    *dest = *src;
    /* The original no longer has ownership */
    src->generic.destructor = NULL;
    return SMRTPTR_NOERR;
}

/* Copies a relay pointer. Does not care about ownership of source, 
 * destination must be non-owning */
[[nodiscard]]
static union smrtptr_relay_types _smrtptr_copy_relay(union smrtptr_relay_types ptr) {
    if(ptr.generic.nrefs == NULL) {
        smrtptr_errno |= SMRTPTR_NULL_REFCOUNT;
        return (union smrtptr_relay_types){0};
    }
    (*ptr.generic.nrefs)++;
    return (union smrtptr_relay_types)(
        (_generic_smrtptr_relay){
            .ptr = ptr.generic.ptr,
            .nrefs = ptr.generic.nrefs,
            // source may have ownersip or not, dest never has ownersip
            .destructor = NULL
        }
    );
}

/* Macro wrappers for functions to handle generic types */
#define smrtptr_relay(T) smrtptr_attribute( cleanup(smrtptr_free_relay) ) T##_smrtptr_relay
#define smrtptr_make_relay(T, alloc, dealloc) _smrtptr_make_relay(alloc, dealloc).T##_field
#define smrtptr_pass_relay(dest, src) \
    _smrtptr_pass_relay((union smrtptr_relay_types*)dest, (union smrtptr_relay_types*)src)
#define smrtptr_copy_relay(T, ptr) \
    _smrtptr_copy_relay((union smrtptr_relay_types)ptr).T##_field;
/* TODO: Reset function and macro wrapper */

#endif // }}}

#endif
