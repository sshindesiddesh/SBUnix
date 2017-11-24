#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/memory.h>
#include <sys/process.h>
#include <sys/gdt.h>
#include <sys/syscall.h>
#include <sys/config.h>
#include <sys/kutils.h>
#include <sys/idt.h>
#include <unistd.h>

/* TODO: Bug : Scheduler schedukes few tasks repetatively.  Cannot see for finite. During infinite, something goes wrong */

extern pml_t *pml;

/* current of the running queue */
pcb_t *cur_pcb = NULL;
/* tail of the running queue */
pcb_t *tail = NULL;

/* Global PID assigner */
static uint64_t PID = -1;

void set_proc_page_table(pcb_t *pcb);
void free_page_entry(pml_t *pml);
void deallocate_pcb(pcb_t *pcb);


pcb_t *get_new_pcb()
{
	pcb_t *l_pcb = (pcb_t *)kmalloc(sizeof(pcb_t));
	l_pcb->pid = ++PID;
	return l_pcb;
}

void add_pcb_to_runqueue(pcb_t *pcb)
{
	if (cur_pcb == NULL) {
		cur_pcb = tail = pcb;
		cur_pcb->next = NULL;
		tail->next = NULL;
	} else {
		tail->next = pcb;
		tail = tail->next;
		tail->next = NULL;
	}
}

/* Remove PCB from tail */
void remove_pcb_from_runqueue(pcb_t *pcb)
{
	/* Cannot remove PCB as there will be no once to run */
	if (tail == NULL)
		return;

	pcb_t *l_pcb = cur_pcb;
	while (l_pcb->next != tail) {
		l_pcb = l_pcb->next;
	}

	tail = l_pcb;
	tail->next = NULL;
}

/* Child is added just after parent in the runqueue */
void add_child_to_runqueue(pcb_t *pcb)
{
	pcb->next = cur_pcb->next;
	cur_pcb->next = pcb;
}

/* Add previous process to the end of runqueue */
pcb_t *get_next_pcb()
{
	if (cur_pcb->next) {
		tail->next = cur_pcb;
		cur_pcb = cur_pcb->next;
		tail = tail->next;
		tail->next = NULL;
	}

	return cur_pcb;
}

/* Create a kernel thread.
 * Allocate a PCN block.
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 * Add it to the scheduler list at the end.
 */
pcb_t *create_user_process(void *func)
{
	pcb_t *l_pcb = get_new_pcb();
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

#if 0
	if (cur_pcb == NULL) {
		cur_pcb = l_pcb;
	} else {
		while (t_pcb->next != cur_pcb)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
	}

	l_pcb->next = cur_pcb;
#endif

	add_pcb_to_runqueue(l_pcb);
	l_pcb->is_usr = 1;

	set_proc_page_table(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(0x1000);

#if 0
	/* set current directory and root node for user process */
	strcpy(l_pcb->current_dir, "/rootfs/bin/");
	char path[100] = "\0";
	strcpy(path, l_pcb->current_dir);
	dir_t *dir1 = tarfs_opendir(path);
	l_pcb->current_node = dir1->node;
	if(dir1->node)
		kprintf(" curr node assigned");
#endif
	
	return l_pcb;
}

void copy_vma_mapping(pcb_t *parent, pcb_t *child);

pcb_t *copy_user_process(pcb_t * p_pcb)
{
	pcb_t *c_pcb = get_new_pcb();
	uint64_t pid = c_pcb->pid;

	/* Copy parent pcb content to child pcb */
	/* After copying change mm_struct, pcb_next, pid and pml */
	memcpy((void *)c_pcb, (void *)p_pcb, sizeof(pcb_t));

	/* Change PID */
	c_pcb->pid = pid;

	/* Update return point from context switch to first instruction after fork syscall handler */
	*((uint64_t *)&c_pcb->kstack[KSTACK_SIZE - (28*8)]) = ((uint64_t)isr80 + 0x29);

	/* Set pcb on the top of kernle stack as it will be popped first from the context switch */
	*((uint64_t *)&c_pcb->kstack[KSTACK_SIZE - (29*8)]) = ((uint64_t)c_pcb);

	/* Set the kernel stack pointer */
	c_pcb->rsp = (uint64_t)&(c_pcb->kstack[KSTACK_SIZE - (29*8)]);

	/* Update kstack to return 0 in child process */
	*((uint64_t *)&c_pcb->kstack[KSTACK_SIZE - (21*8)]) = ((uint64_t)0);

	/* Add it after parent */
	add_child_to_runqueue(c_pcb);

	/* Allocate mm struct */
	c_pcb->mm = (mm_struct_t *)kmalloc(0x1000);

	/* copy vma mappings */
	copy_vma_mapping(cur_pcb, c_pcb);

	/* return child pcb */
	return c_pcb;
}


int kfork()
{
	pcb_t *c_pcb = copy_user_process(cur_pcb);
	/* return child pid for parent process */
	return c_pcb->pid;
}

/* Create a kernel thread.
 * Allocate a PCN block.
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 * Add it to the scheduler list at the end.
 */
pcb_t *create_kernel_process(void *func)
{
	pcb_t *l_pcb = get_new_pcb();
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

	l_pcb->is_usr = 0;

#if 0
	if (cur_pcb == NULL) {
		cur_pcb = l_pcb;
	} else {
		while (t_pcb->next != cur_pcb)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
	}

	l_pcb->next = cur_pcb;
#endif
	add_pcb_to_runqueue(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(0x1000);

	set_proc_page_table(l_pcb);

	return l_pcb;
}

void __exit_switch(pcb_t *cur_pcb);

/* Yield from process */
void kyield(void)
{
	pcb_t *prv_pcb = cur_pcb;

	cur_pcb = get_next_pcb();

	/* This is hardcoded for now as rsp is not updated after returning from the syscall. TODO: Check this. */
	set_tss_rsp((void *)&cur_pcb->kstack[KSTACK_SIZE - 8]);
	
#ifdef PROC_DEBUG
	kprintf("SCH: PID %d", cur_pcb->pid);
#endif
#if	ENABLE_USER_PAGING
	__asm__ volatile("mov %0, %%cr3":: "b"(cur_pcb->pml4));
	__flush_tlb();
#endif
	if (prv_pcb->exit_status) {
		/* Remove PCB from scheduler list */
		remove_pcb_from_runqueue(prv_pcb);

		/* Free page table pages */
		/* Free all the physical pages allocated to the child and not shared */
		free_page_entry((pml_t *)prv_pcb->pml4);
		/* Free PCB struct */
		deallocate_pcb(prv_pcb);
		/* Go to the next process */
		__exit_switch(cur_pcb);
	} else {
		__context_switch(prv_pcb, cur_pcb);
	}
}

void __switch_ring3(pcb_t *pcb);


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
		kprintf("thread 1");
		/* This is yield. Implemented as a system call. */
#if	!PREEMPTIVE_SCHED
		yield();
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
		kprintf("thread 2");
		/* This is yield. Implemented as a system call. */
#if	!PREEMPTIVE_SCHED
		yield();
#endif

	}
}

