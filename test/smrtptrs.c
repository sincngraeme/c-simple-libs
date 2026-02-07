#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define SMRTPTR_IMPLEMENTATION

#define SMRTPTR_UNIQUE_TYPE_LIST \
    SMRTPTR_DERIVE_UNIQUE(int, free) \
    SMRTPTR_DERIVE_UNIQUE(FILE, close_file)

#define SHARED_PTR_TYPE_LIST \
    SHARED_PTR_DERIVE(int, free)

#define SMRTPTR_SHARED_ATOMIC_TYPE_LIST \
    SMRTPTR_DERIVE_SHARED_ATOMIC(int, free)

void close_file(void* void_fp) {
    FILE* fp = void_fp;
    if(fclose(fp) == EOF) {
        fprintf(stderr, "ERROR: Failed to close file\n");
        exit(EOF);
    }
}

#include "../csl-smrtptrs.h"

int main() {
    {
        smrtptr_unique(int) ptr1 = smrtptr_make_unique(int, malloc(sizeof(int)), free);
        deref_smrtptr(ptr1) = 1;
        printf("ptr1: %d\n", *ptr1.ptr);
        smrtptr_unique(int) _ptr1 = smrtptr_move_unique(int, &ptr1);
        assert(ptr1.ptr == NULL);
        assert(deref_smrtptr(_ptr1) == 1);
    }
    {
        // Shared ptr
        smrtptr_strong(int) ptr2 = smrtptr_make_strong(int, malloc(sizeof(int)), free);
        if(smrtptr_errno) return smrtptr_errno;
        deref_smrtptr(ptr2) = 10;
        printf("ptr2: %d\n", deref_smrtptr(ptr2));
        {
            smrtptr_strong(int) ptr3 = smrtptr_clone_strong(int, ptr2, SMRTPTR_STRONG);
            if(smrtptr_errno) return smrtptr_errno;
            { /* Create a smrtptr_weak */
                smrtptr_weak(int) ptr4 = smrtptr_clone_strong(int, ptr3, SMRTPTR_WEAK);
                if(smrtptr_errno) return smrtptr_errno;
                smrtptr_weak(int) ptr5 = smrtptr_clone_weak(int, ptr4, SMRTPTR_WEAK);
                if(smrtptr_errno) return smrtptr_errno;
                { 
                    smrtptr_strong(int) ptr6;
                    if( is_ptr_alive(ptr6 = smrtptr_clone_weak(int, ptr4, SMRTPTR_STRONG)) ) {
                        printf("ptr6: %d\n", deref_smrtptr(ptr6)); /* Reading */
                    }
                }
            }
            printf("ptr2: %d\n", deref_smrtptr(ptr2));
        }
    }
    {
        // Atomic Shared ptrs
        smrtptr_strong_atomic(int) ptr7 = smrtptr_make_strong_atomic(int, malloc(sizeof(int)), free);
        if(smrtptr_errno) return smrtptr_errno;
        deref_smrtptr(ptr7) = 10;
        printf("ptr7: %d\n", deref_smrtptr(ptr7));
        {
            smrtptr_strong_atomic(int) ptr8 = smrtptr_clone_strong_atomic(int, ptr7, SMRTPTR_STRONG_ATOMIC);
            if(smrtptr_errno) return smrtptr_errno;
            { /* Create a smrtptr_weak */
                smrtptr_weak_atomic(int) ptr9 = smrtptr_clone_strong_atomic(int, ptr8, SMRTPTR_WEAK_ATOMIC);
                if(smrtptr_errno) return smrtptr_errno;
                smrtptr_weak_atomic(int) ptr10 = smrtptr_clone_weak_atomic(int, ptr9, SMRTPTR_WEAK_ATOMIC);
                if(smrtptr_errno) return smrtptr_errno;
                { 
                    smrtptr_strong_atomic(int) ptr11;
                    if( smrtptr_lock_weak_atomic(int, ptr11, ptr9) ) {
                        printf("ptr11: %d\n", deref_smrtptr(ptr11)); /* Reading */
                    }
                }
                { 
                    smrtptr_strong_atomic(int) ptr12;
                    if( smrtptr_lock_weak_atomic(int, ptr12, ptr10)) {
                        printf("ptr12: %d\n", deref_smrtptr(ptr12)); /* Reading */
                    }
                }
            }
            printf("ptr7: %d\n", deref_smrtptr(ptr7));
        }
    }
    {
        // File IO
        smrtptr_unique(FILE) fp = smrtptr_make_unique(FILE, fopen("test.txt", "a"), close_file);
        if(smrtptr_errno) return smrtptr_errno;
        if(fp.ptr == NULL) {
            fprintf(stderr, "ERROR: Failed to open file\n");
            exit(-1);
        } else {
            printf("File successfully opened!\n");
            if(fputs("Hello there!\n", fp.ptr) == 0) {
                fprintf(stderr, "ERROR: Failed to write to file\n");
                return -1;
            };
            printf("File successfully written to!\n");
        }
    }
    {
        // File IO
        smrtptr_unique(FILE) fp = smrtptr_make_unique(FILE, fopen("test.txt", "r"), close_file);
        if(smrtptr_errno) return smrtptr_errno;
        char buf[256] = {0};
        if(fp.ptr == NULL) {
            fprintf(stderr, "ERROR: Failed to open file\n");
            exit(-1);
        } else {
            printf("File successfully opened!\n");
            for(int i = 0; fgets(buf, 256, fp.ptr) != NULL; i++) printf("%d | %s", i, buf);
            printf("File successfully read from!\n");
        }
    }
    // {
    //     smrtptr_strong_atomic(int) atom_ptr = smrtptr_make_strong_atomic(int, malloc(sizeof(int)), free);
    //     deref_smrtptr(atom_ptr) = 10;
    //     printf("Shared atomic 1: %d\n", deref_smrtptr(atom_ptr));
    //     smrtptr_strong_atomic(int) atom_ptr2 = smrtptr_clone_strong_atomic(int, atom_ptr, SMRTPTR_STRONG_ATOMIC);
    //     printf("Shared atomic 2: %d\n", deref_smrtptr(atom_ptr));
    // }
    printf("Hello There!\n");
    return 0;
}
