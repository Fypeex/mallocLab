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

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define BMD (sizeof(BlockMetaData))
#define BD (sizeof(BlockData))
#define TD (sizeof(TailData))
#define BDS (BMD + TD)
#define MINIMUM_BLOCK_SIZE (BD + TD)
#define INITIAL_BLOCK_SIZE (1 << 4)
#define INITIAL_HEAP_SIZE (sizeof(HeapData))
#define HEAP_DATA (HeapData*) mem_heap_lo()

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Common routine to set a block as empty
void setEmpty(BlockData *bd) {
    bd->left = bd->right = bd->next = NULL;
    bd->metaData.height = 0;
}

// Common routine to copy metadata to the end of the block
void copyToEnd(BlockMetaData *bd) {
    if (!bd->isUsed) {
        ((TailData *) ((char *) bd + bd->size + BMD))->front = (BlockData *) bd;
    } else {
        ((TailData *) ((char *) bd + BMD + bd->size))->front = bd;
    }
}

// Initializers and wrappers for insert and delete functions
void insertWrapper(BlockData *bd) {
    HeapData *hd = HEAP_DATA;
    insert(bd, &(hd->root));
}

void deleteWrapper(BlockData *bd) {
    HeapData *hd = HEAP_DATA;
    delete(bd, &(hd->root));
}

int mm_init(void) {
    mem_init();
    void *h = mem_sbrk(INITIAL_HEAP_SIZE);
    void *b = mem_sbrk(INITIAL_BLOCK_SIZE + BDS);

    BlockData *bd = (BlockData *) b;

    bd->metaData.size = INITIAL_BLOCK_SIZE;

    copyToEnd((BlockMetaData *) bd);

    HeapData *hd = (HeapData *) h;
    hd->root = bd;

    return 0;
}

BlockData *mergeBlocks(BlockData *bd1, BlockData *bd2) {
    deleteWrapper(bd1);
    deleteWrapper(bd2);

    bSize newSize = bd1->metaData.size + bd2->metaData.size + BDS;
    bd1->metaData.size = newSize;

    setEmpty(bd1);

    copyToEnd((BlockMetaData *) bd1);

    return bd1;
}

void *mm_malloc(size_t size) {
    HeapData *hd = HEAP_DATA;

    //Align size to byte address
    size = MAX(ALIGN(size), MINIMUM_BLOCK_SIZE);

    BlockData *freeBlock = findFirst(hd->root, size);

    if (freeBlock) {
        deleteWrapper(freeBlock);
        setEmpty(freeBlock);
    } else {
        freeBlock = increaseHeap(size);
    }

    BlockData *split = splitBlock(freeBlock, size);
    if (split) {
        split->metaData.isUsed = false;
        insertWrapper(split);
    }

    ((BlockMetaData *) (freeBlock))->isUsed = true;


    return (void *) ((char *) freeBlock + BMD);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    BlockMetaData *bmd = (BlockMetaData *) ((char *) ptr - BMD);

    if (!bmd->isUsed) return;

    BlockData *bd = (BlockData *) bmd;
    bd->metaData.isUsed = false;

    void *low = mem_heap_lo();
    void *high = mem_heap_hi();

    BlockMetaData *prev = ((TailData *) ((char *) bd - TD))->front;
    if ((char *) bd - TD - MINIMUM_BLOCK_SIZE > (char *) low + INITIAL_HEAP_SIZE && !prev->isUsed) {
        deleteWrapper(bd);
        bd = mergeBlocks((BlockData *) prev, bd);
    }

    BlockData *next = (BlockData *) (((char *) bd) + BDS + bd->metaData.size);
    if ((char *) next + TD <= (char *) high + 1 && !(next->metaData.isUsed)) {
        bd = mergeBlocks(bd, next);
    }

    setEmpty(bd);
    copyToEnd((BlockMetaData *) bd);

    insertWrapper(bd);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    copySize = ((BlockMetaData *) (((char *) ptr) - BMD))->size;
    // Check if next block is free and if we can merge

    newptr = mm_malloc(size);
    //If we realloc to a smaller size, we only copy size bytes

    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;

}

