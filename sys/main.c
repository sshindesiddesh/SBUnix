#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

#include <sys/idt.h>
#include <sys/timer.h>
#include <sys/pic.h>
#include <sys/memory.h>

#define INITIAL_STACK_SIZE 4096
uint8_t initial_stack[INITIAL_STACK_SIZE]__attribute__((aligned(16)));
uint32_t* loader_stack;
extern char kernmem, physbase;
void clear();

//void ahci_init();

void memory_init(uint32_t *modulep, void *physbase, void *physfree);

void *memcpy(void *dest, const void *src, int n);

typedef struct PCB {
	uint64_t pid;
	uint64_t rsp;
	uint8_t kstack[512];
} pcb_t;

pcb_t *pcb0;
pcb_t *pcb1;
pcb_t *pcb2;

void con_switch(pcb_t *me, pcb_t *next, va_t addr);
void __context_switch(pcb_t *me, pcb_t *next);

void schedule(int a)
{
	if (a == 1)
		__context_switch(pcb1, pcb2);
	else
		__context_switch(pcb2, pcb1);
}

void func2()
{
	int a = 0;
	while (1) {
		kprintf("Hello %d\n", a++);
		schedule(2);
	}
}

void func1()
{
	int b = 0;
	while (1) {
		kprintf("World %d\n", b++);
		schedule(1);
	}
}

void process_man()
{

	pcb0  = (pcb_t *)kmalloc(sizeof(pcb_t));
	pcb0->pid = 0;
	pcb1  = (pcb_t *)kmalloc(sizeof(pcb_t));
	pcb1->pid = 1;
	pcb2  = (pcb_t *)kmalloc(sizeof(pcb_t));
	pcb2->pid = 2;
	kprintf("PCB : %p %p\n", pcb1, pcb2);

	*((uint64_t *)&pcb1->kstack[496]) = (uint64_t)func1;
	*((uint64_t *)&pcb1->kstack[496-14*8]) = (uint64_t)pcb1;
	pcb1->rsp = (uint64_t)&(pcb1->kstack[496-14*8]);
	*((uint64_t *)&pcb2->kstack[496]) = (uint64_t)func2;
	*((uint64_t *)&pcb2->kstack[496-14*8]) = (uint64_t)pcb2;
	pcb2->rsp = (uint64_t)&(pcb2->kstack[496-14*8]);

	__context_switch(pcb0, pcb1);

	kprintf("We will never written here\n");

	while (1);
}


void start(uint32_t *modulep, void *physbase, void *physfree)
{
	//init_idt();
	//timer_init();
	//pic_init();
	//clear();
	memory_init(modulep, physbase, physfree);
	process_man();
	//ahci_init();
}

void boot(void)
{
  // note: function changes rsp, local stack variables can't be practically used
  register char *temp1, *temp2;

  for(temp2 = (char*)0xb8001; temp2 < (char*)0xb8000+160*25; temp2 += 2) *temp2 = 7 /* white */;
  __asm__ volatile (
    "cli;"
    "movq %%rsp, %0;"
    "movq %1, %%rsp;"
    :"=g"(loader_stack)
    :"r"(&initial_stack[INITIAL_STACK_SIZE])
  );
  init_gdt();
  start(
    (uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
    (uint64_t*)&physbase,
    (uint64_t*)(uint64_t)loader_stack[4]
  );
  while (1);
  for(
    temp1 = "!!!!! start() returned !!!!!", temp2 = (char*)0xb8000;
    *temp1;
    temp1 += 1, temp2 += 2
  ) *temp2 = *temp1;
  while(1) __asm__ volatile ("hlt");
}
