/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


#define INITIAL_BLOCK_SIZE (32)
#define INITIAL_HEAP_SIZE (sizeof(HeapData))

int mm_init(void) {
    mem_init();
    void* p = mem_sbrk(INITIAL_HEAP_SIZE);
    void* b = mem_sbrk(INITIAL_BLOCK_SIZE + 2 * sizeof(BlockMetaData));

    HeapData* hd = (HeapData*) p;

    hd->firstFreeBlock = b;
    hd->largestFreeBlockSize = INITIAL_BLOCK_SIZE;


    return 0;
}

void *mm_malloc(size_t size) {

}

void *increaseHeap(int minSize) {

}

void *findBlock(size_t size) {

}

void *splitBlock(void *p, size_t size) {

}

void *addAsFreeBlock(BlockData* data) {

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *) ((char *) oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}



int main() {

    printf("%lu", sizeof((void *) 0));

}







