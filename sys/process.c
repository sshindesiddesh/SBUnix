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

/* TODO: Bug : Scheduler schedukes few tasks repetatively.  Cannot see for finite. During infinite, something goes wrong */

extern pml_t *pml;

/* Head of the running linked list for yield */
pcb_t *cur_pcb = NULL;

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
	pcb_t *l_pcb = get_new_pcb(), *t_pcb = cur_pcb;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]);

	if (cur_pcb == NULL) {
		cur_pcb = l_pcb;
	} else {
		while (t_pcb->next != cur_pcb)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
	}

	l_pcb->next = cur_pcb;

	l_pcb->is_usr = 1;

	set_proc_page_table(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(0x1000);

	/* set current directory and root node for user process */
	strcpy(l_pcb->current_dir, "/rootfs/bin/");
	char path[100] = "\0";
	strcpy(path, l_pcb->current_dir);
	dir_t *dir1 = tarfs_opendir(path);
	l_pcb->current_node = dir1->node;
	if(dir1->node)
		kprintf(" curr node assigned");
	
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

	/* Update kstack to return 0 in child process */
	c_pcb->rsp = (uint64_t)&(c_pcb->kstack[KSTACK_SIZE - (44*8)]);

	/* Add it after parent */
	c_pcb->next = cur_pcb->next->next;
	cur_pcb->next = c_pcb;

	/* Allocate mm struct */
	c_pcb->mm = (mm_struct_t *)kmalloc(0x1000);

	/* copy vma mappings */
	copy_vma_mapping(cur_pcb, c_pcb);

	/* return child pcb */
	return c_pcb;
}


int sys_fork()
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
	pcb_t *l_pcb = get_new_pcb(), *t_pcb = cur_pcb;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 8 - CON_STACK_SIZE]);

	l_pcb->is_usr = 0;

	if (cur_pcb == NULL) {
		cur_pcb = l_pcb;
	} else {
		while (t_pcb->next != cur_pcb)
			t_pcb = t_pcb->next;
		t_pcb->next = l_pcb;
	}

	l_pcb->next = cur_pcb;

	l_pcb->mm = (mm_struct_t *)kmalloc(0x1000);

	set_proc_page_table(l_pcb);

	return l_pcb;
}

/* Yield from process */
void yield(void)
{
	pcb_t *prv_pcb = cur_pcb;

	if (cur_pcb == cur_pcb->next)
		return;

	cur_pcb = cur_pcb->next;

	/* This is hardcoded for now as rsp is not updated after returning from the syscall. TODO: Check this. */
	set_tss_rsp((void *)&cur_pcb->kstack[KSTACK_SIZE - 8]);
	
#ifdef PROC_DEBUG
	kprintf("SCH: PID %d", cur_pcb->pid);
#endif
#if	ENABLE_USER_PAGING
	__asm__ volatile("mov %0, %%cr3":: "b"(cur_pcb->pml4));
#endif
	__context_switch(prv_pcb, cur_pcb);
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
	kprintf("Thread1");
	while (1);
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
	struct posix_header_ustar *start = (struct posix_header_ustar *)get_posix_header("/rootfs/bin/cat");
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

#if 0
	struct posix_header_ustar *header = (struct posix_header_ustar *)get_posix_header("/rootfs/bin/sbush");
	kprintf("header : %p", header);
	Elf64_Ehdr *elf_header = (Elf64_Ehdr *)header;
	if (is_proper_executable(elf_header) == 0)
		kprintf(" binary verified");
#endif

	dir_t *dir2;
        char *path2 = "/rootfs/";
        dir2 = tarfs_opendir(path2);
        if (dir2) {
                kprintf("opendir success, readdir: ");
                dirent_t *dentry;
                while((dentry = tarfs_readdir(dir2)) != NULL)
                        kprintf("\t%s", dentry->d_name);
        }
        else
                kprintf(" open failed");
        int new_fd = tarfs_open("dpp/abc.txt", O_RDONLY);
        if (new_fd)
                kprintf(" open done, fd : %d ", new_fd);
	char buf[4];
        
	int a = 4;
        while (a != 0 || (buf[a-1] != 0)) {
		a = tarfs_read(new_fd, (void *)buf, 4);
                kprintf("\n read content: %s, %d ", buf, a);
		kprintf(" l:%d ",buf[a-1]);
	}

	kprintf("We will never return here\n");
	while (1);
}
