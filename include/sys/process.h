#ifndef _PROCESS_H_
#define _PROCESS_H_
#include <sys/memory.h>

#define KSTACK_SIZE	4096
/* 14 is the number of push/pop in context switch */
#define CON_STACK_SIZE	(14*8)

typedef enum vma_type {
	TEXT = 0,
	DATA,
	STACK,
	HEAP,
	OTHER
} vma_type;

typedef struct PCB {
	uint64_t pid;
	uint64_t rsp;
	uint8_t kstack[KSTACK_SIZE];
	struct PCB *next;
	pml_t *pml4;
	uint64_t u_stack;
	uint64_t u_rsp;
	struct mm_struct *mm;
	uint64_t entry;
	struct vma *heap_vma;
} pcb_t;

typedef struct mm_struct {
        struct vma *head, *tail;
        uint32_t vma_cnt;
} mm_t;

typedef struct vma {
        struct mm_struct *vm_mm;
        uint64_t start;
        uint64_t end;
	vma_type type;
        struct vma *next;
        uint64_t flags;
        uint64_t file;
	uint64_t offset;
} vma_t;

pcb_t *get_new_pcb();
pcb_t *create_kernel_thread(void *func);
void __context_switch(pcb_t *me, pcb_t *next);
void schedule(int a);
void process_init();
/* Yield from process */
void yield(void);
#endif
