#include <unistd.h>
#include <string.h>

size_t free_blocks, free_bytes, allocated_blocks, allocated_bytes;

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

// head of free blocks list
MallocMetadata dummy_free;

/***
 * @param block - a free block to be added to the free list
 */
void addToFreeList(MallocMetadata* block) {
    // traverse from dummy
    MallocMetadata* iter = &dummy_free;
    while (iter->next) {
        if (iter->next->size > block->size) { // find proper place
            // add
            block->prev = iter;
            block->next = iter->next;
            iter->next->prev = block;
            iter->next = block;

            return;
        }
    }

    // add at end of list
    block->next = nullptr;
    block->prev = iter;
    iter->next = block;
}

/***
 * @param block - an allocated block to be removed from the free list
 */
void removeFromFreeList(MallocMetadata* block) {
    block->prev->next = block->next;
    block->next->prev = block->prev;
    block->prev = nullptr;
    block->next = nullptr;
}

void* smalloc(size_t size) {
    // check conditions
    if (size == 0) return nullptr;


    // search the first free that have enough size in the list
    // update free_blocks, free_bytes
    // remove from list

    // if no, allocate with sbrk + _size_meta_data()
    // add meta (die in spanish)
    // update allocated_blocks, allocated_bytes

    // when return, don't forget the offset

}

void* scalloc(size_t num, size_t size) {
    // use smalloc with num * size

    // nullify with memset

}

void sfree(void* p) {
    // check if null or released

    // use p - _size_meta_data()

    // mark as released

    // add to list (call function)
    // update used free_blocks, free_bytes

}

void* srealloc(void* oldp, size_t size) {
    // if old == null just smalloc

    // if size is smaller, reuse

    // find new place with smalloc

    // copy with memcpy

    // free with sfree (only if you succeed until now)

}

size_t _num_free_blocks() {
    return free_blocks;
}

size_t _num_free_bytes() {
    return free_bytes;
}

size_t _num_allocated_blocks() {
    return allocated_blocks;
}

size_t _num_allocated_bytes() {
    return allocated_bytes;
}

size_t _num_meta_data_bytes() {
    return allocated_blocks * _size_meta_data;
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}