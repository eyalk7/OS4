#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>

/*---------------DECLARATIONS-----------------------------------*/
#define MAX_ALLOC 100000000
#define MMAP_THRESHOLD 131072 // = 128*1024

void* smalloc(size_t size);
void sfree(void* p);
size_t _size_meta_data();

size_t free_blocks, free_bytes, allocated_blocks, allocated_bytes;

struct MallocMetadata {
    size_t size;
    bool is_free;
    bool is_mmap;

    MallocMetadata* next_free; // if not free then null
    MallocMetadata* prev_free; // sorted by size

    MallocMetadata* heap_next; // every block has
    MallocMetadata* heap_prev; // sorted by address
};

// sorted list by size
MallocMetadata dummy_free = {0, false, false, nullptr, nullptr, nullptr, nullptr};

MallocMetadata* heap_head = nullptr;
MallocMetadata* wilderness = nullptr;

/*---------------HELPER FUNCTIONS---------------------------*/

bool LARGE_ENOUGH(size_t old_size, size_t size) {
    return ( (old_size - _size_meta_data() - size) >= 128);
}

/***
 * @param block - a free block to be added to the free list
 */
void addToFreeList(MallocMetadata* block) {
    assert(block);

    // traverse from dummy
    MallocMetadata* iter = &dummy_free;
    while (iter->next_free) {
        if (iter->next_free->size > block->size) { // find proper place
            // add
            block->prev_free = iter;
            block->next_free = iter->next_free;
            iter->next_free->prev_free = block;
            iter->next_free = block;

            return;
        }

        iter = iter->next_free;
    }

    // add at end of list
    block->next_free = nullptr;
    block->prev_free = iter;
    iter->next_free = block;
}

/***
 * @param block - an allocated block to be removed from the free list
 */
void removeFromFreeList(MallocMetadata* block) {
    assert(block);

    if (block->prev_free) block->prev_free->next_free = block->next_free;
    if (block->next_free) block->next_free->prev_free = block->prev_free;
    block->prev_free = nullptr;
    block->next_free = nullptr;
}

/***
 * @param block - a free block that is LARGE ENOUGH to be cut
 */
void cutBlocks(MallocMetadata* block, size_t wanted_size) {
    // put a new meta object in (block + _size_meta_data()  + wanted_size)
    MallocMetadata* new_block = (MallocMetadata*) ((char*)block + _size_meta_data() + wanted_size);
    new_block->size = block->size - wanted_size - _size_meta_data();
    new_block->is_free = true; // new block is free
    new_block->is_mmap = false; // new block not mmap'ed

    // add the new block to free list
    addToFreeList(new_block);

    // update old block size, remove from free list and add again (so it will be in proper place)
    block->size = wanted_size;
    removeFromFreeList(block);
    addToFreeList(block);

    // update global vars
    allocated_blocks++;                     // created new block
    allocated_bytes -= _size_meta_data();   // we've allocated this amount of bytes to be
                                            // metadata from the previously user bytes

    // update heap list
    new_block->heap_next = block->heap_next;
    new_block->heap_prev = block;
    block->heap_next = new_block;
}

void combineBlocks(MallocMetadata* block) {
     // 4 options:
    // prev_heap+curr+next_heap
    // curr+next_heap
     // prev_heap + curr
    // no combinations

     // if combined, remove from free list and add the new one
     // update global vars
     // update heap_list

}

void* reallocate(void* oldp, size_t old_size, size_t new_size) {
    void* newp = smalloc(new_size);
    if (newp == nullptr) return nullptr;    // smalloc failed

    // copy old data to new block using memcpy
    memcpy(newp, oldp, old_size);

    // free old data using sfree (only if you succeed until now)
    sfree(oldp);

    return newp;
}

void* enlargeWilderness(size_t size) {
    size_t missing_size = size - wilderness->size;
    void* res = sbrk(missing_size);
    if (res == (void*)(-1)) return nullptr; // something went wrong

    // update global var
    allocated_bytes += missing_size;
    free_bytes -= wilderness->size;
    free_blocks--;

    // update wilderness size
    wilderness->size += missing_size;

    return (char*)wilderness + _size_meta_data();
}

