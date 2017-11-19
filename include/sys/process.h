#ifndef _PROCESS_H_
#define _PROCESS_H_
#include <sys/memory.h>

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
