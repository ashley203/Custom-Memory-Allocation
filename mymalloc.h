#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

void myinit(int allocAlg);
void* mymalloc(size_t size);
void myfree(void* ptr);
void* myrealloc(void* ptr, size_t size); void mycleanup();
double utilization();