/*------------ASSIGNMENT FUNCTIONS----------------------------------*/
void* smalloc(size_t size) {
    // check conditions
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // if size >= 128*1024 use mmap (+_size_meta_data())
    if (size >= MMAP_THRESHOLD) {
        MallocMetadata* alloc = (MallocMetadata*) mmap(NULL, _size_meta_data() + size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        alloc->size = size;
        alloc->is_mmap = true;

        // update allocated vars
        allocated_blocks++;
        allocated_bytes += size;

        return (char*)alloc + _size_meta_data();
    }

    // find the first free block that have enough size
    MallocMetadata* to_alloc = dummy_free.next_free;
    while (to_alloc) {
        if (to_alloc->size >= size) break;
        to_alloc = to_alloc->next_free;
    }
    if (to_alloc) { // we found a block!
        // if block large enough, cut it
        if (LARGE_ENOUGH(to_alloc->size, size)) {
            cutBlocks(to_alloc, size);
        }

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

    // if no free block was found And the wilderness chunk is free
    if (wilderness && wilderness->is_free) {
        // enlarge the wilderness (sbrk)
        void* res = enlargeWilderness(size);
        if (res == nullptr) return nullptr; // something went wrong

        // update wilderness status
        wilderness->is_free = false;

        // remove wilderness from free list
        removeFromFreeList(wilderness);

        return res; // (pointer already includes metadata offset)
    }

    // the previous program break will be the new block's place
    MallocMetadata* new_block = (MallocMetadata*) sbrk(0);
    if (new_block == (void*)(-1)) return nullptr; // somthing went wrong

    // allocate with sbrk
    void* res = sbrk(_size_meta_data() + size);
    if (res == (void*)(-1)) return nullptr; // sbrk failed

    // add metadata
    new_block->size = size;
    new_block->is_mmap = false;
    new_block->is_free = false;
    new_block->next_free = nullptr;
    new_block->prev_free = nullptr;
    new_block->heap_next = nullptr;
    new_block->heap_prev = wilderness; // if no wilderness: wilderness == nullptr, so it's ok

    // update wilderness
    if (wilderness) wilderness->heap_next = new_block;
    wilderness = new_block;

    // if first allocation initialize heap_head and wilderness
    if (!heap_head) {
        heap_head = new_block;
        wilderness = new_block;
    }


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

    // if mmaped no need to nullify
    if (((MallocMetadata*)((char*)alloc - _size_meta_data()))->is_mmap) return alloc;

    // nullify with memset
    memset(alloc, 0, num * size);

    return alloc;
}

void sfree(void* p) {
    // check if null or released
    if (!p) return;
    MallocMetadata* meta = (MallocMetadata*) ((char*)p - _size_meta_data());
    if (meta->is_free) return;

    // if block is mmap'ed than munmap
    if (meta->is_mmap) {
        // update allocated_blocks, allocated_bytes
        allocated_blocks--;
        allocated_bytes -= meta->size;

        // unmap
        assert(munmap(meta, meta->size + _size_meta_data()) == 0);

        return;
    }

    // mark as released
    meta->is_free = true;

    // add to free list
    addToFreeList(meta);

    // update used free_blocks, free_bytes
    free_blocks++;
    free_bytes += meta->size;

    // call combine
    combineBlocks(meta);
}

void* srealloc(void* oldp, size_t size) {
    // check parameters
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // if given null pointer, allocate normally
    if (oldp == nullptr)
        return smalloc(size);

    auto meta = (MallocMetadata*) ((char*)oldp - _size_meta_data());
    size_t old_size = meta->size;

    // if old_block is mmap()-ed
    if (meta->is_mmap) {
        // mmap() new block using smalloc (size >= MMAP_THRESHOLD)
        // copy data, and free old block
        return reallocate(oldp, old_size, size);
    }

    // if not mmap()=ed and size is smaller
    if (size <= old_size) return oldp; // reuse the same block

    // Othewise, need to try and enlarge the block
    // From here onwards size > old_Size

    // If wilderness block was given
    if (meta == wilderness && meta->heap_next == nullptr) {
        // enlarge wilderness block and update global vars
        return enlargeWilderness(size);
        // (pointer already includes metadata offset)
        // nullptr is returned if something went wrong
    }

    // if size > old_size then try enlarging heap before merge check

    // try merging (prev_heap, upper_heap, three blocks)
    // if large enough call cut block
    // update heap_list
    // update free_list (you need to remove prev or next you used)
    // update free global vars

    // Final option: find new block in heap using smalloc
    // copy data, and free old block
    return reallocate(oldp, old_size, size);
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