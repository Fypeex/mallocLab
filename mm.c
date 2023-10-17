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
#include <string.h>
#include <stdlib.h>
#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define BLOCK_METADATA_SIZE (sizeof(BlockMetaData) + sizeof(BlockData)) //32B
#define MINIMUM_BLOCK_SIZE (BLOCK_METADATA_SIZE)
#define INITIAL_BLOCK_SIZE (1024)
#define INITIAL_HEAP_SIZE (sizeof(HeapData))


int mm_init(void) {
    mem_init();
    void *p = mem_sbrk(INITIAL_HEAP_SIZE);
    void *b = mem_sbrk(INITIAL_BLOCK_SIZE + BLOCK_METADATA_SIZE);

    HeapData *hd = (HeapData *) p;

    hd->firstFreeBlock = b;

    BlockData *bd = (BlockData *) b;
    bd->metaData.size = INITIAL_BLOCK_SIZE;
    bd->metaData.isUsed = false;
    bd->next = NULL;
    bd->previous = NULL;

    cloneToEnd(bd);

    return 0;
}

void *cloneToEnd(BlockData *bd) {
    return memcpy(jumpToEnd(bd), (BlockMetaData*) bd, sizeof(BlockMetaData));
}

void *mm_malloc(size_t size) {
    //Align size to byte address
    size = ALIGN(size);

    //Get heapdata from start of heap
    void *p = mem_heap_lo();
    HeapData *hd = (HeapData *) p;

    //Get block pointer
    void *b;
    b = findBlock(size);
    if (b == NULL) {
        b = increaseHeap(size);
    }

    //Cast to BlockData
    if (b == NULL) return NULL;
    BlockData *bd = (BlockData *) b;

    //Mark block as used
    bd->metaData.isUsed = true;

    //If block can be split into two, do that
    BlockData *newBlock = NULL;
    if (bd->metaData.size >= size + MINIMUM_BLOCK_SIZE) {
        newBlock = splitBlock(bd, size);
    }

    //Rewire LL
    if (bd->next) bd->next->previous = bd->previous;
    if (bd->previous) bd->previous->next = bd->next;

    //If b was firstFreeBlock in LL, set it to next
    if (hd->firstFreeBlock == bd) hd->firstFreeBlock = bd->next;

    //Remove refs from block
    bd->next = NULL;
    bd->previous = NULL;

    //Clone bd data to end
    cloneToEnd(bd);

    //If split block exists, add to start of LL
    if (newBlock) {
        newBlock->next = hd->firstFreeBlock;
        if (hd->firstFreeBlock) hd->firstFreeBlock->previous = newBlock;
        hd->firstFreeBlock = newBlock;
    }
    return (void *) (bd + 1);
}

void resetBlock(BlockData *p) {
    p->metaData.isUsed = false;
    p->metaData.other = 0;
    p->metaData.size = 0;
    p->next = NULL;
    p->previous = NULL;
}

void *increaseHeap(size_t minSize) {
    bSize newSize = minSize > (2 << 12) ? minSize : (2 << 12);

    void *p = mem_sbrk(newSize + BLOCK_METADATA_SIZE);
    if (p == (void *) -1) return NULL;
    BlockData *pd = (BlockData *) p;
    resetBlock(pd);

    pd->metaData.size = newSize;
    cloneToEnd(pd);

    return (void *) mergeWithPrev((BlockData *) p);
}

BlockData *mergeWithPrev(BlockData *p) {
    BlockData *prev = jumpToPrevious(p);
    if (prev == NULL) return p;

    if (prev->metaData.isUsed) return p;

    unsigned long newSize = prev->metaData.size + p->metaData.size + BLOCK_METADATA_SIZE;

    prev->metaData.size = newSize;
    cloneToEnd(prev);

    //Remove p from LL to confirm merge
    if (p->next) p->next->previous = p->previous;
    if (p->previous) p->previous->next = p->next;

    return prev;
}


void *findBlock(size_t size) {
    void *p = mem_heap_lo();
    HeapData *hd = (HeapData *) p;

    if (hd->firstFreeBlock == NULL) return NULL;
    BlockData *curr = hd->firstFreeBlock;

    while (curr->metaData.size < size && curr->next != NULL) {
        curr = curr->next;
    }

    if (curr->metaData.size < size) return NULL;
    return curr;
}

void *splitBlock(BlockData *p, size_t size) {
    if (p->metaData.size < size + BLOCK_METADATA_SIZE) return NULL;

    bSize sizeBefore = p->metaData.size;

    //Set to new size
    p->metaData.size = size;

    //Extract new block
    BlockData *newBlock = jumpToNext(p);

    //Reset data in new block
    resetBlock(newBlock);

    //Set metadata in new block
    newBlock->metaData.size = sizeBefore - size - BLOCK_METADATA_SIZE;
    newBlock->metaData.isUsed = false;

    //Clone both to end
    cloneToEnd(p);
    cloneToEnd(newBlock);

    return newBlock;
}

