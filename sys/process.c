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

extern pml_t *pml;

/* Head of the running linked list for yield */
pcb_t *head = NULL;

/* Global PID assigner */
static uint64_t PID = -1;

void set_proc_page_table(pcb_t *pcb);

pcb_t *get_new_pcb()
{
	pcb_t *l_pcb = (pcb_t *)kmalloc(sizeof(pcb_t));
	l_pcb->pid = ++PID;
	return l_pcb;
}

/* Create a kernel thread.
 * Allocate a PCN block.
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 * Add it to the scheduler list at the end.
 */
pcb_t *create_user_process(void *func)
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

	l_pcb->is_usr = 1;

	set_proc_page_table(l_pcb);

	uint64_t u_stack = (uint64_t)kmalloc_user(l_pcb, 0x1000);
	l_pcb->u_rsp = (u_stack + 4096 - 8);
	kprintf("URSP : %p\n", l_pcb->u_rsp);

	return l_pcb;
}

/* Create a kernel thread.
 * Allocate a PCN block.
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 * Add it to the scheduler list at the end.
 */
pcb_t *create_kernel_process(void *func)
{
	pcb_t *l_pcb = get_new_pcb(), *t_pcb = head;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]);

	l_pcb->is_usr = 0;

	if (head == NULL) {
		head = l_pcb;
	} else {
		while (t_pcb->next != head)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
	}
	l_pcb->next = head;

	set_proc_page_table(l_pcb);

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
#if	ENABLE_USER_PAGING
	__asm__ volatile("mov %0, %%cr3":: "b"(head->pml4));
#endif
	__context_switch(cur_pcb, head);
}

void __switch_ring3(pcb_t *pcb);

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

}


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
	usr_pcb_1->entry = (uint64_t)thread1;
	__switch_ring3(usr_pcb_1);
}

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

void func2()
{
	set_tss_rsp((void *)&usr_pcb_2->kstack[KSTACK_SIZE - 8]);
	usr_pcb_2->entry = (uint64_t)thread2;
	__switch_ring3(usr_pcb_1);
}

/* This is a kernel process */
void init_process()
{
	while (1) {
		__syscall_write("init\n");
#if	!PREEMPTIVE_SCHED
		__syscall_yield();
#endif
	}
}

int load_elf_code(pcb_t *pcb, void *start);

void elf_process()
{
	struct posix_header_ustar *start = (struct posix_header_ustar *)get_posix_header("/rootfs/bin/sbush");
	load_elf_code(usr_pcb_1, (void *)start);
	set_tss_rsp((void *)&usr_pcb_1->kstack[KSTACK_SIZE - 8]);
	__switch_ring3(usr_pcb_1);
}


/* Initialise kernel thread creation. */
void process_init()
{
	pcb_t *pcb0 = get_new_pcb();
	pcb_t *pcb1 = create_kernel_process(init_process);
	create_kernel_process(thread2);
/* If user paging is ebabled, user process cannot use code in kernel space.
 * Only option then is to load the elf executable.  */
#if ENABLE_USER_PAGING
	usr_pcb_1 = create_user_process(elf_process);
#else
	usr_pcb_1 = create_user_process(func1);
#endif
	/* This happens only once and kernel should not return to this stack. */
	__context_switch(pcb0, pcb1);

	kprintf("We will never return here\n");
	while (1);
}
