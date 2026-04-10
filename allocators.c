/* Name:        arenas.c
 * Description: Arena implementations
 * By:          Nigel Sinclair
 * Github:      https://github.com/sincngraeme
 */

#include "allocators.h"

/* Scratch arenas allocate a given number of bytes each time, each pointer 
 * having the same lifetime as the arena. The only way to deallocate is to 
 * reset or delete the arena
 */
static void* ScratchAllocator_alloc(ScratchAllocatorData* allocator, usize size) {
    if(allocator == NULL             
    || allocator->arena.mem == NULL 
    || allocator->arena.size == 0    
    || size == 0
    ) return NULL;
    allocator->offset += size;
    return (void*)(allocator->arena.mem + allocator->offset);
}

static void ScratchAllocator_delete(ScratchAllocatorData* allocator) {
    free(allocator->arena.mem);
    allocator->arena.mem = NULL;
    allocator->arena.size = 0;
    allocator->offset = 0;
}

static void ScratchAllocator_reset(ScratchAllocatorData* allocator) {
    allocator->offset = 0;
}

void ScratchAllocator_init(Allocator* allocator, ScratchAllocatorData* opts, Allocator* fallback) {
    *allocator = (Allocator){
        .data = { .scratch = opts },
        .fallback = fallback,
        .vtable = {
            .alloc   = (AllocatorAllocFn*)ScratchAllocator_alloc,
            .realloc = NULL,
            .dealloc = NULL,
            .reset   = (AllocatorResetFn*)ScratchAllocator_reset,
            .delete  = (AllocatorDeleteFn*)ScratchAllocator_delete
        }
    };
}

/*** Heap Allocator Implementation ***/
static void* SysAllocator_alloc(SysAllocatorData* data, size_t size) {
    if(data == NULL || size == 0) return NULL;
    // OS specific implementation 
    data->mem = mmap(NULL, size, 
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, 
        -1, 0   // Since the memory is "anonymous" (no file)
    );
    if(data->mem == MAP_FAILED) return NULL;
    data->size = size;
    return data->mem;
}

static bool SysAllocator_dealloc(SysAllocatorData* data, void* mem) {
    if(data == NULL             
    || data->mem == NULL 
    || data->size == 0    
    || mem == NULL
    ) return false;
    if(munmap(mem, data->size) != 0) return false;
    return true;
}

static bool SysAllocator_realloc(SysAllocatorData* data, void* mem, usize size) {
    if(data == NULL             
    || data->mem == NULL 
    || data->size == 0    
    || mem == NULL
    ) return false;
    data->mem = mremap(mem, data->size, size, MREMAP_MAYMOVE, data->mem);
    if(data->mem == MAP_FAILED) return NULL;
    data->size = size;
    return data->mem;
}

Allocator SysAllocator_init(SysAllocatorData* config) {
    *config = (SysAllocatorData){0};
    return (Allocator){
        .data = { .sys = config },
        .vtable = {
            .alloc   = (AllocatorAllocFn*)SysAllocator_alloc,
            .dealloc = (AllocatorDeallocFn*)SysAllocator_dealloc,
            .realloc = (AllocatorReallocFn*)SysAllocator_realloc,
        }
    };
}

/****** GENERIC ALLOCATOR INTERFACE FUNCTIONS ******/
void* mem_alloc(Allocator* self, u64 size) {
    if(!self
    || !self->data.any
    || !self->vtable.alloc
    ) goto fail;
    void* result = self->vtable.alloc(self->data.any, size);
    /* If allocation failed and resize is implemented, 
     * attempt to resize the entire allocation space,
     * then reattempt allocation */
    if(!result) {
        Allocator* parent = self->fallback;
        bool parent_supports_realloc = (self->fallback == NULL) ? false: self->fallback->vtable.realloc;
        if(parent_supports_realloc) {
            if(!parent->vtable.realloc(parent, self->data.any, size)
            || self->vtable.alloc(self->data.any, size) == NULL) goto fail;
            /* Resize and allocation successful */
        } else goto fail;
    }
    return result;
fail:
    return NULL;
}

bool mem_dealloc(Allocator* self, void* ptr) {
    if(!self
    || !self->data.any
    || !self->vtable.dealloc
    ) goto fail;
    
    return self->vtable.dealloc(self->data.any, ptr);

fail:
    return false;
}

bool mem_realloc(Allocator* self, void* mem, usize size) {
    if(!self
    || !self->data.any
    || !self->vtable.realloc
    ) goto fail;
    
    return self->vtable.realloc(self->data.any, mem, size);

fail:
    return false;
}

void reset_allocator(Allocator* self) {
    assert( self != NULL );
    assert( self->data.any != NULL );
    assert( self->vtable.reset != NULL );
    self->vtable.reset(self->data.any);
}

void delete_allocator(Allocator* self) {
    assert( self != NULL );
    assert( self->data.any != NULL );
    assert( self->vtable.delete != NULL );
    self->vtable.delete(self->data.any);
}
