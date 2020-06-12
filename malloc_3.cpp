#include <unistd.h>
#include <string.h>

/*---------------DECLARATIONS-----------------------------------*/
#define MAX_ALLOC 100000000

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

MallocMetadata dummy_free; // sorted list by size

MallocMetadata* heap_head;
MallocMetadata* wilderness;

#define LARGE_ENOUGH (old_size - _size_meta_data() - size) >= 128)




/*---------------HELPER FUNCTIONS---------------------------/
/***
 * @param block - a free block to be added to the free list
 */
void addToFreeList(MallocMetadata* block) {
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
    block->prev_free->next_free = block->next_free;
    block->next_free->prev_free = block->prev_free;
    block->prev_free = nullptr;
    block->next_free = nullptr;
}


// block is a free block
void cutBlocks(MallocMetadata* block, size_t wanted_size) {
    // put new mata object in (block + wanted_size + _size_meta_data())

    // remove old one from list
    // add the second block to free_list (start from dummy to find place)
    // update global vars

    // heap_list update:
    // second block is next of first block
    // second block's next is first block next

    // new_mata->size = old_mata->size - ( wanted + _size_meta_data() )

    // is free = true in new meta

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


/*------------ASSIGNMENT FUNCTION----------------------------------*/
void* smalloc(size_t size) {


    // conditions

    // if size >= 128*1024 use mmap (+_size_meta_data())
    // update allocated vars

    // search the first free that have enough size in the list
    // if large enough - cut blocks
    // update free_blocks, free_bytes
    // remove from list

    // if no free memory chunk was found big enough.
    // And the wilderness chunk is free
    // enlarge the wilderness (sbrk)
    // (size of wanted-wilderness)

    // if no, allocate with sbrk + _size_meta_data()
    // add meta (die in spanish)
    // update allocated_blocks, allocated_bytes
    // update wilderness

    // when return, don't forget the offset



    // check conditions
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // find the first free block that have enough size
    MallocMetadata* to_alloc = dummy_free.next;
    while (to_alloc) {
        if (to_alloc->size >= size) break;
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
    if (new_block == (void*)(-1)) return nullptr; // somthing went wrong

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

    // nullify with memset
    memset(alloc, 0, num * size);

    return alloc;
}

void sfree(void* p) {



    // check if null or released

    // use p - _size_meta_data()

    // if block is mmap'ed use munmap
    // update allocated_blocks, allocated_bytes (no need to update free)

    // mark as released

    // add to free_list (call function)
    // update used free_blocks, free_bytes
    // call combine




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
    // if old == null just smalloc

    // if size is smaller, reuse
    // don't update global vars

    // if mmap - unmap and than mmap (+_size_meta_data()
    // update allocated vars

    // try merging (prev_heap, upper_heap, three blocks)
    // if large enough call cut block
    // update heap_list
    // update free_list (you need to remove prev or next you used)
    // update free global vars

    // if i'm wilderness enlarge brk
    // update global vars

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