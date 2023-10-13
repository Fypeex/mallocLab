#include <stdio.h>

typedef uintptr_t pSize;

typedef struct BlockMetaData {
    int32_t isUsed:1;
    int32_t otherStuff:31;
} BlockMetaData;

typedef struct BlockData {
    BlockMetaData metaData;
    int32_t size;
    struct BlockData* nextFreeBlock;
    struct BlockData* previousFreeBlock;
} BlockData;


typedef struct HeapData {
    BlockData* firstFreeBlock;
    int32_t largestFreeBlockSize;
    int32_t otherStuff;
} HeapData;



extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
void *cloneToEnd(BlockData* bd);
void *findBlock(size_t size);
void *splitBlock(void *p, size_t size);
void *increaseHeap(int minSize);
void *addAsFreeBlock(BlockData* data);
extern void *mm_realloc(void *ptr, size_t size);
