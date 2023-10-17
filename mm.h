#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t bSize;

typedef struct BlockMetaData {
    bool isUsed;
    bSize other: 31;
} BlockMetaData;

typedef struct BlockData {
    BlockMetaData metaData;
    bSize size;
    struct BlockData *nextBlock;
    struct BlockData *left;
    struct BlockData *right;

} BlockData;

typedef struct HeapData {
    BlockData *root;
} HeapData;


extern int mm_init(void);

extern void *mm_malloc(size_t size);

extern void mm_free(void *ptr);

int validateHeap();

bool validateLL();

void resetBlock(BlockData *p);

BlockData *mergeWithPrev(BlockData *p);

void *cloneToEnd(BlockData *bd);

BlockData *findExactFit(BlockData *start, size_t size);

BlockData *findFirstFit(BlockData *start, size_t size);

void *splitBlock(BlockData *p, size_t size);

void addBlock(BlockData *bd, bSize size);
void removeBlock(BlockData *node);

BlockData *increaseHeap(size_t minSize);

BlockData *findLargestFreeBlock(BlockData *);

extern void *mm_realloc(void *ptr, size_t size);

BlockData *jumpToNext(BlockData *p);

BlockData *jumpToPrevious(BlockData *p);

BlockData *jumpToEnd(BlockData *p);

BlockData *jumpToFront(BlockData *p);