void func2();
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
		kprintf("Inint\n");
#if	!PREEMPTIVE_SCHED
		yield();
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

/* Try making it on stack */
static char kargs[10][100];

/* Execve */
uint64_t kexecve(char *file, char *argv[], char *env[])
{
	uint64_t argc = 0, len, *u_rsp, i = 0;
	/*Array of 10 pointers to be passed to the user */
	uint64_t *uargv[10];
	pcb_t *usr_pcb = create_user_process(NULL);
	pcb_t *t_pcb;
	t_pcb = cur_pcb;
	cur_pcb = usr_pcb;
	kprintf("file %s\n", file);

	/* Copy all arguments in kernel memory */
	strcpy(kargs[argc++], file);
	if (argv) {
		while (argv[argc - 1]) {
			strcpy(kargs[argc], argv[argc - 1]);
			argc++;
		}
	}

	struct posix_header_ustar *start = (struct posix_header_ustar *)get_posix_header(file);


	/* Switch CR3 so that, whatever page table entries are added through mmap->pagefaults are in the user process */
	__asm__ volatile("mov %0, %%cr3":: "b"(cur_pcb->pml4));

	load_elf_code(cur_pcb, (void *)start);

	/*  TODOG : Hardcode required ??? */
	u_rsp = (uint64_t *)(STACK_TOP - 8);

	/* Copy argument values on user stack */
	for (i = argc - 1; i > 0; i--) {
		len = strlen(kargs[i]) + 1;
		u_rsp -= len;
		memcpy((char *)u_rsp, kargs[i], len);
		uargv[i] = u_rsp;
	}

	/* Copy argument pointers on user stack */
	for (i = argc - 1; i > 0; i--) {
		u_rsp--;
		*(uint64_t *)u_rsp = (uint64_t)uargv[i];
	}

	/* Copy argc */
	/* TODO: Check argc - 1 ???? */
	u_rsp--;
	*((uint64_t *)u_rsp) = (uint64_t)(argc - 1);
	cur_pcb->u_rsp = (uint64_t)u_rsp;

	set_tss_rsp((void *)&cur_pcb->kstack[KSTACK_SIZE - 8]);

	cur_pcb = t_pcb;
	__switch_ring3(usr_pcb);

	return 0;
}

void kexit(int status)
{
	/* Make all children's parent as init process */
	/* Remove it's entry from parent */
	cur_pcb->exit_status = 1;
	yield();
}

void thread3()
{
	while (1) {
		kprintf("Thread 3");
		/* This is yield. Implemented as a system call. */
#if	!PREEMPTIVE_SCHED
		yield();
#endif

	}
}

void thread4()
{
	while (1) {
		kprintf("Thread 4");
		/* This is yield. Implemented as a system call. */
#if	!PREEMPTIVE_SCHED
		yield();
#endif
	}
}

/* Initialise kernel thread creation. */
void process_init()
{
	pcb_t *pcb0 = get_new_pcb();
	pcb_t *pcb1 = create_kernel_process(init_process);
	create_kernel_process(thread1);
	create_kernel_process(thread2);
	create_kernel_process(thread3);
	create_kernel_process(thread4);

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
