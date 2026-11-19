/* Name:        arenas.c
 * Description: Arena implementations
 * By:          Nigel Sinclair
 * Github:      https://github.com/sincngraeme
 */

#include "csl-allocators.h"

/* Scratch arenas allocate a given number of bytes each time, each pointer 
 * having the same lifetime as the arena. The only way to deallocate is to 
 * reset or destroy the arena
 */
static void* ScratchAllocator_alloc(ScratchAllocatorData* allocator, usize size) {
    if(allocator == NULL             
    || allocator->arena.mem == NULL 
    || allocator->arena.size == 0    
    || size == 0
    ) return NULL;
    void* ptr = &allocator->arena.mem[allocator->offset];
    allocator->offset += size;
    return ptr;
}

static void ScratchAllocator_destroy(Allocator* self) {
    assert(self != NULL);
    assert(self->data.any != NULL);
    assert(self->parent != NULL);
    assert(self->parent->vtable.dealloc != NULL);
    ScratchAllocatorData* data = (ScratchAllocatorData*)self->data.any;
    bool result = self->parent->vtable.dealloc(self->parent->data.any, data->arena.mem);
    assert(result);
    data->arena.mem = NULL;
    data->arena.size = 0;
    data->offset = 0;
}

static void ScratchAllocator_reset(ScratchAllocatorData* data) {
    data->offset = 0;
}

void ScratchAllocator_init(Allocator* allocator, ScratchAllocatorData* opts, Allocator* fallback) {
    *allocator = (Allocator){
        .data = { .scratch = opts },
        .parent = fallback,
        .vtable = {
            .alloc   = (AllocatorAllocFn*)ScratchAllocator_alloc,
            .realloc = NULL,
            .dealloc = NULL,
            .reset   = (AllocatorResetFn*)ScratchAllocator_reset,
            .destroy  = (AllocatorDestroyFn*)ScratchAllocator_destroy
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

/*** Block Allocators ***/
void* BlockAllocator_alloc(AllocatorData* self, size_t size);
bool  BlockAllocator_realloc(AllocatorData* self, void* data, size_t size);
bool  BlockAllocator_dealloc(AllocatorData* self, void* data);
void  BlockAllocator_reset(AllocatorData* self);
void  BlockAllocator_destroy(struct Allocator* self);

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
        Allocator* parent = self->parent;
        bool parent_supports_realloc = (self->parent == NULL) ? false: self->parent->vtable.realloc;
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

void destroy_allocator(Allocator* self) {
    assert( self != NULL );
    assert( self->data.any != NULL );
    assert( self->parent != NULL);
    assert( self->parent->vtable.dealloc != NULL);
    /* If there is a destructor function, run it */
    if(self->vtable.destroy != NULL) self->vtable.destroy(self);
    /* NULL out the data to indicate that it is no longer valid
     * (to be used again must be reinit)*/
    self->data.any = NULL;
}