//b1 < b2
BlockData *mergeBlocks(BlockData *b1, BlockData *b2) {
    HeapData *hd = mem_heap_lo();

    bSize newTotalSize = b1->metaData.size + b2->metaData.size + BLOCK_METADATA_SIZE;

    b1->metaData.size = newTotalSize;

    //Remove b1 from the LL
    if (b1->previous) {
        b1->previous->next = b1->next;
    }
    if (b1->next) {
        b1->next->previous = b1->previous;
    }

    //Remove b2 from LL
    if (b2->previous) {
        b2->previous->next = b2->next;
    }
    if (b2->next) {
        b2->next->previous = b2->previous;
    }

    //If b1 or b2 was head, set next to head
    if (hd->firstFreeBlock == b1 || hd->firstFreeBlock == b2) {
        hd->firstFreeBlock = b1->next ? b1->next : b2->next;
    }

    b1->next = NULL;
    b1->previous = NULL;
    b2->next = NULL;
    b2->previous = NULL;


    cloneToEnd(b1);
    return b1;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    BlockData *bd = (BlockData *) ptr;
    bd--;

    if (!bd->metaData.isUsed) {
        return;
    }

    bd->metaData.isUsed = false;
    void *h = mem_heap_lo();
    HeapData *hd = (HeapData *) h;

    //Check if block before/after can be merged
    void *low = h;
    char *high = (char *) mem_heap_hi();

    //jumpToPrevious is not safe to use unless we know for sure that the previous block is inside the scope
    BlockData *prev = jumpToPrevious(bd);
    if (prev && (char *) prev >= ((char *) low) + sizeof(HeapData) && !(prev->metaData.isUsed)) {
        bd = mergeBlocks(prev, bd);
    }


    //Jump to next is always safe to use if bd is actual blockdata
    BlockData *next = jumpToNext(bd);
    if ((char *) (next + 1) <= high + 1 && !(next->metaData.isUsed)) {
        bd = mergeBlocks(bd, next);
    }

    //Then we add bd to the LL
    if (hd->firstFreeBlock != bd) bd->next = hd->firstFreeBlock;
    if (hd->firstFreeBlock != NULL) hd->firstFreeBlock->previous = bd;
    bd->previous = NULL;
    hd->firstFreeBlock = bd;
    cloneToEnd(bd);
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

    //If we realloc to a smaller size, we only copy size bytes
    copySize = (((BlockData *) ptr) - 1)->metaData.size;

    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}

int validateHeap() {
    BlockData *start = (BlockData *) (((char *) mem_heap_lo()) + sizeof(HeapData));
    int nrOfBlocks = 1;

    BlockData *prv = start;
    BlockData *curr = NULL;

    while (((void *) (curr = jumpToNext(prv))) < mem_heap_hi()) {
        if (nrOfBlocks == 340) {
            printf("Reached");
        }
        int diff = (int) (((char *) jumpToNext(curr)) - ((char *) curr));
        if (curr->metaData.isUsed == false) {
            printf("Block start: %p \n Block end: %p\n Diff: %d\n Is valid: %b\n Is used: %b \n Size: %u\n Index: %d\n Other stuff: %d \n ----------------------------------------------\n",
                   curr,
                   jumpToNext(curr),
                   diff,
                   diff == curr->metaData.size + BLOCK_METADATA_SIZE && curr->metaData.other == 0,
                   curr->metaData.isUsed,
                   curr->metaData.size,
                   nrOfBlocks,
                   curr->metaData.other
            );
        }
        if (diff != curr->metaData.size + BLOCK_METADATA_SIZE || curr->metaData.other != 0) {
            printf("INVALID");
        }
        prv = curr;
        nrOfBlocks++;
    }

    return nrOfBlocks;
}

bool validateLL() {
    HeapData *hd = (HeapData *) mem_heap_lo();

    BlockData *firstBlock = hd->firstFreeBlock;

    BlockData *current = firstBlock;

    while (current->next != NULL) {
        if (current->metaData.isUsed) {
            return false;
        }
        if (current->previous == NULL && current != firstBlock) {
            return false;
        }
        if (current->metaData.other != 0) {
            return false;
        }

        current = current->next;
    }


    //Check block structure
    firstBlock = (BlockData *) (hd + 1);
    BlockData *prev = NULL;
    current = firstBlock;

    while (current < (BlockData *) mem_heap_hi()) {
        if (prev != NULL) {
            if (prev->metaData.isUsed == false && current->metaData.isUsed == false) {
                return false;
            }

            int distance = (int) (current - prev);
            int gap = distance - (prev->metaData.size + BLOCK_METADATA_SIZE);
            if (gap > 0 && (gap < MINIMUM_BLOCK_SIZE || gap % 8 != 1)) {
                return false;
            }
        }

        if (current->metaData.other != 0) {
            return false;
        }

        prev = current;
        current = jumpToNext(current);
    }
    return true;
}

int run() {
    void *a0 = mm_malloc(2040);
    void *a1 = mm_malloc(2040);
    mm_free(a1);
    void *a2 = mm_malloc(48);
    void *a3 = mm_malloc(4072);
    mm_free(a3);
    void *a4 = mm_malloc(4072);
    mm_free(a0);
    mm_free(a2);
    void *a5 = mm_malloc(4072);
    mm_free(a4);
    mm_free(a5);
    return 0;
}

BlockData *jumpToNext(BlockData *p) {
    return (BlockData *) (((char *) p) + BLOCK_METADATA_SIZE + p->metaData.size);
}

BlockData *jumpToPrevious(BlockData *p) {
    if (((char *) p - BLOCK_METADATA_SIZE) - sizeof(HeapData) <= (char *) mem_heap_lo()) return NULL;
    bSize prevSize = (((BlockMetaData *) p) - 1)->size;
    return (BlockData *) (((char *) p) - BLOCK_METADATA_SIZE - prevSize);
}

BlockMetaData *jumpToEnd(BlockData *p) {
    return (BlockMetaData *) (((char *) (p + 1)) + p->metaData.size);
}


