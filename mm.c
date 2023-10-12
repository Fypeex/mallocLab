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
#define METADATA_SIZE (2 * sizeof(BlockData))
#define INITIAL_HEAP_SIZE (METADATA_SIZE + 8)

typedef uint64_t pSize;

typedef struct {
    pSize used: 1;
    pSize somethingElse: 63;
} Metadata;

typedef struct BlockData {
    Metadata data;
    pSize size;
    struct BlockData* next;
    struct BlockData* previous;
} BlockData;

typedef struct {
    BlockData* firstFree;
    BlockData* firstUsed;
} HeapData;


int mm_init(void) {
    mem_init();
    void *start = mem_sbrk(16);

    HeapData *startdata = (HeapData *) start;
    startdata->firstFree = 0;
    startdata->firstUsed = 0;

    //Other 8 bytes are for search results

    //Initialize first block
    void *firstFree = mem_sbrk(INITIAL_HEAP_SIZE);
    BlockData *front = (BlockData *) firstFree;

    //Set metadata of first block
    front->size = INITIAL_HEAP_SIZE - METADATA_SIZE;
    front->previous = 0;
    front->next = 0;
    front->data.used = 0;
    front->data.somethingElse = 0;

    BlockData *end = (BlockData *) (((char *) front) + INITIAL_HEAP_SIZE - sizeof(BlockData));
    *end = *front;

    startdata -> firstFree = front;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//void *mm_malloc(size_t size)
//{
//    int newsize = ALIGN(size + SIZE_T_SIZE);
//    void *p = mem_sbrk(newsize);
//
//    //mem_sbrk returns (void *) -1 if it failed
//    if (p == (void *)-1)
//	    return NULL;
//    else {
//        //Store the size of the block in the byte pointed at by p
//        *(size_t *)p = size;
//
//        //Skip the amount of bytes that are necessary to store the size
//        //Return the next free bytes adress
//
//        //In short: Store the blocks size and return the next free address
//        return (void *)((char *)p + SIZE_T_SIZE);
//    }
//}

void *mm_malloc(size_t size) {
    //Define general metadata
    int totalSize = ALIGN(size + METADATA_SIZE);
    //Align
    void *p = findBlock(totalSize);
    if (p == (void *) -1) {
        increase_heap(totalSize);
    }

    //Try again
    p = findBlock(totalSize);
    if (p == (void *) -1) {
        return NULL;
    }


    void *split = splitBlock(p, totalSize);

    BlockData *start = (BlockData *) p;
    start->size = totalSize;
    start->next = split;

    BlockData *endBlockOfPrevious = (BlockData *) (((char *) start) - sizeof(BlockData));
    start->previous = (BlockData *) (((char *) endBlockOfPrevious) - endBlockOfPrevious->size +sizeof(BlockData));

    start->data.used = 1;
    BlockData *end = (BlockData *) ((char *) p) + size + sizeof(BlockData);

    *end = *start;

    return (void *) ((char *) p + sizeof(BlockData));
}

void *increaseHeap(int minSize) {
    int newSize = minSize > 1024? minSize:1024;

    void* p = mem_sbrk(newSize);

    //


    HeapData* data = mem_heap_lo();
    data->firstFree

    return p;
}

void *findBlock(size_t size) {
    void *heap_lo = mem_heap_lo();
    HeapData *data = (HeapData *) heap_lo;
    if (data->firstFree == NULL) return (void *) -1;

    BlockData *firstFree = data->firstFree;

    while (firstFree->size < size && firstFree->next != 0) {
        firstFree = firstFree->next;
    }

    //If the next free block is not defined and the available size is not big enough return "error"
    if (firstFree->size < size) {
        return (void *) -1;
    }

    return (void *) firstFree;
}

void *splitBlock(void *p, size_t size) {
    BlockData *data = (BlockData *) (uintptr_t) p;

    BlockData *nextBlock = (BlockData *) (char *) p + size;

    nextBlock->size = data->size - size;
    nextBlock->previous = data->previous;
    nextBlock->next = data->next;

    return (pSize *) nextBlock;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    BlockData *data = (BlockData *) ptr;

    if (data->data.used == 0) {
        return;
    }

    HeapData *heapData = mem_heap_lo();

    data->data.used = 0;
    data->previous = 0;
    data->next = heapData->firstFree;
    heapData->firstFree = data;
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

    mm_init();

    mm_malloc(16);

}







