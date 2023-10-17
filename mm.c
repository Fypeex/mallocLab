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
    void *h = mem_sbrk(INITIAL_HEAP_SIZE);
    void *b = mem_sbrk(INITIAL_BLOCK_SIZE + BLOCK_METADATA_SIZE);

    BlockData *bd = (BlockData *) b;

    resetBlock(bd);
    bd->size = INITIAL_BLOCK_SIZE;

    HeapData *hd = (HeapData *) h;
    hd->root = bd;

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

    //Find block with exact size
    BlockData *bd = findExactFit(hd->root, size);
    bool isFromTree = true;
    if (bd == NULL) {
        bd = (hd->root != NULL) ? findFirstFit(hd->root, size) : NULL;

        if (bd == NULL) {
            bd = increaseHeap(size);
            isFromTree = false;
        }
    }


    if (bd == NULL) return NULL;

    BlockData *split = splitBlock(bd, size);
    if (split != NULL) {
        addBlock(split, split->size);
    }

    bd->metaData.isUsed = true;
    if (isFromTree) {
        removeBlock(bd);
    }
    //Got raw pointer, need to add to tree etc


    return bd + 1;

}

void insertBlockRecursive(BlockData *insertPoint, BlockData *bd, bSize size) {
    if(insertPoint->size == size) {
        while(insertPoint -> nextBlock) insertPoint = insertPoint -> nextBlock;
        insertPoint -> nextBlock = bd;
    }else if (insertPoint->size > size) {
        if (insertPoint->left == NULL) {
            insertPoint->left = bd;
        } else {
            insertBlockRecursive(insertPoint->left, bd, size);
        }
    } else {
        if (insertPoint->right == NULL) {
            insertPoint->right = bd;
        } else {
            insertBlockRecursive(insertPoint->right, bd, size);
        }
    }
}

void addBlock(BlockData *bd, bSize size) {
    HeapData *hd = mem_heap_lo();

    if (hd->root == NULL) {
        hd->root = bd;
    } else {
        insertBlockRecursive(hd->root, bd, size);
    }
}

void resetBlock(BlockData *p) {
    p->metaData.isUsed = false;
    p->metaData.other = 0;
    p->size = 0;
    p->nextBlock = NULL;
}

BlockData *increaseHeap(size_t minSize) {
    bSize newSize = minSize > (1 << 13) ? minSize : (1 << 13);

    void *p = mem_sbrk(newSize + BLOCK_METADATA_SIZE);
    if (p == (void *) -1) return p;
    BlockData *pd = (BlockData *) p;
    resetBlock(pd);

    pd->size = newSize;
    cloneToEnd(pd);

    return mergeWithPrev((BlockData *) p);
}

BlockData *mergeWithPrev(BlockData *p) {
    BlockData *prev = jumpToPrevious(p);
    if (prev == NULL) return p;

    if (prev->metaData.isUsed) return p;

    removeBlock(p);
    removeBlock(prev);

    unsigned long newSize = prev->size + p->size + BLOCK_METADATA_SIZE;

    prev->size = newSize;
    cloneToEnd(prev);

    removeBlock(prev);

    return prev;
}

void removeBlock(BlockData *node) {

}

BlockData *findExactFit(BlockData *start, size_t size) {
    if (start == NULL) return NULL;
    if (start->size == size) {
        BlockData *curr = start;

        while (curr != NULL && curr->metaData.isUsed) {
            curr = curr->nextBlock;
        }

        return curr;
    }
    if (start->size < size) return findExactFit(start->right, size);
    if (start->size > size) return findExactFit(start->left, size);
}

BlockData *findFirstFit(BlockData *start, size_t size) {
    if (start == NULL) return NULL;
    if (start->size >= size) {
        BlockData *curr = start;

        while (curr != NULL && curr->metaData.isUsed) {
            curr = curr->nextBlock;
        }

        if (curr == NULL) {
            return findFirstFit(start->right, size);
        }

        return curr;
    }
    if (start->size < size) return findFirstFit(start->right, size);
}

void *splitBlock(BlockData *p, size_t size) {
    if (p->size < size + BLOCK_METADATA_SIZE) return NULL;

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

    cloneToEnd(b1);
    return b1;
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

    //If we realloc to a smaller size, we only copy size bytes
    copySize = (((BlockData *) ptr) - 1)->size;

    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}

BlockData *jumpToNext(BlockData *p) {
    return (BlockData *) (((char *) (p + 2)) + p->size);
}

BlockData *jumpToPrevious(BlockData *p) {
    if (((char *) (p - 2)) - sizeof(HeapData) <= (char *) mem_heap_lo()) return NULL;
    bSize prevSize = (p - 1)->size;
    return (BlockData *) (((char *) (p - 2)) - prevSize);
}

BlockData *jumpToEnd(BlockData *p) {
    return (BlockData *) (((char *) (p + 1)) + p->size);
}






