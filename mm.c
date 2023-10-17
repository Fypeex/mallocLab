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
#define BLOCK_METADATA_SIZE (2 * sizeof(BlockData))
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
    bd->size = INITIAL_BLOCK_SIZE;
    bd->metaData.isUsed = false;
    bd->left = NULL;
    bd->right = NULL;

    cloneToEnd(bd);

    return 0;
}

void *cloneToEnd(BlockData *bd) {
    return memcpy(jumpToEnd(bd), bd, sizeof(BlockData));
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
    if (b == (void *) -1) return NULL;
    BlockData *bd = (BlockData *) b;

    //Mark block as used
    bd->metaData.isUsed = true;

    //If block can be split into two, do that
    BlockData *newBlock = NULL;
    if (bd->size >= size + MINIMUM_BLOCK_SIZE) {
        newBlock = splitBlock(bd, size);
    }

    //Rewire LL
    if (bd->left) bd->left->right = bd->right;
    if (bd->right) bd->right->left = bd->left;

    //If b was firstFreeBlock in LL, set it to next
    if (hd->firstFreeBlock == bd) hd->firstFreeBlock = bd->left;

    //Remove refs from block
    bd->left = NULL;
    bd->right = NULL;

    //Clone bd data to end
    cloneToEnd(bd);

    //If split block exists, add to start of LL
    if (newBlock) {
        newBlock->left = hd->firstFreeBlock;
        if (hd->firstFreeBlock) hd->firstFreeBlock->right = newBlock;
        hd->firstFreeBlock = newBlock;
    }
    return (void *) (bd + 1);
}

void resetBlock(BlockData *p) {
    p->metaData.isUsed = false;
    p->metaData.other = 0;
    p->size = 0;
    p->left = NULL;
    p->right = NULL;
}

void *increaseHeap(size_t minSize) {
    bSize newSize = minSize > (2 << 12) ? minSize : (2 << 12);

    void *p = mem_sbrk(newSize + BLOCK_METADATA_SIZE);
    if (p == (void *) -1) return p;
    BlockData *pd = (BlockData *) p;
    resetBlock(pd);

    pd->size = newSize;
    cloneToEnd(pd);

    return (void*) mergeWithPrev((BlockData *) p);
}

BlockData *mergeWithPrev(BlockData *p) {
    HeapData *hd = (HeapData *) mem_heap_lo();

    BlockData *prev = jumpToPrevious(p);
    if(prev == NULL) return p;

    if (prev->metaData.isUsed) return p;

    unsigned long newSize = prev->size + p->size + BLOCK_METADATA_SIZE;

    prev->size = newSize;
    cloneToEnd(prev);

    //Remove p from LL to confirm merge
    if (p->left) p->left->right = p->right;
    if (p->right) p->right->left = p->left;

    return prev;
}


void *findBlock(size_t size) {
    void *p = mem_heap_lo();
    HeapData *hd = (HeapData *) p;

    if (hd->firstFreeBlock == NULL) return NULL;
    BlockData *curr = hd->firstFreeBlock;

    while (curr->size < size && curr->left != NULL) {
        curr = curr->left;
    }

    if (curr->size < size) return NULL;
    return curr;
}

void *splitBlock(BlockData *p, size_t size) {
    if (p->size < size + BLOCK_METADATA_SIZE) return (void *) -1;

    bSize sizeBefore = p->size;

    //Set to new size
    p->size = size;

    //Extract new block
    BlockData *newBlock = jumpToNext(p);

    //Reset data in new block
    resetBlock(newBlock);

    //Set metadata in new block
    newBlock->size = sizeBefore - size - BLOCK_METADATA_SIZE;
    newBlock->metaData.isUsed = false;

    //Clone both to end
    cloneToEnd(p);
    cloneToEnd(newBlock);

    return newBlock;
}

