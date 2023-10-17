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
#define INITIAL_BLOCK_SIZE (40)
#define INITIAL_HEAP_SIZE (sizeof(HeapData))


void replaceNodeWithNextLargest(BlockData *node, BlockData *parent);

int mm_init(void) {
    mem_init();
    void *h = mem_sbrk(INITIAL_HEAP_SIZE);
    void *b = mem_sbrk(INITIAL_BLOCK_SIZE + BLOCK_METADATA_SIZE);

    BlockData *bd = (BlockData *) b;

    resetBlock(bd);
    bd->size = INITIAL_BLOCK_SIZE;

    cloneToEnd(bd);

    HeapData *hd = (HeapData *) h;
    hd->root = bd;

    return 0;
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
    bd->metaData.isUsed = true;
    if (isFromTree) {
        removeBlock(bd);
    }

    if (bd == NULL) return NULL;

    BlockData *split = splitBlock(bd, size);
    if (split != NULL) {
        addBlock(split, split->size);
    }

    //Got raw pointer, need to add to tree etc

    return bd + 1;

}

void insertBlockRecursive(BlockData *insertPoint, BlockData *bd, bSize size) {
    if (insertPoint->size == size) {
        while (insertPoint->nextBlock) insertPoint = insertPoint->nextBlock;
        insertPoint->nextBlock = bd;
    } else if (insertPoint->size > size) {
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
    if (p == (void *) -1) return NULL;
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

    removeBlock(prev);

    unsigned long newSize = prev->size + p->size + BLOCK_METADATA_SIZE;

    prev->size = newSize;
    cloneToEnd(prev);

    addBlock(prev, newSize);

    return prev;
}

void removeBlock(BlockData *node) {
    HeapData *hd = (HeapData *) mem_heap_lo();

    if(node == hd -> root) {
        //Node is only list in tree
        if(node -> left == NULL && node -> right == NULL) {
            hd -> root = node -> nextBlock;
            return;
        }

        //Node is root element
        if(node -> nextBlock == NULL) {
            //Only element
            //Find next biggest element and move it
            replaceNodeWithNextLargest(node, NULL);

        }else {
            node -> nextBlock -> left = node -> left;
            node -> nextBlock -> right = node -> right;

            hd -> root = node -> nextBlock;
        }

        return;
    }

    BlockData *list = hd->root;
    BlockData *parent = NULL;
    while (list->size != node->size) {
        parent = list;
        if (node->size > list->size) list = list->right;
        if (node->size < list->size) list = list->left;
    }



    BlockData *prev = NULL;
    while (list != node) {
        prev = list;
        list = list->nextBlock;
    }

    if(prev == NULL) {
        //First element of list
        if(list -> nextBlock == NULL) {
            // Only element of list
            replaceNodeWithNextLargest(list, parent);

        } else {
            list -> nextBlock -> left = list -> left;
            list -> nextBlock -> right = list -> right;

            if(parent -> left == list) {
                parent -> left = list -> nextBlock;
            }
            else {
                parent -> right = list -> nextBlock;
            }

        }
    } else {

        prev -> nextBlock = list -> nextBlock;

    }
}

void replaceNodeWithNextLargest(BlockData *node, BlockData *parent) {
    if(parent && node -> left == NULL && node -> right != NULL) {
        if(parent -> left == node) parent -> left = node -> right;
        else parent -> right = node -> right;
    }
    else if(parent && node -> left != NULL && node -> right == NULL) {
        if(parent -> left == node) parent -> left = node -> left;
        else parent -> right = node -> left;
    }
    else if(parent && node -> left == NULL && node -> right == NULL) {
        if(parent -> left == node) parent -> left = NULL;
        else parent -> right = NULL;
    } else {
        BlockData *nextLargest = node -> right;
        BlockData *par = node;
        while(nextLargest -> right != NULL) {
            par = nextLargest;
            nextLargest = nextLargest -> left;
        }

        if(nextLargest -> right != NULL) {
            par -> left = nextLargest -> right;
        }

        nextLargest -> left = node -> left;
        nextLargest -> right = node -> right;

        if(parent) {
            if (parent->left == node) {
                parent->left = nextLargest;
            } else {
                parent->right = nextLargest;
            }
        }

    }
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


void *cloneToEnd(BlockData *bd) {
    return memcpy(jumpToEnd(bd), bd, sizeof(BlockData));
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



int main() {
    mm_init();

    void *p1 = mm_malloc(40);
    void *p2 = mm_malloc(64);
    void *p3 = mm_malloc(88);
    void *p4 = mm_malloc(56);
    void *p5 = mm_malloc(80);
    void *p6 = mm_malloc(96);
}

