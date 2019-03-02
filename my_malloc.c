#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdio.h>
#include "my_malloc.h"
#include <pthread.h>

//#define sizeof(mem_list) sizeof(struct mem_list)

mem_list *head = NULL;
mem_list *last = NULL;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
__thread mem_list* head_nolock = NULL;
__thread mem_list* last_nolock = NULL;

// Best Fit Policy
void *ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lock1);  // lock the thread 
  if (size <= 0) {
    pthread_mutex_unlock(&lock1);
    return NULL;
  }
  mem_list *target = NULL;
  mem_list * bf_block = NULL;
  int bf_size = INT_MAX;
  if (head) { // Not the first time
    mem_list *curr = head;
    while (curr) {
      if (curr->free && curr->size == size) { // find the block that has exactly the same size as the required malloc size
	bf_size = size;
	bf_block = curr;
	break;
      }
      if (curr->free && curr->size > size) { // find the best fit block
        if(curr->size < bf_size){
          bf_size = curr->size;
          bf_block = curr;          
        } 
      }   
      curr = curr->next;
    }
    target = bf_block;
    
    if (target) { // found the best fit block
      if(target->size - size > sizeof(mem_list)) { // could split
        mem_list * new_block;
	void * temp = (void*)target + size + sizeof(mem_list);
	new_block = (mem_list *)temp;
	new_block->free = 1;
	new_block->size = target->size - size - sizeof(mem_list);
	new_block->next = target->next;
	new_block->prev = target;
	target->size = size;
	if (!target->next) { // the splitting block is the last one
	  target->next = new_block;
	  last = new_block;
        }
        else { // the splitting block is not the last one
          target->next->prev = new_block;
	  target->next = new_block;  
	}
      }
      target->free = 0; // allocated memory
      pthread_mutex_unlock(&lock1);
      return target + 1;
    }
    else { // could not find the free block that match the new_size
      mem_list *new_tail = sbrk(sizeof(mem_list) + size);
      if (new_tail == (void *)(-1)) {
	pthread_mutex_unlock(&lock1);
        return NULL;
      }
      new_tail->size = size;
      new_tail->free = 0;
      last->next = new_tail;
      new_tail->prev = last;
      new_tail->next = NULL;
      last = new_tail;
      pthread_mutex_unlock(&lock1);
      return new_tail + 1;
    }
  }
  else { // head == NULL
    mem_list *curr = sbrk(sizeof(mem_list) + size);
    if (curr == (void *)(-1)) {
      pthread_mutex_unlock(&lock1);
      return NULL;
    }
    curr->size = size;
    curr->free = 0;
    curr->next = NULL;
    curr->prev = NULL;
    head = curr;
    last = curr;
    pthread_mutex_unlock(&lock1);
    return curr + 1;
  }
  pthread_mutex_unlock(&lock1);
}

// function to free a block of memory                                                                 
void ts_free_lock(void *ptr) {
  pthread_mutex_lock(&lock1);
  if (ptr == NULL) {
    pthread_mutex_unlock(&lock1);
    return;
  }
  mem_list *curr;
  curr = ptr - sizeof(mem_list);
  curr->free = 1;
  // merge with the previous block if possible

  if (curr->prev != NULL && curr->prev->free) {
    curr->prev->size += curr->size + sizeof(mem_list);
      if (curr->next) {
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
      }
      else {
        curr->prev->next = NULL;
        last = curr->prev;
      }
      curr = curr->prev;
  }
  // merge with the next block if possible                                                                                               
  if (curr->next != NULL && curr->next->free) {
      curr->size += sizeof(mem_list) + curr->next->size;
      if (curr->next->next) {
        curr->next = curr->next->next;
        curr->next->prev = curr;
      }
      else {
        curr->next = NULL;
        last = curr;
      }
  }
  pthread_mutex_unlock(&lock1);
}

void *ts_malloc_nolock(size_t size) {
  if (size <= 0) {
    return NULL;
  }
  mem_list *target = NULL;
  mem_list *bf_block = NULL;
  int bf_size = INT_MAX;
  if (head_nolock) { // Not the first time
    mem_list *curr = head_nolock;
    while (curr) {
      if (curr->free && curr->size == size) { // find the block that has exactly the same size as the required malloc size
	bf_size = size;
	bf_block = curr;
	break;
      }
      if (curr->free && curr->size > size) { // find the best fit block
        if(curr->size < bf_size){
          bf_size = curr->size;
          bf_block = curr;          
        } 
      }   
      curr = curr->next;
    }
    target = bf_block;
    if (target) { // found the best fit block
      if(target->size - size > sizeof(mem_list)) { // could split
        mem_list * new_block;
	void * temp = (void*)target + size + sizeof(mem_list);
	new_block = (mem_list *)temp;
	new_block->size = target->size - size - sizeof(mem_list);
	new_block->free = 1;
	//	new_block->next = target->next;
	new_block->prev = target;
	target->size = size;
	if (!target->next) { // the splitting block is the last one
	  target->next = new_block;
	  new_block->next = NULL;
	  last_nolock = new_block;	  
        }
        else { // the splitting block is not the last one
          target->next->prev = new_block;
	  new_block->next = target->next;
	  target->next = new_block;  
	}
      }
      target->free = 0; // allocated memory
      return target + 1;
    }
    else { // could not find the free block that match the new_size
      pthread_mutex_lock(&lock2); 
      mem_list *new_tail = sbrk(sizeof(mem_list) + size);
      if (new_tail == NULL) {
	pthread_mutex_unlock(&lock2);
	return NULL;
      }
      pthread_mutex_unlock(&lock2); 
      new_tail->size = size;
      new_tail->free = 0;
      last_nolock->next = new_tail;
      new_tail->prev = last_nolock;
      new_tail->next = NULL;
      last_nolock = new_tail;
      return new_tail + 1;
    }
  }
  else { // head == NULL
    pthread_mutex_lock(&lock2); 
    mem_list *curr = sbrk(sizeof(mem_list) + size);
    if (curr == NULL) {
      pthread_mutex_unlock(&lock2);
      return NULL;
    }
    pthread_mutex_unlock(&lock2); 
    curr->size = size;
    curr->free = 0;
    curr->next = NULL;
    curr->prev = NULL;
    head_nolock = curr;
    last_nolock = curr;
    return curr + 1;
  }
}


// function to free a block of memory                                                                                         
void ts_free_nolock(void *ptr) {
  if (ptr == NULL) {
    return;
  }
  mem_list *curr = head_nolock;
  curr = ((mem_list *)ptr - 1);
  curr->free = 1;
}
