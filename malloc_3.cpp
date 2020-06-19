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

bool LARGE_ENOUGH(MallocMetadata* block, size_t size) {
    return ( (block->size - _size_meta_data() - size) >= 128);
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

    block->is_free = true;

    // add at end of list
    block->next_free = nullptr;
    block->prev_free = iter;
    iter->next_free = block;

    // update used free_blocks, free_bytes
    free_blocks++;
    free_bytes += block->size;
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

    block->is_free = false;

    // update used free_blocks, free_bytes
    free_blocks--;
    free_bytes -= block->size;
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

/***
 * @param block - a free block to merge with adjacent free blocks
 */
void combineBlocks(MallocMetadata* block) {
    auto prev = block->heap_prev;
    auto next = block->heap_next;

    bool free_prev = prev->is_free;
    bool free_next = next->is_free;

    if (!free_prev && !free_next) return; // no combinations to do

    // actions to be taken in any combination option:
    removeFromFreeList(block); // remove the current block from the free list
    MallocMetadata* new_block = prev;
    size_t new_size = block->size + _size_meta_data();
    allocated_blocks--;
    allocated_bytes += _size_meta_data();

    // 3 combine options available

    // Option 1: Both adjacent blocks
    if (free_prev && free_next) {
        // remove previous and next blocks from the free list
        removeFromFreeList(prev);
        removeFromFreeList(next);

        // update heap pointers and set the new size accordingly
        prev->heap_next = next->heap_next;
        next->heap_next->heap_prev = prev;
        new_size += prev->size + next->size + _size_meta_data();

        allocated_blocks--;
        allocated_bytes += _size_meta_data();

    } else if (free_prev) {
        // Option 2: Only previous block

        // remove previous block from the free list
        removeFromFreeList(prev);

        // update heap pointers and set the new size accordingly
        prev->heap_next = block->heap_next;
        block->heap_next->heap_prev = prev;
        prev->size += prev->size;

    } else { // if (free_next)
        // Option 3: Only the next block
        new_block = block;

        // remove previous and next blocks from the free list
        removeFromFreeList(next);

        // update heap pointers and set the new size accordingly
        block->heap_next = next->heap_next;
        next->heap_next->heap_prev = block;
        new_size += next->size;
    }

    new_block->size = new_size; // update new_block's size
    addToFreeList(new_block);   // insert new block into the free list
                                // (+ update global variables)

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

/***
 * @param size - the desired final size of the wilderness
 */
void* enlargeWilderness(size_t size) {
    size_t missing_size = size - wilderness->size;
    void* res = sbrk(missing_size);
    if (res == (void*)(-1)) return nullptr; // something went wrong

    // update global var
    allocated_bytes += missing_size;

    // update wilderness size
    wilderness->size += missing_size;

    return (char*)wilderness + _size_meta_data();
}

/***
 * @param block - an allocated block to merge with adjacent free blocks
 *                (used for realloc)
 */
MallocMetadata* tryMergingNeighbor(MallocMetadata* block, size_t wanted_size) {
    auto prev = block->heap_prev;
    auto next = block->heap_next;

    bool free_prev = prev->is_free;
    bool free_next = next->is_free;

    if (!free_prev && !free_next) return nullptr; // merging is not an option

    size_t required_size = wanted_size - block->size - _size_meta_data();

    // Try merging with previous neighbour
    if (free_prev && prev->size <= required_size) {
        // remove the free neighbor from free list
        removeFromFreeList(prev);
        prev->is_free = false;

        // copy the data to the start of the new block
        void* copy_from = (char*)block + _size_meta_data();
        void* copy_to = (char*)prev + _size_meta_data();
        memcpy(copy_to, copy_from, block->size);

        // update new block's size
        prev->size += _size_meta_data() + block->size;

        // update heap pointers
        prev->heap_next = block->heap_next;
        block->heap_next->heap_prev = prev;

        // update global variables
        allocated_blocks--;
        allocated_bytes += _size_meta_data();

        return prev;    // return for allocation (not added to free list)
    }

    // Try merging with next neighbour
    if (free_next && next->size <= required_size) {
        // remove the free neighbor from free list
        removeFromFreeList(next);
        next->is_free = false;

        // no need to copy the data

        // update new block's size
        block->size += _size_meta_data() + next->size;

        // update heap pointers
        block->heap_next = next->heap_next;
        next->heap_next->heap_prev = block;

        allocated_blocks--;
        allocated_bytes += _size_meta_data();

        return block;
    }

    required_size -= _size_meta_data();

    // Try merging with both neighbours
    if (free_prev && free_next && prev->size + next->size <= required_size) {
        // remove the free neighbor from free list
        removeFromFreeList(prev);
        removeFromFreeList(next);
        prev->is_free = false;

        // copy the data to the start of the new block
        void* copy_from = (char*)block + _size_meta_data();
        void* copy_to = (char*)prev + _size_meta_data();
        memcpy(copy_to, copy_from, block->size);

        // update new block's size
        prev->size += 2*_size_meta_data() + block->size + next->size;

        // update heap pointers
        prev->heap_next = next->heap_next;
        next->heap_next->heap_prev = prev;

        allocated_blocks -= 2;
        allocated_bytes += 2*_size_meta_data();

        return prev;    // return for allocation (not added to free list)
    }

    return nullptr; // merging is not an option
}

void cutAllocatedBlock(MallocMetadata* block, size_t size) {
    if (LARGE_ENOUGH(block, size)) {
        block->is_free = true;
        addToFreeList(block);
        cutBlocks(block, size);
        removeFromFreeList(block);
        block->is_free = false;
    }
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
        if (LARGE_ENOUGH(to_alloc, size)) {
            cutBlocks(to_alloc, size);
        }

        // mark block as alloced
        to_alloc->is_free = false;

        // remove from list (+ update global variables)
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
        // (+ update global variables)
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

    // add to free list (+update global variables)
    addToFreeList(meta);

    // call combine
    combineBlocks(meta);
}

void* srealloc(void* oldp, size_t size) {
    // check parameters
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // if given null pointer, allocate normally
    if (oldp == nullptr)
        return smalloc(size);

    auto block = (MallocMetadata *) ((char *) oldp - _size_meta_data());
    size_t old_size = block->size;

    // if old_block is mmap()-ed
    if (block->is_mmap) {
        // mmap() new block using smalloc (size >= MMAP_THRESHOLD)
        // copy data, and free old block
        return reallocate(oldp, old_size, size);
    }

    // if not mmap()=ed and size is smaller
    if (size <= old_size) {
        cutAllocatedBlock(block, size); // try to cut block
        return oldp;                    // reuse the same block
    }

    // Othewise, need to try and enlarge the block
    // From here onwards size > old_Size

    // If wilderness block was given
    if (block == wilderness && block->heap_next == nullptr) {
        // enlarge wilderness block and update global vars
        return enlargeWilderness(size);
        // (pointer already includes metadata offset)
        // nullptr is returned if something went wrong
    }

    // try merging with a neighboring free block
    // after merge the neighbor blocks are not in the free list
    // and the old data was copied to the beginning of the block
    MallocMetadata *merged_block = tryMergingNeighbor(block, size);
    if (!merged_block) {
        // if merging was not an option
        // Final option: find new block in heap using smalloc
        // copy data, and free old block
        return reallocate(oldp, old_size, size);
    }
    // else, merging was done

    cutAllocatedBlock(merged_block, size); // Try to cut blocks
    merged_block->is_free = false;  // set merged block as not free

    return (char *)merged_block + _size_meta_data();
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