//b1 < b2
BlockData *mergeBlocks(BlockData *b1, BlockData *b2) {
    HeapData *hd = mem_heap_lo();

    bSize newTotalSize = b1->size + b2->size + BLOCK_METADATA_SIZE;

    b1->size = newTotalSize;

    //Remove b1 from the LL
    if (b1->right) {
        b1->right->left = b1->left;
    }
    if (b1->left) {
        b1->left->right = b1->right;
    }



    //Remove b2 from LL
    if (b2->right) {
        b2->right->left = b2->left;
    }
    if (b2->left) {
        b2->left->right = b2->right;
    }

    //If b1 or b2 was head, set next to head
    if (hd->firstFreeBlock == b1 || hd->firstFreeBlock == b2) {
        hd->firstFreeBlock = b1->left ? b1->left : b2->left;
    }

    b1->left = NULL;
    b1->right = NULL;
    b2->left = NULL;
    b2->right = NULL;


    cloneToEnd(b1);
    return b1;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    static int i = 0;

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
    char *high = (char*) mem_heap_hi();

    //jumpToPrevious is not safe to use unless we know for sure that the previous block is inside the scope
    BlockData *prev = jumpToPrevious(bd);
    if (prev && (void*) (((char*) (bd-1)) - sizeof(HeapData)) >= low && !(prev->metaData.isUsed)) {
        bd = mergeBlocks(prev, bd);
    }


    //Jump to next is always safe to use if bd is actual blockdata
    BlockData *next = jumpToNext(bd);
    if ((char *) (next + 1) <= high + 1&& !(next->metaData.isUsed)) {
        bd = mergeBlocks(bd, next);
    }

    //Then we add bd to the LL
    if (hd->firstFreeBlock != bd) bd->left = hd->firstFreeBlock;
    if (hd->firstFreeBlock != NULL) hd->firstFreeBlock->right = bd;
    bd->right = NULL;
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
    copySize = (((BlockData *) ptr) - 1)->size;

    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}

int validateHeap() {
    BlockData *start = (BlockData *) (((char *) mem_heap_lo()) + sizeof(HeapData));
    int nrOfBlocks = 1;

    BlockData* prv = start;
    BlockData* curr = NULL;

    while (((void *) (curr = jumpToNext(prv))) < mem_heap_hi()) {
        if (nrOfBlocks == 340) {
            printf("Reached");
        }
        int diff = (int) (((char*) jumpToNext(curr)) - ((char*) curr));
        if(curr->metaData.isUsed == false) {
            printf("Block start: %p \n Block end: %p\n Diff: %d\n Is valid: %b\n Is used: %b \n Size: %ld\n Index: %d\n Other stuff: %d \n ----------------------------------------------\n",
                   curr,
                   jumpToNext(curr),
                   diff,
                   diff == curr->size + BLOCK_METADATA_SIZE && curr->metaData.other == 0,
                   curr->metaData.isUsed,
                   curr->size,
                   nrOfBlocks,
                   curr->metaData.other
            );
        }
        if(diff != curr->size + BLOCK_METADATA_SIZE || curr->metaData.other != 0) {
            printf("INVALID");
        }
        prv = curr;
        nrOfBlocks++;
    }

    return nrOfBlocks;
}
bool validateLL() {
    HeapData *hd = (HeapData*) mem_heap_lo();

    BlockData *firstBlock = hd->firstFreeBlock;

    BlockData *current = firstBlock;

    while(current->left != NULL) {
        if(current->metaData.isUsed) {
            return false;
        }
        if(current->right == NULL && current != firstBlock) {
            return false;
        }
        if(current->metaData.other != 0) {
            return false;
        }

        current = current->left;
    }


    //Check block structure
    firstBlock = (BlockData *) (hd + 1);
    BlockData *prev = NULL;
    current = firstBlock;

    while(current < (BlockData *) mem_heap_hi()) {
        if(prev != NULL) {
            if(prev->metaData.isUsed == false && current->metaData.isUsed == false) {
                return false;
            }

            int distance = (int) (current - prev);
            int gap = distance - (prev->size + BLOCK_METADATA_SIZE);
            if(gap > 0 && (gap < MINIMUM_BLOCK_SIZE || gap % 8 != 1)) {
                return false;
            }
        }

        if(current->metaData.other != 0) {
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
    return (BlockData *) (((char *) (p + 2)) + p->size);
}

BlockData *jumpToPrevious(BlockData *p) {
    if(((char*) (p - 2)) - sizeof(HeapData) <= (char*) mem_heap_lo()) return NULL;
    bSize prevSize = (p - 1)->size;
    return (BlockData *) (((char *) (p - 2)) - prevSize);
}

BlockData *jumpToEnd(BlockData *p) {
    return (BlockData *) (((char *) (p + 1)) + p->size);
}