// rotates to the left
BlockData *rotate_left(BlockData *root) {
    if (root == NULL || root->right == NULL) {
        return root;
    }

    BlockData *right_child = root->right;
    root->right = right_child->left;
    right_child->left = root;

    root->metaData.height = 1 + MAX(height(root->left), height(root->right));
    right_child->metaData.height = 1 + MAX(height(right_child->left), height(right_child->right));

    return right_child;
}

BlockData *rotate_right(BlockData *root) {
    if (root == NULL || root->left == NULL) {
        return root;
    }

    BlockData *left_child = root->left;
    root->left = left_child->right;
    left_child->right = root;

    root->metaData.height = 1 + MAX(height(root->left), height(root->right));
    left_child->metaData.height = 1 + MAX(height(left_child->left), height(left_child->right));

    return left_child;
}


bSize height(BlockData *root) {
    if (root == NULL) {
        return 0;
    }
    return root->metaData.height;
}

bSize balance_factor(BlockData *root) {
    if (root == NULL) {
        return 0;
    }
    return height(root->left) - height(root->right);
}


BlockData *rebalance(BlockData *node) {
    if (node == NULL) return NULL;

    node->metaData.height = 1 + MAX(height(node->left), height(node->right));

    bSize balance = balance_factor(node);

    if (balance > 1) {
        if (balance_factor(node->left) >= 0) {
            return rotate_right(node);
        } else {
            node->left = rotate_left(node->left);
            return rotate_right(node);
        }
    } else {
        if (balance_factor(node->right) <= 0) {
            return rotate_left(node);
        } else {
            node->right = rotate_right(node->right);
            return rotate_left(node);
        }
    }

    return node;
}

// inserts a new node in the AVL tree
BlockData *insert(BlockData *bd, BlockData **root) {
    HeapData *hd = HEAP_DATA;

    if (hd->root == NULL) {
        hd->root = bd;
    } else if (hd->root->metaData.size == bd->metaData.size) {
        BlockData *curr = hd->root;
        while (curr->next != NULL) curr = curr->next;
        curr->next = bd;
    } else {
        BlockData *curr = hd->root;
        BlockData *parent = NULL;

        while (curr != NULL && curr->metaData.size != bd->metaData.size) {
            parent = curr;
            if (curr->metaData.size > bd->metaData.size) curr = curr->left;
            else curr = curr->right;
        }
        if (curr == NULL) {
            if (bd->metaData.size > parent->metaData.size) parent->right = bd;
            else parent->left = bd;
        } else {
            while (curr->next) curr = curr->next;
            curr->next = bd;
        }

        *root = rebalance(*root);

        return *root;
    }
    return NULL;
}

