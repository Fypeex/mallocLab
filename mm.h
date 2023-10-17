#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t bSize;

typedef struct BlockMetaData {
    bool isUsed:1;
    bSize other:31;
    bSize size;
} BlockMetaData;

typedef struct BlockData {
    BlockMetaData metaData;
    struct BlockData* next;
    struct BlockData* previous;
} BlockData;


typedef struct HeapData {
    BlockData* firstFreeBlock;
} HeapData;



extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
int validateHeap();
bool validateLL();
void resetBlock(BlockData *p);
BlockData *mergeWithPrev(BlockData *p);
void *cloneToEnd(BlockData* bd);
void *findBlock(size_t size);
void *splitBlock(BlockData *p, size_t size);
void *increaseHeap(size_t minSize);
BlockData *findLargestFreeBlock(BlockData*);
extern void *mm_realloc(void *ptr, size_t size);

BlockData* jumpToNext(BlockData* p);
BlockData* jumpToPrevious(BlockData* p) ;
BlockMetaData* jumpToEnd(BlockData* p);
BlockData* jumpToFront(BlockData* p);