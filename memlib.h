#include <unistd.h>


void mem_init(void);               
void mem_deinit(void);
void *mem_sbrk(int incr);
void mem_reset_brk(void); 
void *mem_heap_lo(void);
void *mem_heap_hi(void);
void *findBlock(size_t size);
void *splitBlock(void *p, size_t size);
size_t mem_heapsize(void);
size_t mem_pagesize(void);