// deletes a bd from the AVL tree
BlockData *delete(BlockData *bd, BlockData **root) {
    HeapData *hd = (HEAP_DATA);
    if (bd == hd->root) {
        if (hd->root->left != NULL && hd->root->right != NULL) {
            BlockData *swap = hd->root->left;
            BlockData *swapParent = hd->root;

            // Find largest node in the left subtree
            while (swap->right != NULL) {
                swapParent = swap;
                swap = swap->right;
            }

            // Bypass swap from its parent
            if (swapParent->right == swap) {
                swapParent->right = swap->left;
            } else {
                swapParent->left = swap->left;
            }

            // Update swap to point to root's children
            swap->left = hd->root->left;
            swap->right = hd->root->right;

            // Update hd->root to point to swap
            hd->root = swap;
        } else if ((hd->root->left == NULL) ^ (hd->root->right == NULL)) {
            hd->root = hd->root->left == NULL ? hd->root->right : hd->root->left;
        } else {
            hd->root = NULL;
        }
    } else {
        BlockData *curr = hd->root;
        BlockData *parent = NULL;
        BlockData *pred = NULL;

        while (curr != NULL && curr->metaData.size != bd->metaData.size) {
            parent = curr;
            if (curr->metaData.size > bd->metaData.size) curr = curr->left;
            else curr = curr->right;
        }
        if (curr == NULL) return NULL;

        while (curr != NULL && curr != bd) {
            pred = curr;
            curr = curr->next;
        }
        if (curr == NULL) return NULL;

        if (pred == NULL) {
            //Is first element of list
            if (curr->next == NULL) {
                if (curr->left == NULL && curr->right == NULL) {
                    if (parent->left == curr) parent->left = NULL;
                    else parent->right = NULL;
                } else if ((curr->left == NULL) ^ (curr->right == NULL)) {
                    if (parent->left == curr) parent->left = curr->left ? curr->left : curr->right;
                    else parent->right = curr->left ? curr->left : curr->right;
                } else {
                    BlockData *swap = curr->left;
                    BlockData *swapParent = NULL;
                    while (swap->right != NULL) {
                        swapParent = swap;
                        swap = swap->right;
                    }

                    if (swapParent == NULL) {
                        //Immediate left of current
                        if (parent->right == curr) {
                            parent->right = swap;

                            swap->right = curr->right;
                        } else {
                            parent->left = swap;
                            swap->right = curr->right;
                        }
                    } else {
                        swapParent->right = swap->left;
                        swap->left = curr->left;
                        swap->right = curr->right;
                        if (parent->left == curr) parent->left = swap;
                        else parent->right = swap;
                    }
                }

            } else {
                if (parent->left == bd) {
                    parent->left = curr->next;
                } else {
                    parent->right = curr->next;
                }
                curr->next->metaData.height = curr->metaData.height;
                curr->next->left = curr->left;
                curr->next->right = curr->right;
            }
        } else {
            pred->next = curr->next;
        }
    }

    if (hd->root != NULL) {
        *root = rebalance(*root);
    }
    return *root;
}


BlockData *abc(BlockData *root, bSize size) {
    if (root == NULL) return NULL;
    // If current block is bigger or equal, search both sides
    if (root->metaData.size >= size) {
        if (root->left == NULL || root->left->metaData.size < size) return root;
            //Check if left is tighter
        else {
            return findFirst(root->left, size);
        }
    }
        // If current block is smaller, only search the right
    else {
        return findFirst(root->right, size);
    }
}

BlockData *findFirst(BlockData *root, bSize size) {
    while(root != NULL && root -> metaData.size < size) root = root -> right;

    return root;
}

BlockData *splitBlock(BlockData *bd, bSize size) {
    if (bd == NULL) return NULL;
    if (bd->metaData.size <= size + MINIMUM_BLOCK_SIZE) return NULL;

    bSize oldSize = bd->metaData.size;

    bd->metaData.size = size;
    copyToEnd((BlockMetaData *) bd);

    BlockData *split = (BlockData *) (((char *) bd + bd->metaData.size + BMD + TD));
    split->metaData.size = oldSize - size - BDS;
    setEmpty(split);
    copyToEnd((BlockMetaData *) split);
    return split;
}

BlockData *increaseHeap(bSize size) {
    bSize min = size > 1 << 13 ? size : 1 << 13;

    BlockData *bd = (BlockData *) mem_sbrk(min + BDS);

    bd->metaData.size = min;
    setEmpty(bd);
    copyToEnd((BlockMetaData *) bd);

    return mergeWithPrevious(bd);
}

BlockData *mergeWithPrevious(BlockData *bd) {
    if (bd == NULL) return bd;

    BlockData *prev = ((TailData *) (((char *) bd) - TD))->front;
    if (prev == NULL) return bd;
    if (prev->metaData.isUsed) return bd;

    deleteWrapper(prev);
    setEmpty(prev);

    bSize newSize = bd->metaData.size + prev->metaData.size + BDS;

    prev->metaData.size = newSize;

    copyToEnd((BlockMetaData *) prev);

    return prev;
}

