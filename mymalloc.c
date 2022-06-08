#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INITIAL_HEAP_SIZE 1024*1024
#define HEAP_SIZE   1024*1024 - 16 -4

#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Doubleword size (bytes) */
#define MINBLOCKSIZE    24

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))
#define PUT_PTR(p, val)  (*(char *)(p) = *(val))

#define GET8(p)      (*(unsigned long *)(p))
#define PUT8(p, val) (*(unsigned long *)(p) = (unsigned long)(val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x3)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given a block ptr, returns a pointer to the next and prev free block */
//#define NEXT_FREE_BLKP(bp)  ((char *)bp)
#define NEXT_FREE_BLKP(bp)  ((char*)GET8((char*)(bp)))
//#define PREV_FREE_BLKP(bp)  ((char *)bp + 1)
#define PREV_FREE_BLKP(bp)  ((char*)GET8((char*)(bp) + DSIZE))

char* myHeap = NULL;
int fit = 0;
char* nextPtr = NULL; // for next best fit
char* freeRoot = NULL;
size_t used_space = 0;

void myinit(int allocAlg){
    myHeap = malloc(INITIAL_HEAP_SIZE);  // 2**20 or 1 << 20
    fit = allocAlg;

    PUT(myHeap, 0); // alignment padding
    PUT(myHeap + (1*WSIZE), PACK(DSIZE, 1)); // Prologue header
    PUT(myHeap + (2*WSIZE), PACK(DSIZE, 1)); // Prologue footer
    PUT(myHeap + INITIAL_HEAP_SIZE - WSIZE, PACK(WSIZE, 1));     // Epilogue header

    freeRoot = myHeap + (4*WSIZE);  // after alignment + Header + Footer + Header of free block

    //printf("!!%p\n", myHeap+16);

    printf("freeroot: %p\n",freeRoot);

    //make one big free block
    PUT(freeRoot-4, PACK(HEAP_SIZE, 0)); // header
    PUT(myHeap + INITIAL_HEAP_SIZE - DSIZE, PACK(HEAP_SIZE, 0));  // footer
    PUT8(freeRoot, (unsigned long)NULL);  // next pointer = NULL
    PUT8(freeRoot +8, (unsigned long)NULL);  // prev pointer = NULL
    printf("Initial freeroot: %u %u %p\n",*(freeRoot+8), GET(freeRoot+8), NEXT_BLKP(freeRoot));
    nextPtr = freeRoot;
    //printf("!!%p %u\n", myHeap+16, GET(myHeap+16));
  
}

void splitAndInsert(char* ptr, size_t size){
    //printf("split %p\n", ptr);

    //update nextptr

    char* next = NEXT_FREE_BLKP(ptr);
    char* prev = PREV_FREE_BLKP(ptr);

    //enough room for a new free block
    if(GET_SIZE(HDRP(ptr)) - size >= 24){
        printf("enough room\n");
        //make new free block header and footer
        char* free = ptr + size;
        PUT( free  - WSIZE, PACK( GET_SIZE(HDRP(ptr)) - size, 0) );
        PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)) - size, 0) );
        
        //add next and prev to new free block
        PUT8(free, next); // prev block: put into prev's next pointer
        PUT8(free + DSIZE, prev);

        //change next of prev block and prev of next block
        if (prev!=NULL){
            PUT8(prev, free);
        }  // 
        else if(next!=NULL){
            PUT8(next + DSIZE, free); // link next free's prev to split block
        }

        //make new malloced block with size and valid
        PUT(HDRP(ptr), PACK( size , 1));
        PUT(FTRP(ptr), PACK( size , 1));

        //update freeRoot if freeRoot is malloced
        if(freeRoot==(ptr)){
            freeRoot=freeRoot+size;
        }
    }
    else{  //use whole free block
        //update freeRoot if freeRoot is malloced
        if(ptr==freeRoot){
            freeRoot=NEXT_FREE_BLKP(freeRoot);
        }

        PUT8(NEXT_FREE_BLKP(prev), next); // prev block: put into prev's next pointer
        PUT8(PREV_FREE_BLKP(next) + DSIZE, prev); // next block
        
        PUT(HDRP(ptr), PACK( GET_SIZE(HDRP(ptr)) , 1)); //instead of adjustedsize use size of entire free block
	    PUT(FTRP(ptr), PACK( GET_SIZE(HDRP(ptr)) , 1));

    }
}

void* first_next_fit(size_t size){
    void* bp;
    if(fit == 0){
        bp = freeRoot;
    }
    else{ // fit = 1
        if(nextPtr==NULL){
            nextPtr=freeRoot;
        }
        bp = nextPtr;
    }

    if(fit==0){
        for (; bp!=NULL; bp = NEXT_FREE_BLKP(bp)) {
          if (!GET_ALLOC(HDRP(bp)) && (size <= GET_SIZE(HDRP(bp)))) {
              if (size <= GET_SIZE(HDRP(bp))) {
                  nextPtr = NEXT_FREE_BLKP(bp);
                  return bp;
              }
          }
        }
    }
    else{
        do{
            if(bp==NULL){
                bp=freeRoot;
            }
            if (size <= GET_SIZE(HDRP(bp))) {
                nextPtr = NEXT_FREE_BLKP(bp);
                return bp;
            }
            bp = NEXT_FREE_BLKP(bp);
        }while(bp!=nextPtr);
    }
    return NULL;
}

