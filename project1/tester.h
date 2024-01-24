#include<unistd.h>

struct bundle_struct {
	int (*find_best)(void*, int, size_t);
	int* (*sorted_copy)(int*, size_t);
	int (*combine)(int, int);
};

typedef struct bundle_struct bundle;

int test(bundle*);
