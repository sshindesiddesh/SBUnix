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


#define MAX_NO_PROCESS	1000
/* TODO: Bug : Scheduler schedukes few tasks repetatively.  Cannot see for finite. During infinite, something goes wrong */

extern pml_t *pml;

/* current of the running queue */
pcb_t *cur_pcb = NULL;

/* Current running index */
int cur_index = 0;

/* tail of the running queue */
pcb_t *tail = NULL;

/* Head of zombies */
pcb_t *zombie_head = NULL;

/* Global PID assigner */
static uint64_t PID = -1;

void set_proc_page_table(pcb_t *pcb);
void free_page_entry(pml_t *pml);
void deallocate_pcb(pcb_t *pcb);

pcb_t proc_array[MAX_NO_PROCESS];

#if 0
void add_to_zombie(pcb_t *pcb)
{
#if 0
	kprintf("\nAdded Zombie %p\n", pcb);
#endif
	if (!zombie_head) {
		zombie_head = pcb;
		zombie_head->next = NULL;
	} else {
		pcb->next = zombie_head;
		zombie_head = pcb;
	}
}

void free_zombies()
{
	pcb_t *zhead = zombie_head;

	while (zhead) {
#if 0
		kprintf("ZOMBIE FREE %p\n", zhead);
#endif
		/* Free page table pages */
		/* Free all the physical pages allocated to the child and not shared */
		free_page_entry((pml_t *)zhead->pml4);
		/* Free PCB struct */
		deallocate_pcb(zhead);
		zhead = zhead->next;
	}

	/* After this clean up, zombie head should always be clen */
	zombie_head = zhead;
}
#endif

pcb_t *get_new_pcb()
{
	pcb_t *l_pcb = NULL;
	int i;
	for (i = 1; i < MAX_NO_PROCESS; i++) {
		if (proc_array[i].state == AVAIL) {
			l_pcb = &proc_array[i];
			memset(l_pcb, 0, sizeof(l_pcb));
			/* TODO: Check this */
			proc_array[i].state = READY;
			break;
		}
	}

	if (l_pcb == NULL) {
		kprintf("!!!PANIC : System out of processes memory!!!");
		while (1);
	}
	l_pcb->pid = ++PID;
	return l_pcb;
}

void init_proc_array()
{
	pcb_t *l_pcb = NULL;
	int i;
	for (i = 0; i < MAX_NO_PROCESS; i++) {
		l_pcb = &proc_array[i];
		l_pcb->state = AVAIL;
	}

}

pcb_t *get_next_ready_pcb()
{
	pcb_t *l_pcb = NULL;
	cur_index++;
	/* TODO: Check for infinite loop */
	for (; cur_index <= MAX_NO_PROCESS; cur_index++) {
		//kprintf("ready cur_index %x\n", cur_index);
		if (cur_index == MAX_NO_PROCESS) {
			cur_index = 1;
		}

		if (proc_array[cur_index].state == READY) {
			l_pcb = &proc_array[cur_index];
			break;
		}
	}

	return l_pcb;
}

pcb_t *create_user_process(void *func)
{
	pcb_t *l_pcb = get_new_pcb();
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

	l_pcb->is_usr = 1;

	set_proc_page_table(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(PG_SIZE);

	l_pcb->state = READY;
	
	return l_pcb;
}

pcb_t *create_clone_for_exec()
{
	pcb_t *l_pcb = get_new_pcb();
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)0;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

	l_pcb->is_usr = 1;

	set_proc_page_table(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(PG_SIZE);

	cur_pcb = l_pcb;

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
	c_pcb->state = READY;

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
 * Populate a dummy stack for it.
 * Fill appropriate rsp.
 */
pcb_t *create_kernel_process(void *func)
{
	pcb_t *l_pcb = get_new_pcb();

	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

	l_pcb->is_usr = 0;

	l_pcb->state = READY;

	l_pcb->mm = (mm_struct_t *)kmalloc(PG_SIZE);

	set_proc_page_table(l_pcb);

	return l_pcb;
}

void __exit_switch(pcb_t *cur_pcb);

/* Yield from process */
void kyield(void)
{
	pcb_t *prv_pcb = cur_pcb;
	cur_pcb = get_next_ready_pcb();

	/* This is hardcoded for now as rsp is not updated after returning from the syscall. TODO: Check this. */
	set_tss_rsp((void *)&cur_pcb->kstack[KSTACK_SIZE - 8]);
	
#ifdef PROC_DEBUG
	kprintf("SCH: PID %d", cur_pcb->pid);
#endif
#if	ENABLE_USER_PAGING
	__asm__ volatile("mov %0, %%cr3":: "b"(cur_pcb->pml4));
	__flush_tlb();
#endif
	__context_switch(prv_pcb, cur_pcb);
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
		kyield();

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
		yield();

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
		kprintf("Init\n");
		kyield();
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
	cur_pcb->exit_status = 1;
	cur_pcb->state = SLEEP;

	create_clone_for_exec();

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

	__switch_ring3(cur_pcb);

	return 0;
}

void kexit(int status)
{
	/* Make all children's parent as init process */
	/* Remove it's entry from parent */
	cur_pcb->exit_status = 1;
	kyield();
}

void thread3()
{
	while (1) {
		kprintf("Thread 3");
		kyield();

	}
}

void thread4()
{
	while (1) {
		kprintf("Thread 4");
		kyield();
	}
}

/* Initialise kernel thread creation. */
void process_init()
{
	init_proc_array();
	pcb_t *pcb0 = &proc_array[0];
	pcb_t *pcb1 = create_kernel_process(init_process);
	create_kernel_process(thread1);
	create_kernel_process(thread2);
	create_kernel_process(thread3);
	create_kernel_process(thread4);

	usr_pcb_1 = create_user_process(elf_process);

	/* Set to pcb of init process. Required as cur_pcb for first yield */
	cur_pcb = pcb1;
	/* This happens only once and kernel should not return to this stack. */
	__context_switch(pcb0, pcb1);

	kprintf("We will never return here\n");
	while (1);
}
