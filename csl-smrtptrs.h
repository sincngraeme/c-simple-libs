#include <stdio.h>
#include <stdlib.h>

#ifdef SMRTPTR_IMPLEMENTATION

/* Helper macro to disable diagnostic warnings for ignored attributes 
 * (necissary for use of smrtptr types in file scope)
 */
#define SMRTPTR_DISABLE_WATTRIBUTES(code)               \
    _Pragma("GCC diagnostic push")                      \
    _Pragma("GCC diagnostic ignored \"-Wattributes\"")  \
    code                                                \
    _Pragma("GCC diagnostic pop")


enum smrtptr_types {
    UNIQUE_PTR,
    // SOLE_PTR,
    // ACCCESS_PTR,
    WEAK_PTR,
    SHARED_PTR,
    SHARED_PTR_ATOMIC,
};

enum smrtptr_errors {
    SMRTPTR_INVALID_TYPE = 1,
    SMRTPTR_ATTEMPTED_DIRECT_ASSIGNMENT,
    SMRTPTR_MAKE_RECIEVED_NULL,
    SMRTPTR_NULL_CTRL_BLOCK,
};

[[noreturn]] void smrtptr_log_error(enum smrtptr_errors code) {
    switch (code) {
        case SMRTPTR_ATTEMPTED_DIRECT_ASSIGNMENT: {
            fprintf(stderr, "ERROR: Attempted direct assignment of shared_ptr");
            exit(SMRTPTR_ATTEMPTED_DIRECT_ASSIGNMENT);
        }
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

#define UNIQUE_PTR_DERIVE(T, free_fn)           \
    typedef T* T##_unique_ptr;                  \
    void free_##T##_unique_ptr(void *smrtptr) { \
        void **_smrtptr = (void **)smrtptr;     \
        free_fn(*_smrtptr);                     \
    }

UNIQUE_PTR_TYPE_LIST 

/* @brief: Creates a unique_ptr */
#define unique_ptr(T) __attribute__(( cleanup(free_##T##_unique_ptr) )) T##_unique_ptr

#endif

#ifdef INIT_SOLE_PTR

unsigned int sole_ptr_nrefs = 0;

/* @brief:  Sole ptr can only be created once, and must correspond to one
 * address at a time. It is managed in global state and each reference does
 * not itself have ownership. DO NOT USE RAW POINTER REFERENCES.
 *      - Each new allocation requires the sole_ptr to go fully out of scope and
 *        be freed first
 */
void create_sole_ptr() { sole_ptr_nrefs++; }

/* @brief:  Frees a sole ptr once the global reference count has become 0 */
void free_sole_ptr(void* smrtptr) {
    void **_smrtptr = (void **)smrtptr;
    sole_ptr_nrefs--;
    if (sole_ptr_nrefs == 0) free(*_smrtptr);
}

/* @breif:  Defines a sole_ptr and increments the global counter */
#define sole_ptr(T)                                 \
    create_sole_ptr();                              \
    __attribute__((cleanup(free_sole_ptr))) T *

#endif

#ifdef ACCESS_PTR_REGISTRY

/* @brief:  Frees an access ptr once the reference count becomes 0 */
void free_access_ptr(void* smrtptr) {
    void** _smrtptr = (void**)smrtptr;
    sole_ptr_nrefs--;
    if(sole_ptr_nrefs == 0) free(*_smrtptr);
}

/* @brief:  Defines a sole_ptr and increments the global counter */
#define access_ptr(T, group_name)                               \
    create_##group_name##_access_ptr();                         \
    __attribute__((cleanup(free_##group_name##_access_ptr))) T*

/* Generate struct definition */
#define REGISTER_ACCESS_PTR(T, group_name) unsigned int group_name;

struct Access_ptr_registry {
    ACCESS_PTR_REGISTRY
} access_ptr_registry = {0};

#undef REGISTER_ACCESS_PTR

/* Generate function definitions */
#define REGISTER_ACCESS_PTR(T, group_name)                          \
    void create_##group_name##_access_ptr() {                       \
        access_ptr_registry.group_name++;                           \
    }                                                               \
    void free_##group_name##_access_ptr(T** smrtptr) {              \
        access_ptr_registry.group_name--;                           \
        if(access_ptr_registry.group_name == 0) free(*smrtptr);     \
    }

ACCESS_PTR_REGISTRY

#undef REGISTER_ACCESS_PTR
#endif

#ifdef SHARED_PTR_TYPE_LIST
/* True shared_ptrs like in C++ */
typedef struct {
    unsigned int nshared;
    unsigned int nweak;
    void (*destructor)(void*);
} shared_ptr_ctrlblk;

/* @brief:      Defines the necessary functions for each derived shared pointer type
 *                  - make: Create a shared pointer from a raw pointer 
 *                      (intended for use with malloc)
 *                  - clone: Increment the counter for the type of reference 
 *                      (does not allocate new space, only gives a new shared or 
 *                      weak reference)
 *                  - free: Delete the allocated memory if appropriate 
 *                      (uses the counters in the control block to decide)
 *                      - If the shared reference count (nshared) reaches 0, free the data
 *                      - If the weak reference count (nweak) reaches 0, free control block
 * @param:      T - the type to derive a shared pointer type for 
 */
#define SHARED_PTR_DERIVE(T, free_fn)                                       \
    typedef struct {                                                        \
        T *ptr;                                                             \
        shared_ptr_ctrlblk *ctrl;                                           \
    } T##_shared_ptr;                                                       \
    \
    [[nodiscard]] T##_shared_ptr make_##T##_shared_ptr(void *ptr) {         \
        if (ptr == NULL) {                                                  \
            smrtptr_log_error(SMRTPTR_MAKE_RECIEVED_NULL);                  \
        }                                                                   \
        shared_ptr_ctrlblk *temp_ctrl = malloc(sizeof(shared_ptr_ctrlblk)); \
        *temp_ctrl = (shared_ptr_ctrlblk){                                  \
            .nshared = 1,                                                   \
            .nweak = 0,                                                     \
            .destructor = free_fn                                           \
        };                                                                  \
        return (T##_shared_ptr){                                            \
            .ptr = ptr,                                                     \
            .ctrl = temp_ctrl,                                              \
        };                                                                  \
    }                                                                       \
    \
    [[nodiscard]] T##_shared_ptr clone_##T##_shared_ptr(                    \
        T##_shared_ptr ptr,                                                 \
        enum smrtptr_types type                                             \
    ) {                                                                     \
        switch (type) {                                                     \
            case SHARED_PTR: {                                              \
                ptr.ctrl->nshared++;                                        \
                break;                                                      \
            }                                                               \
            case WEAK_PTR: {                                                \
                ptr.ctrl->nweak++;                                          \
                break;                                                      \
            }                                                               \
            default: {                                                      \
                smrtptr_log_error(SMRTPTR_INVALID_TYPE);                    \
            }                                                               \
        }                                                                   \
        return ptr;                                                         \
    }                                                                       \
    \
    void free_##T##_shared_ptr(T##_shared_ptr *ptr) {                       \
        if(ptr->ctrl == NULL) smrtptr_log_error(SMRTPTR_NULL_CTRL_BLOCK);   \
        ptr->ctrl->nshared--;                                               \
        if(ptr->ctrl->nshared == 0) ptr->ctrl->destructor(ptr->ptr);        \
    }

