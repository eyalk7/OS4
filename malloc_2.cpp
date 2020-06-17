#include <unistd.h>
#include <string.h>


/*---------------DECLARATIONS-----------------------------------*/
#define MAX_ALLOC 100000000

size_t _size_meta_data();

size_t free_blocks, free_bytes, allocated_blocks, allocated_bytes;

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

// head of free blocks list
MallocMetadata dummy_free = {0, false, nullptr, nullptr};


/*---------------HELPER FUNCTIONS---------------------------/
/***
 * @param block - a free block to be added to the free list
 */
void addToFreeList(MallocMetadata* block) {
    assert(block);

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

        iter = iter->next;
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
    assert(block);

    if (block->prev) block->prev->next = block->next;
    if (block->next) block->next->prev = block->prev;
    block->prev = nullptr;
    block->next = nullptr;
}

/*------------ASSIGNMENT FUNCTIONS----------------------------------*/
void* smalloc(size_t size) {
    // check conditions
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // find the first free block that have enough size
    MallocMetadata* to_alloc = dummy_free.next;
    while (to_alloc) {
        if (to_alloc->size >= size) break;
        to_alloc = to_alloc->next;
    }
    if (to_alloc) { // we found a block!
        // mark block as alloced
        to_alloc->is_free = false;

        // update free_blocks, free_bytes
        free_blocks--;
        free_bytes -= to_alloc->size;

        // remove from list
        removeFromFreeList(to_alloc);

        // return the address after the metadata
        return (char*)to_alloc + _size_meta_data();
    }

    // the previous program break will be the new block's place
    MallocMetadata* new_block = (MallocMetadata*) sbrk(0);
    if (new_block == (void*)(-1)) return nullptr; // something went wrong

    // allocate with sbrk
    void* res = sbrk(_size_meta_data() + size);
    if (res == (void*)(-1)) return nullptr; // sbrk failed

    // add metadata
    new_block->size = size;
    new_block->is_free = false;
    new_block->next = nullptr;
    new_block->prev = nullptr;

    // update allocated_blocks, allocated_bytes
    allocated_blocks++;
    allocated_bytes += size;

    // when return, don't forget the offset
    return (char*)new_block + _size_meta_data();
}

void* scalloc(size_t num, size_t size) {
    // use smalloc with num * size
    void* alloc = smalloc(num * size);
    if (!alloc) return nullptr;

    // nullify with memset
    memset(alloc, 0, num * size);

    return alloc;
}

void sfree(void* p) {
    // check if null or released
    if (!p) return;
    MallocMetadata* meta = (MallocMetadata*) ((char*)p - _size_meta_data());
    if (meta->is_free) return;

    // mark as released
    meta->is_free = true;

    // add to free list
    addToFreeList(meta);

    // update used free_blocks, free_bytes
    free_blocks++;
    free_bytes += meta->size;
}

void* srealloc(void* oldp, size_t size) {
    // check parameters
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // if given null pointer, allocate normally
    if (oldp == nullptr)
        return smalloc(size);

    auto meta = (MallocMetadata*) ((char*)oldp - _size_meta_data());
    size_t old_size = meta->size;

    // if size is smaller, reuse the same block
    if (size <= old_size) return oldp;

    // Ohterwise, find new block using smalloc
    void* newp = smalloc(size);
    if (newp == nullptr) return nullptr;    // smalloc failed

    // copy old data to new block using memcpy
    memcpy(newp, oldp, old_size);

    // free old data using sfree (only if you succeed until now)
    sfree(oldp);

    return newp;
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
    return allocated_blocks * _size_meta_data();
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}