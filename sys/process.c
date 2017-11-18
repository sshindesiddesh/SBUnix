#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/memory.h>
#include <sys/process.h>
#include <sys/gdt.h>
#include <sys/syscall.h>
#include <sys/config.h>

/* TODO: Bug : Scheduler schedukes few tasks repetatively.  Cannot see for finite. During infinite, something goes wrong */

/* Head of the running linked list for yield */
pcb_t *head = NULL;

/* Global PID assigner */
static uint64_t PID = -1;

pcb_t *get_new_pcb()
{
	pcb_t *l_pcb = (pcb_t *)kmalloc(sizeof(pcb_t));
	l_pcb->pid = ++PID;
	return l_pcb;

}

uint64_t u_test;
/* Create a kernel thread.
 * Allocate a PCN block.
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 * Add it to the scheduler list at the end.
 */
pcb_t *create_user_thread(void *func)
{
	pcb_t *l_pcb = get_new_pcb(), *t_pcb = head;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]);
	if (head == NULL) {
		head = l_pcb;
	} else {
		while (t_pcb->next != head)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
		
	}

	l_pcb->next = head;

	uint64_t u_stack = (uint64_t)kmalloc_user(0x1000);
	l_pcb->u_stack = u_stack;
	l_pcb->u_rsp = (u_stack + 4096 - 8);
	kprintf("URSP : %p %d\n", l_pcb->u_rsp, *(uint64_t *)(l_pcb->u_rsp));

	u_test = (uint64_t)kmalloc_user(0x1000);
	return l_pcb;
}

/* Create a kernel thread.
 * Allocate a PCN block.
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 * Add it to the scheduler list at the end.
 */
pcb_t *create_kernel_thread(void *func)
{
	pcb_t *l_pcb = get_new_pcb(), *t_pcb = head;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]);
	if (head == NULL) {
		head = l_pcb;
	} else {
		while (t_pcb->next != head)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
		
	}
	l_pcb->next = head;
	return l_pcb;
}

/* Yield from process */
void yield(void)
{
	pcb_t *cur_pcb = head;

	if (head == head->next)
		return;

	head = head->next;

	/* This is hardcoded for now as rsp is not updated after returning from the syscall. TODO: Check this. */
	set_tss_rsp((void *)&head->kstack[KSTACK_SIZE - 8]);
	
#ifdef PROC_DEBUG
	kprintf("SCH: PID %d", head->pid);
#endif
	__context_switch(cur_pcb, head);
}

void __switch_ring3(uint64_t rsp, uint64_t func);

void func2();

pcb_t *usr_pcb_1;
pcb_t *usr_pcb_2;


/* Function to try all the syscalls */
void try_syscall()
{
	/* Default */
	__asm__ volatile ("mov $5, %rax");
	__asm__ volatile ("int $0x80");

	/* Write */
	char *buf = "Hello World\n";
	kprintf("buf %p\n", buf);
	__asm__ volatile (
		"movq $1, %%rax;"
		"movq %0, %%rbx;"
		: "=m"(buf)
		:
		: "rax", "rbx", "rcx", "rdx"
		);
	__asm__ volatile ("int $0x80");

	/* Yield */
	__asm__ volatile ("mov $2, %rax");
	__asm__ volatile ("int $0x80");
}


/* User Process */
void thread1()
{
	while (1) {
		__syscall_write("thread 1\n");
		/* This is yield. Implemented as a system call. */
#if	!PREEMPTIVE_SCHED
		__syscall_yield();
#endif

	}
}

void func1()
{
	set_tss_rsp((void *)&usr_pcb_1->kstack[KSTACK_SIZE - 8]);
	__switch_ring3(usr_pcb_1->u_rsp, (uint64_t)thread1);
}

/* User Process */
void thread2()
{
	while (1) {
		__syscall_write("thread 2\n");
		/* This is yield. Implemented as a system call. */
#if	!PREEMPTIVE_SCHED
		__syscall_yield();
#endif

	}
}

void func3()
{
	set_tss_rsp((void *)&usr_pcb_2->kstack[KSTACK_SIZE - 8]);
	__switch_ring3(usr_pcb_2->u_rsp, (uint64_t)thread2);
}

void func5()
{
	while (1) {
		__syscall_write("func 5\n");
#if	!PREEMPTIVE_SCHED
		__syscall_yield();
#endif
	}
}

/* Initialise kernel thread creation. */
void process_init()
{
	pcb_t *pcb0 = get_new_pcb();
	usr_pcb_1 = create_user_thread(func1);
	usr_pcb_2 = create_user_thread(func3);
	create_kernel_thread(func5);
#if 0
	pcb_t *elf_pcb = create_elf_process("bin/sbush", NULL);	
	*((uint64_t *)&elf_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)elf_pcb->entry;
        *((uint64_t *)&elf_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]) = (uint64_t)elf_pcb;
        elf_pcb->rsp = (uint64_t)&(elf_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]);

	if (elf_pcb && pcb0)
                kprintf("both PCB allocated");

#endif
	/* This happens only once and kernel should not return to this stack. */
	__context_switch(pcb0, usr_pcb_1);

	kprintf("We will never return here\n");

	while (1);
}
