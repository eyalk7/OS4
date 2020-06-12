#include <unistd.h>
#include <string.h>

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

bool FIRST_ALLOC;

MallocMetadata dummy_free; // sorted list by size

MallocMetadata* heap_head;
MallocMetadata* wilderness;

#define LARGE_ENOUGH (old_size - sizeof(MallocMetaData) - size) >= 128)

size_t align(size_t size) {
    if (size % 8 == 0) return size;
    return ((size / 8) + 1) * 8;
}

void addToFreeList(MallocMetadata* block) {
    // traverse from dummy

    // find proper place
    // add
}

void removeFromFreeList(MallocMetadata* block) {
    // prev next next prev
}

// block is a free block
void cutBlocks(MallocMetadata* block, size_t wanted_size) {
    // put new mata object in (block + wanted_size + sizeof(mata))

    // remove old one from list
    // add the second block to free_list (start from dummy to find place)
    // update global vars

    // heap_list update:
    // second block is next of first block
    // second block's next is first block next

    // new_mata->size = old_mata->size - ( wanted + sizeof(mata) )

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


void* smalloc(size_t size) {
    // if FIRST ALLOC:
    // check if (sbrk(0) % 8 != 0) align (only affective for first alloc)

    // conditions

    // align size

    // if size >= 128*1024 use mmap (+sizeof(meta))
    // update allocated vars

    // search the first free that have enough size in the list
    // if large enough - cut blocks
    // update free_blocks, free_bytes
    // remove from list

    // if no free memory chunk was found big enough.
    // And the wilderness chunk is free
    // enlarge the wilderness (sbrk) (aligned!)
    // (size of wanted-wilderness)

    // if no, allocate with sbrk + sizeof (mata)
    // add meta (die in spanish)
    // update allocated_blocks, allocated_bytes
    // update wilderness

    // when return, don't forget the offset
}

void* scalloc(size_t num, size_t size) {
    // use smalloc with num * size

    // nullify with memset

}

void sfree(void* p) {
    // check if null or released

    // use p - sizeof(metadata)

    // if block is mmap'ed use munmap
    // update allocated_blocks, allocated_bytes (no need to update free)

    // mark as released

    // add to free_list (call function)
    // update used free_blocks, free_bytes
    // call combine

}

void* srealloc(void* oldp, size_t size) {
    // if old == null just smalloc

    // if size is smaller, reuse
    // don't update global vars

    // if mmap - unmap and than mmap
    // update allocated vars

    // try merging (prev_heap, upper_heap, three blocks)
    // if large enough call cut block
    // update heap_list
    // update free_list (you need to remove prev or next you used)
    // update free global vars

    // if i'm wilderness enlarge brk (aligned!)
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
    return align(sizeof(MallocMetadata));
}