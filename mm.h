#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t bSize;

typedef struct BlockMetaData {
    bool isUsed:1;
    bSize height: 31;
    bSize size;
} BlockMetaData;

typedef struct BlockData {
    BlockMetaData metaData;
    struct BlockData *left;
    struct BlockData *right;
    struct BlockData * next;
} BlockData;

typedef struct TailData {
    void *front;
} TailData;

typedef struct HeapData {
    BlockData *root;
} HeapData;


extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

BlockData *findFirst(BlockData* root, bSize  size);
bSize height(BlockData* root);
BlockData* insert(BlockData* bd, BlockData **root);
BlockData *delete(BlockData *bd, BlockData **root);

BlockData* splitBlock(BlockData* bd, bSize size);

BlockData *increaseHeap(bSize size);
BlockData *mergeWithPrevious(BlockData* bd);

void deleteWrapper(BlockData *bd);
void insertWrapper(BlockData *bd);