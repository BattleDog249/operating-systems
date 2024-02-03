#include "tester.h" // Include the header file for the test function and the bundle struct definition
#include <stdlib.h>
#include <string.h>
#include <limits.h> // Include the limits library for INT_MIN

// Compare function for qsort
int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

// Find the best function in a list of functions by calling each function with the given parameter and returning the index of the function that returns the highest value
int find_best(int (*func_list[])(int), int param, int list_size) {
    int max_value = INT_MIN; // Initialize max_value to the smallest possible integer
    int max_index = -1;
    for (int i = 0; i < list_size; i++) {
        int value = func_list[i](param); // Call the function with the given parameter
        if (value > max_value) {
            max_value = value;
            max_index = i;
        }
    }
    return max_index;
}

// Create a sorted copy of the input array using qsort
int* sorted_copy(int* array, size_t length) {
    int* copy = malloc(length * sizeof(int)); // Allocate memory for the new array
    if (copy == NULL) { // Check if malloc failed
        exit(1);
    }
    memcpy(copy, array, length * sizeof(int)); // Copy the input array to the new array
    qsort(copy, length, sizeof(int), compare); // Sort the copy using qsort and the compare function
    return copy;
}

// Combine two integers by taking the bitwise OR of the two, then shifting the result 3 bits to the right, and finally zeroing out the last byte
int combine(int a, int b) {
    int result = (a | b) >> 3; // Bitwise OR and shift right
    result &= ~0xFF; // Zero out the last byte
    return result;
}

int main() {
	// Create a bundle struct with the function pointers
    bundle my_bundle = {
        .find_best = (int (*)(void*, int, size_t)) find_best,
        .sorted_copy = sorted_copy,
        .combine = combine
    };

    // Call the test function with the bundle struct
    test(&my_bundle);

    return 0;
}
