#include <stdio.h>
#include <stdbool.h>

typedef unsigned long bSize;

typedef struct BlockMetaData {
    bool isUsed:1;
    unsigned long other:63;
} BlockMetaData;

typedef struct BlockData {
    BlockMetaData metaData;
    size_t size;
    struct BlockData* nextFreeBlock;
    struct BlockData* previousFreeBlock;
} BlockData;


typedef struct HeapData {
    BlockData* firstFreeBlock;
    BlockData* largestFreeBlock;
} HeapData;



extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
int validateHeap();

BlockData *mergeWithPrev(BlockData *p);
void *cloneToEnd(BlockData* bd);
void *findBlock(size_t size);
void *splitBlock(BlockData *p, size_t size);
void *increaseHeap(size_t minSize);
BlockData *findLargestFreeBlock(BlockData*);
extern void *mm_realloc(void *ptr, size_t size);

BlockData* jumpToNext(BlockData* p);
BlockData* jumpToPrevious(BlockData* p) ;
BlockData* jumpToEnd(BlockData* p);
BlockData* jumpToFront(BlockData* p);