#include <stdio.h>
#include <stdlib.h>

#ifdef SMRTPTR_IMPLEMENTATION

enum smrtptr_types {
  // UNIQUE_PTR,
  // SOLE_PTR,
  // ACCCESS_PTR,
  WEAK_PTR,
  SHARED_PTR,
  // SHARED_PTR_ATOMIC,
};

enum smrtptr_errors {
  SMRTPTR_INVALID_TYPE = 1,
  SMRTPTR_ATTEMPTED_DIRECT_ASSIGNMENT,
  SMRTPTR_MAKE_RECIEVED_NULL,
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
        fprintf(
            stderr,
            "ERROR: Invalid smrtptr type. must be of type 'enum smrtptr_types'\n");
        exit(SMRTPTR_INVALID_TYPE);
      }
      default: {
        fprintf(stderr, "ERROR: Unknown code");
        exit(-1);
      }
  }
}

/* @brief:  This is called implicitly at the end of the scope and does not
 * require any reference management. do not assign to a raw ptr except as a
 * function argument */
void free_unique_ptr(void *smrtptr) {
  void **_smrtptr = (void **)smrtptr;
  free(*_smrtptr);
}
/* @brief: Creates a unique_ptr */
#define unique_ptr(T)                                           \
  typedef T *unique_ptr_##T;                                    \
  _Pragma("GCC diagnostic push")                                \
  _Pragma("GCC diagnostic ignored \"-Wignored-attributes\"")    \
  __attribute__((cleanup(free_unique_ptr))) unique_ptr_##T      \
  _Pragma("GCC diagnostic pop")

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
void free_sole_ptr(void *smrtptr) {
  void **_smrtptr = (void **)smrtptr;
  sole_ptr_nrefs--;
  if (sole_ptr_nrefs == 0)
    free(*_smrtptr);
}

/* @breif:  Defines a sole_ptr and increments the global counter */
#define sole_ptr(T)                                                            \
  create_sole_ptr();                                                           \
  __attribute__((cleanup(free_sole_ptr))) T *

#endif

#ifdef ACCESS_PTR_REGISTRY

/* @brief:  Frees an access ptr once the reference count becomes 0 */
void free_access_ptr(void *smrtptr) {
  void **_smrtptr = (void **)smrtptr;
  sole_ptr_nrefs--;
  if (sole_ptr_nrefs == 0)
    free(*_smrtptr);
}

/* @brief:  Defines a sole_ptr and increments the global counter */
#define access_ptr(T, group_name)                                              \
  create_##group_name##_access_ptr();                                          \
  __attribute__((cleanup(free_##group_name##_access_ptr))) T *

/* Generate struct definition */
#define REGISTER_ACCESS_PTR(T, group_name) unsigned int group_name;

struct Access_ptr_registry {
  ACCESS_PTR_REGISTRY
} access_ptr_registry = {0};

#undef REGISTER_ACCESS_PTR

/* Generate function definitions */
#define REGISTER_ACCESS_PTR(T, group_name)                                     \
  void create_##group_name##_access_ptr() {                                    \
    access_ptr_registry.group_name++;                                          \
  }                                                                            \
  void free_##group_name##_access_ptr(T **smrtptr) {                           \
    access_ptr_registry.group_name--;                                          \
    if (access_ptr_registry.group_name == 0)                                   \
      free(*smrtptr);                                                          \
  }

ACCESS_PTR_REGISTRY

#undef REGISTER_ACCESS_PTR
#endif

#ifdef SHARED_PTR_TYPE_LIST
/* True shared_ptrs like in C++ */
typedef struct {
  unsigned int nshared;
  unsigned int nweak;
} shared_ptr_ctrlblk;

#define SHARED_PTR_DERIVE(T)                                            \
  typedef struct {                                                      \
    T *ptr;                                                             \
    enum smrtptr_types type;                                            \
    shared_ptr_ctrlblk *ctrl;                                           \
  } T##_shared_ptr;                                                     \
  [[nodiscard]] T##_shared_ptr make_shared_ptr(void *ptr) {             \
    if (ptr == NULL) {                                                  \
      smrtptr_log_error(SMRTPTR_MAKE_RECIEVED_NULL);                    \
    }                                                                   \
    shared_ptr_ctrlblk *temp_ctrl = malloc(sizeof(shared_ptr_ctrlblk)); \
    *temp_ctrl = (shared_ptr_ctrlblk){.nshared = 1, .nweak = 1};        \
    return (T##_shared_ptr){.ptr = ptr, .ctrl = temp_ctrl};             \
  }                                                                     \
  T##_shared_ptr clone_##T##_shared_ptr(T##_shared_ptr ptr,             \
                                        enum smrtptr_types type) {      \
    switch (type) {                                                     \
    case SHARED_PTR: {                                                  \
      ptr.ctrl->nshared++;                                              \
    }                                                                   \
    case WEAK_PTR: {                                                    \
      ptr.ctrl->nweak++;                                                \
    }                                                                   \
    default: {                                                          \
      smrtptr_log_error(SMRTPTR_INVALID_TYPE);                          \
    }                                                                   \
    }                                                                   \
    return ptr;                                                         \
  }                                                                     \
  void free_##T##_shared_ptr(T##_shared_ptr *ptr) {                     \
    ptr->
  }

SHARED_PTR_TYPE_LIST

#define shared_ptr(T)                                               \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wignored-attributes\"")      \
    __attribute__((cleanup(free_##T##_shared_ptr))) T##_shared_ptr \
    _Pragma("GCC diagnostic pop")

#undef SHARED_PTR_DERIVE
#define SHARED_PTR_DERIVE(T) , T : clone_##T##_shared_ptr

#define clone_shared_ptr(ptr) _Generic(typeof(ptr) SHARED_PTR_TYPE_LIST)(ptr)

#endif

#endif
