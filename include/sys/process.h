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
} pcb_t;

pcb_t *get_new_pcb();
pcb_t *create_kernel_thread(void *func);
void __context_switch(pcb_t *me, pcb_t *next);
void schedule(int a);
void process_init();
#endif
