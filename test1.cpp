#include <iostream>
#include <assert.h>
#include "tests_header.h"
using namespace std;

int N = 10;

int main() {
    int* arr = (int*)smalloc(sizeof(int) * N);
    assert(arr);

    for (int i=0; i<N; i++) arr[i] = i;
    for (int i=0; i<N; i++) assert(arr[i] == i);

    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == N*sizeof(int));
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);

    sfree(arr);

    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == N*sizeof(int));
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == N*sizeof(int));

    arr = (int*) scalloc(2*N, sizeof(int));
    assert(arr);

    for (int i=0; i<2*N; i++) assert(arr[i] == 0);

    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 2*N*sizeof(int));
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);

    sfree(arr);

    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 2*N*sizeof(int));
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 2*N*sizeof(int));

    cout << "FINISHED ALRIGHT" << endl;

    return 0;
}