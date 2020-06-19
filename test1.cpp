#include <iostream>
#include <assert.h>
#include <unistd.h>
#include "tests_header.h"
using namespace std;

int N = 10;

int main() {
    cout << _size_meta_data() << endl;
    cout << sbrk(0) << endl;
    int* arr = (int*)smalloc(16);
    int* arr2 = (int*)smalloc(16);

    arr[0] = 1;
    arr[1] = 3;
    arr[2] = 5;
    arr[3] = 7;
    arr2[0] = 2;
    arr2[1] = 4;
    arr2[2] = 6;
    arr2[3] = 8;

    cout << arr << endl;
    cout << arr2 << endl;

    return 0;
}