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

} pcb_t;

pcb_t *pcb1;
pcb_t *pcb2;

void con_switch(pcb_t *me, pcb_t *next, va_t addr);

void func ()
{
	int a = 0;
	kprintf("Hello");
	while (1) {
		a++;
		con_switch(pcb2, pcb1, 0);
	}
}

void con_switch(pcb_t *me, pcb_t *next, va_t addr)
{
#if 0
	uint64_t f_rsp = 0;
	uint64_t f_rbp = 0;
	__asm__ __volatile__(
		"movq %%rbp, %0;"
		:"=m"(f_rbp)
		:
		:"memory"
		);
	__asm__ __volatile__(
		"movq %%rsp, %0;"
		:"=m"(f_rsp)
		:
		:"memory"
		);
#if 0
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*0)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*1)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*2)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*3)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*4)));
	kprintf("rbp %p rsp %p\n", f_rbp, f_rsp);
	for (int i = 0; i < 64; i+=8) {
		//kprintf ("%x ", *((uint8_t *)(f_rsp + i)));
		kprintf ("%p ", *((uint64_t *)(f_rsp + i)));
		if (i%15 == 0 && i != 0)
			kprintf("\n");
	}
#endif
	kprintf(" ra %p\n", __builtin_return_address(0));
	while (1);
#endif
	/* Save me on my stack  */
	__asm__ volatile ("push %rdi;");
	/* Save my stack pointer */
	__asm__ __volatile__(
		"movq %%rsp, %0;"
		:"=m"(me->rsp)
		:
		:"memory"
		);

	if (addr) {
		memcpy((void *)(addr + 64), (void *)(me->rsp), 64 * 8);
		kprintf("new stack addr %p\n", addr + 64);
		next->rsp = (uint64_t)(addr + 64);
#define f_rsp	me->rsp
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*0)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*1)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*2)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*3)));
	kprintf ("%p ", *((uint64_t *)(f_rsp + 8*4)));
		kprintf("func %p\n", func);
		*((uint64_t *)(next->rsp + 8*4)) = (uint64_t)&func;
		//*((uint64_t *)(next->rsp + 8*3)) = (uint64_t)&func;
		//*((uint64_t *)(next->rsp + 8*2)) = (uint64_t)&func;
		//*((uint64_t *)(next->rsp + 8*1)) = (uint64_t)&func;
		*((uint64_t *)(next->rsp + 8*0)) = (uint64_t)pcb2;
		//*((uint64_t *)(next->rsp + 8*1)) = (uint64_t)0;
		//*((uint64_t *)(next->rsp + 8*2)) = (uint64_t)pcb2;
		//*((uint64_t *)(next->rsp + 8*3)) = (uint64_t)pcb2;
		kprintf("Done ..... %p", *((uint64_t *)(next->rsp + 8*0)));
	}


	/* Switch to new stack pointer */
	__asm__ __volatile__(
		"movq %0, %%rsp;"
		:
		:"m"(next->rsp)
		:"memory"
		);

	/* Update me to new task */
	__asm__ volatile ("pop %rdi;");

	__asm__ __volatile__(
		"movq %%rdi, %0;"
		:"=r"(me)
		:
		:
		);
}

void process_man();
void create_first_stack(pcb_t *me, uint64_t stack)
{
	//__asm__ volatile ("pop %rdi;");
}


void process_man()
{
	pcb1  = (pcb_t *)kmalloc(sizeof(pcb_t));
	pcb1->pid = 1;
	pcb2  = (pcb_t *)kmalloc(sizeof(pcb_t));
	pcb2->pid = 23;
	kprintf("PCB : %p %p\n", pcb1, pcb2);
	va_t addr = kmalloc(512);
	kprintf(" new stack addr %p\n", addr);
	con_switch(pcb1, pcb2, addr);
	kprintf("Back Here\n");
	con_switch(pcb1, pcb2, addr);
	kprintf(" Awesome Back Here\n");
	//while (1);
}


void start(uint32_t *modulep, void *physbase, void *physfree)
{
	init_idt();
	timer_init();
	pic_init();
	clear();
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
