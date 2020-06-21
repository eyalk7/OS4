#ifndef OS4_TESTS_HEADER_H
#define OS4_TESTS_HEADER_H

#include <unistd.h>
#include <iostream>
#include <string.h>
#include <iostream>
#include <assert.h>
#include <unistd.h>
#include "tests_header.h"
using namespace std;

void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();

#endif //OS4_TESTS_HEADER_H
