#include <stddef.h>

typedef struct mem_list{
  size_t size;
  struct mem_list * next;
  struct mem_list * prev;
  int free;  // 1 -> free; 0 -> not free
}mem_list;


void *bf_malloc(size_t size);
void bf_free(void* ptr);
unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();
//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread safe malloc/free: unlock version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);