void* best_fit(size_t size){
    printf("r\n");
    void *bestFit=freeRoot; 
    void *bp;
    for (bp = freeRoot; bp!=NULL; bp = NEXT_FREE_BLKP(bp)) {
        if(GET_SIZE(HDRP(bp))==size){
            return bp;
        }
        //free block size is big enough && smaller than current best block
        else if ((size <= GET_SIZE(HDRP(bp))) && (GET_SIZE(HDRP(bp)) <  GET_SIZE(HDRP(bestFit))) ) {
            printf("djska\n");
            bestFit=bp;
        }
    }
    return bestFit;
}

void* mymalloc(size_t size){
    if(size == 0) return NULL;
    char* ptr = NULL;
    size_t adjustedSize;

    adjustedSize = DSIZE + MAX(2*DSIZE,size);
    if((adjustedSize % 8)!=0){
        adjustedSize = adjustedSize - (adjustedSize % 8) + DSIZE;
    }
    
    if(fit == 0 || fit == 1){
        ptr = first_next_fit(size);
    }
    else{ // fit == 2 
        ptr = best_fit(size);
    }
    // TODO split the free block at ptr and update both new blocks
    splitAndInsert(ptr, adjustedSize);
    return ptr;
}

void mycleanup(){
    free(myHeap);
}

void myfree(void* ptr){
    if(ptr == NULL) return;
    // trying to free an address not in heap
    if ((uintptr_t)ptr < (uintptr_t)myHeap || 
        (uintptr_t)ptr >= (uintptr_t)myHeap + (uintptr_t)INITIAL_HEAP_SIZE){
            printf("error: not a heap pointer\n");
            return;
    }
    // if not valid malloc address
    if( (uintptr_t)((ptr)) % 8 != 0) {
        printf("error: not a malloced address");
        return;
    }
    // if already free
    if( GET_ALLOC(HDRP((char*) ptr)) == 0){
        printf("error: double free");
        return;
    }

    used_space -= GET_SIZE(HDRP(ptr));
    // coalescing
    size_t prev = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));
    size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    char *header = HDRP(ptr);
    char *footer = FTRP(ptr);
    // CASE 1: neither block is free (change nothing)
    if(prev && next) { /**/ }
    // CASE 2: only next is free
    else if(prev && !next) {
        size += next_size;
        PUT8(FTRP(ptr), NULL);  // overwrites full 4 bytes of footer
        int* n_header = (int*)HDRP(NEXT_BLKP(ptr));
        n_header = NULL;
    }
    // CASE 3: only prev is free
    else if(!prev && next) {
        size += prev_size;
        header = HDRP(PREV_BLKP(ptr));
        ptr = PREV_BLKP(ptr);
        // remove: prev's footer, curr's header
        int* p_footer = (int*)FTRP(PREV_BLKP(ptr));
        p_footer = NULL;
        int* c_header = (int*)HDRP(ptr);
        c_header = NULL;
        
    }
    // CASE 4: both blocks are free
    else {
        size += prev_size + GET_SIZE(FTRP(NEXT_BLKP(ptr)));
        header = HDRP(PREV_BLKP(ptr));
        footer = FTRP(NEXT_BLKP(ptr));
        ptr = PREV_BLKP(ptr);
        // TODO remove: prev's footer, curr's header & footer, next's header
        int* p_footer = (int*)FTRP(PREV_BLKP(ptr));
        p_footer = NULL;
        int* c_header = (int*)HDRP(ptr);
        c_header = NULL;
        int* c_footer = (int*)FTRP(ptr);
        c_footer = NULL;
        int* n_header = (int*)HDRP(NEXT_BLKP(ptr));
        n_header = NULL;
    }
    PUT(header, PACK(size,0));
    PUT(footer, PACK(size,0));

}


int main(void) {
    myinit(2);
    int* a = mymalloc(sizeof(int));
    *a = 5;
    printf("address: %p, value: %d\n",a, *a);
    int *b = mymalloc(sizeof(int));
    *b=4;
    int *c = mymalloc(sizeof(int));
    *c=3;
    printf("address: %p, value: %d\n",a, *a);
    printf("address: %p, value: %d\n",b, *b);
    printf("address: %p, value: %d\n",c, *c);
    printf("alloc: %d\n", GET_ALLOC(HDRP(a)));
    myfree(b);
    b = mymalloc(sizeof(int));
    int *d=malloc(sizeof(int));
    myfree(d);
   
    mycleanup();
}
