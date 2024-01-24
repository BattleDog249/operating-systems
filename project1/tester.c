 /* If you get memory errors or other weird behaviour from this code, try 
 * disabling the tests one at a time.  The most likely cause is that your 
 * bundle struct contains something other than valid function pointers, 
 * and disabling each test in turn will let you see which pointer is
 * at fault.
 *
 * If you change anything in this file, be sure to change it back and make
 * certain your final solution works without changing this file or tester.h.
 */

#include "tester.h"
#include<stdio.h>
#include<stdlib.h>

#define FILL(Y, X) int r##X(int x){return (10000 - ((x - X) * (x - X)));} Y = r##X;

int test(bundle* b){
	/* Begin test of find_best function */
	int (*returners[10])(int);
	FILL(returners[0], 10);
	FILL(returners[1], 20);
	FILL(returners[2], 30);
	FILL(returners[3], 40);
	FILL(returners[4], 50);
	FILL(returners[5], 60);
	FILL(returners[6], 70);
	FILL(returners[7], 80);
	FILL(returners[8], 90);
	FILL(returners[9], 100);

	int c1 = b->find_best((void*)returners, 23, 10), c2 = b->find_best((void*)returners, 77, 10);
	int done = 1;
	if(c1 == 1 && c2 == 7)
		printf("find_best apparently works!\n");
	else{
		printf("find_best returns incorrect answer\n");
		done = 0;
	}

	/* Begin test of sorted_copy function */
	int numbers[] = {4, 1, 2, 7, 3, 5, 6, 0, 8, 9};
	int numbers_dup[] = {4, 1, 2, 7, 3, 5, 6, 0, 8, 9};

	size_t len = sizeof(numbers)/sizeof(int);
	int* sorted_numbers = b->sorted_copy(numbers, len);
	int flag = 0;
	for(int* i = sorted_numbers; i < sorted_numbers + (len - 1); i++){
		printf("%d\n", *i);
		if(*i > *(i+1)){
			printf("Sorting error:  %d > %d\n", *i, *(i+1));
			flag = 1;
			done = 0;
		}
	}
	if(!flag){  
		printf("No sorting errors detected, but was the original unchanged?\n");
		for(size_t i = 0; i < len; ++i)
			if(numbers[i] != numbers_dup[i]) flag = 1;
		if(!flag)
			if(sorted_numbers[2] != 2)
				printf("Sorted numbers doesn't have a 2 in the right place!\n");
			else
				printf("No changes to numbers array, good!\n");
		else
			printf("Numbers array was changed!  Keep working!\n");

	}
	free(sorted_numbers);
	
	/* Begin test of combine function */
	int n = b->combine(0x636174, 0x656172);
	if(n == 0xcec00)
		printf("Combine returned %x (%d), correct!\n", 0xcec00, 0xcec00);
	else {
		printf("Combine returned %x (%d) rather than 0xCEC00, keep trying!\n", n, n);
		done = 0;
	}

	if(done){
		printf("Your bundle contained functions that passed all the tests\n");
		printf("Good job!\n");
	}

	return 0;
}
