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
	/* offset : 8 Do not move this. Used in assembly with offset */
	uint64_t rsp;
	/* offset : 0x10 Do not move this. Used in assembly with offset */
	pml_t pml4;
	/* offset : 0x18 Do not move this. Used in assembly with offset */
	uint64_t entry;
	/* offset : 0x20 Do not move this. Used in assembly with offset */
	/* User Space stack */
	uint64_t u_rsp;
	/* Kernel Space stack */
	uint8_t kstack[KSTACK_SIZE];
	uint8_t is_usr;
	struct mm_struct *mm;
	struct vma *heap_vma;
	struct PCB *next;
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

va_t kmalloc_user(pcb_t *pcb, const uint64_t size);
void allocate_vma(pcb_t *pcb, vma_t *vma);

#endif
