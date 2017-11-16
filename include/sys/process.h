#ifndef _PROCESS_H_
#define _PROCESS_H_

#define KSTACK_SIZE	4096
/* 14 is the number of push/pop in context switch */
#define CON_STACK_SIZE	(14*8)

typedef struct PCB {
	uint64_t pid;
	uint64_t rsp;
	uint8_t kstack[KSTACK_SIZE];
	struct PCB *next;
	uint64_t u_stack;
	uint64_t u_rsp;
	struct MM_struct *mm;
	uint64_t entry;
	struct VMA *heap_vma;
} pcb_t;

typedef struct VMA {
        struct MM_struct *vm_mm;
        uint64_t vma_start;
        uint64_t vma_end;
        struct VMA *next;
        uint64_t vma_flags;
        uint64_t vma_file;
	uint64_t vma_offset;
	uint64_t vma_size;
} vma_t;

typedef struct MM_struct {
        struct VMA *vma_list;
        uint64_t pml4e;
        uint32_t vma_count;
        uint64_t hiwater_vm;
        uint64_t total_vm;
        uint64_t stack_vm;
        uint64_t start_brk, end_brk, start_stack;
        uint64_t arg_start, arg_end, env_start, env_end;
} mm_t;

pcb_t *get_new_pcb();
pcb_t *create_kernel_thread(void *func);
void __context_switch(pcb_t *me, pcb_t *next);
void schedule(int a);
void process_init();
#endif
