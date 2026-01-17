#include <stdlib.h>

#ifdef SMRTPTR_TYPE_LIST

#define SMRTPTR_DERIVE(T) \
    typedef T* smrtptr_##T; \
    unsigned int _smrtptr_##T##_nrefs; \
    static void _create_smrtptr_##T(T** smrtptr) { \
       \ 
    }\
    static void _free_smrtptr_##T(T** smrtptr) {}

SMRTPTR_TYPE_LIST /* X-macro containing list of SMRTPTR_DERIVE calls */
#undef SMRTPTR_DERIVE

#define smrtptr(T) __attribute__((cleanup(free_smrtptr_##T))) smrtptr_##T

#endif

