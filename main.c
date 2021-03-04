#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int has_initialized = 0;

#define MEM_SIZE (64*1024)
uint8_t heap[MEM_SIZE];

void *managed_memory_start; 

struct mem_control_block {
  int size;
  struct mem_control_block *next;
};

struct mem_control_block *free_list_start;      

void print_mem() {
  printf("\n------\n");
  struct mem_control_block* prev = (struct mem_control_block *)managed_memory_start;
  struct mem_control_block* mem = (struct mem_control_block *)managed_memory_start;
  while (1) {
    int is_in_free_list = 0;
    struct mem_control_block *curr = free_list_start;
    while(curr != NULL) {
      if (mem == curr) {
        is_in_free_list = 1;
        break;
      }
      curr = curr->next;
    }
    printf(" %d | addr: %p size: %d free: %d |",(int) ((uint8_t *)mem - (uint8_t*)prev), mem, mem->size, is_in_free_list);
    prev = mem;
    mem = (struct mem_control_block *)((uint8_t *)(mem + 1) + mem->size);
    if (mem >= (struct mem_control_block *)(managed_memory_start + MEM_SIZE) || mem <= (struct mem_control_block *)managed_memory_start) {
      break;
    }
  }
  printf("\n------\n");
}

void print_list(struct mem_control_block *list) {
  struct mem_control_block *curr = list;
  int counter = 0;
  while (1) {
    printf("%d: addr %p size %d next %p\n", counter++, curr, curr->size, curr->next);
    if (curr->next == (void *)0) {
      break;
    }
    curr = curr->next;
  }
}


void mymalloc_init() { 
  managed_memory_start = &heap;
  struct mem_control_block *m = (struct mem_control_block *)managed_memory_start;
  m->size = MEM_SIZE - sizeof(struct mem_control_block);
  m->next = (struct mem_control_block *)0;
  free_list_start = m;
  has_initialized = 1;
}

int round_up_to_multiple(int multiple, int num) {
  if (num % 8 == 0) {
    return num;
  } else {
    return num + (multiple - (num % multiple));
  }
}

void *mymalloc(long _numbytes) {
  long numbytes = round_up_to_multiple(8, _numbytes);
  if (has_initialized == 0) {
     mymalloc_init();
  }

  struct mem_control_block *curr_free_block = free_list_start;
  int new_used_block_tot_size = numbytes + sizeof(struct mem_control_block);
  while(curr_free_block) {
    if (curr_free_block->size >= new_used_block_tot_size) {
      void *curr_block_end_addr = (uint8_t *)curr_free_block + sizeof(struct mem_control_block) + curr_free_block->size;
      struct mem_control_block *new_used_block = (struct mem_control_block *)(curr_block_end_addr - new_used_block_tot_size);
      new_used_block->size = numbytes;
      curr_free_block->size -= new_used_block_tot_size;
      return new_used_block + 1;
    }
    curr_free_block = curr_free_block->next;
  }
  return (void *)0;
}

void myfree(void *firstbyte) {
  struct mem_control_block *firstblock = (struct mem_control_block *) firstbyte - 1;
  if (firstblock->size < 0 || firstblock->size > MEM_SIZE) {
    printf("Got something weird when trying to free, are you sure this is a malloced pointer? size: %d\n", firstblock->size);
    exit(1);
  }

  struct mem_control_block *iter = free_list_start;
  struct mem_control_block *prev = NULL;
  while(iter != NULL) {
    if (iter->next == NULL || iter->next > firstblock) {
      firstblock->next = iter->next;
      prev = iter;
      prev->next = firstblock;
      break;
    }
    iter = iter->next;
  }
  if (iter == NULL) {
    printf("Something went wrong, iter shouldnt be NULL here\n");
    return;
  }

  // merge left
  if (prev != NULL && (uint8_t *)prev + prev->size + sizeof(struct mem_control_block) == (uint8_t *)firstblock) {
    prev->next = firstblock->next;
    prev->size += firstblock->size + sizeof(struct mem_control_block); 
  }
  // merge right
  if (firstblock->next != NULL && (uint8_t *)firstblock + firstblock->size + sizeof(struct mem_control_block) == (uint8_t *)firstblock->next) {
    firstblock->size += firstblock->next->size + sizeof(struct mem_control_block);
    firstblock->next = firstblock->next->next;
  }
}

int main(int argc, char **argv) {
  void *p1 = mymalloc(16); 
  printf("Starting mem alloc. Start of memory is %p\n", managed_memory_start);
  void *p2 = mymalloc(23); 
  void *p3 = mymalloc(53); 
  printf("SIZE %d\n", ((struct mem_control_block*)(p3) - 1)->size);
  void *p4 = mymalloc(492); 
  print_mem();
  myfree(p3);
  print_mem();


  // 53 is rounded up to 56
  if (free_list_start->next == NULL || free_list_start->next->size != 56) {
    if (free_list_start->next == NULL) {
      printf("FAIL: expected free_list_start->next->size to be 56, but free_list_start->next was NULL\n");
    } else {
      printf("FAIL: expected free_list_start->next->size to be 56, but got %d\n", free_list_start->next->size);
    }
  }


  void *p = mymalloc(16);
  struct mem_control_block* p_mcb = (struct mem_control_block*)((uint8_t *)p - sizeof(struct mem_control_block));
  printf("addr %p, size: %d\n",p, p_mcb->size);
  if (p_mcb->size != 16) {
    printf("ERROR: Did not get 16 bytes");
  }

  /* add your test cases here! */
}