SHARED_PTR_TYPE_LIST


/* Weak pointers (are implemented for any type that shared ptr is implemented for) */
#undef SHARED_PTR_DERIVE
#define SHARED_PTR_DERIVE(T, unused)                                            \
    typedef struct {                                                            \
        T* ptr;                                                                 \
        shared_ptr_ctrlblk* ctrl;                                               \
    } T##_weak_ptr;                                                             \
\
    [[nodiscard]] T##_weak_ptr clone_##T##_weak_ptr(T##_weak_ptr ptr) {         \
        ptr.ctrl->nweak++;                                                      \
        return ptr;                                                             \
    }                                                                           \
\
    [[nodiscard]] T##_shared_ptr access_##T##_weak_ptr(T##_weak_ptr ptr) {      \
        if(ptr.ctrl->nshared > 0) {                                             \
            ptr.ctrl->nshared++;                                                \
            return (T##_shared_ptr){ .ptr = ptr.ptr, .ctrl = ptr.ctrl };        \
        }                                                                       \
        return (T##_shared_ptr){0};                                             \
    }                                                                           \
\
    void free_##T##_weak_ptr(T##_weak_ptr* ptr) {                               \
        ptr->ctrl->nweak--;                                                     \
        if(ptr->ctrl->nshared == 0 && ptr->ctrl->nweak == 0) free(ptr->ctrl);   \
    }

SHARED_PTR_TYPE_LIST

#undef SHARED_PTR_DERIVE
#define SHARED_PTR_DERIVE(T, unused) , T##_shared_ptr : clone_##T##_shared_ptr

#define shared_ptr(T) __attribute__((cleanup(free_##T##_shared_ptr))) T##_shared_ptr
#define clone_shared_ptr(ptr, type) _Generic(ptr SHARED_PTR_TYPE_LIST)(ptr, type)
#define make_shared_ptr(T, ptr)  make_##T##_shared_ptr(ptr) 
#define deref_shared_ptr(smrtptr) *smrtptr.ptr
#define shared_ptr_raw(smrtptr) smrtptr.ptr

#endif
#endif
