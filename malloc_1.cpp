#include <unistd.h>

/*---------------DECLARATIONS-----------------------------------*/
#define MAX_ALLOC 100000000

/*------------ASSIGNMENT FUNCTION----------------------------------*/
void* smalloc(size_t size) {
    // check parameters
    if (size == 0 || size > MAX_ALLOC) return nullptr;

    // allocate with sbrk
    void* ptr = sbrk(size);
    if (ptr == (void*)(-1)) return nullptr; // sbrk failed

    return ptr; // return previous program break
}


