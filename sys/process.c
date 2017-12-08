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
#include <sys/ksyscall.h>
#include <sys/elf64.h>

void clear();
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

/* For debugging purpose */
void print_siblings(pcb_t *pcb)
{
	pcb_t *sib = pcb->child_head;
	kprintf("SIBS of PID %d\n", pcb->pid);
	while (sib) {
		kprintf("sib PID %x %p\n", sib->pid, sib);
		sib = sib->sibling;
	}
}

void add_child_to_siblings(pcb_t *p_pcb, pcb_t *c_pcb)
{
	if (!p_pcb || !c_pcb) {
		kprintf("Parent Child corrupted\n");
		while (1);
	}

	pcb_t *child = p_pcb->child_head;
	if (!child) {
		p_pcb->child_head = c_pcb;
		c_pcb->sibling = NULL;
	} else {
		c_pcb->sibling = p_pcb->child_head;
		p_pcb->child_head = c_pcb;
	}
}

void rem_child_from_sibling(pcb_t *p_pcb, pcb_t *c_pcb)
{
	if (!p_pcb || !c_pcb) {
		kprintf("Parent Child corrupted\n");
		while (1);
	}
	pcb_t *child = p_pcb->child_head;

	/* No children present for the parent */
	if (!child) {
		return;
	}

	pcb_t *child_p = NULL;
	while (child != c_pcb) {
		child_p = child;
		child = child->sibling;
	}
	if (child_p) {
		child_p->sibling = child->sibling;
	} else {
		p_pcb->child_head = child->sibling;
	}
}

uint64_t avb_index = 0;
/* Always gives a zero filled page for PCB */
pcb_t *get_new_pcb()
{
	pcb_t *l_pcb = NULL;
	int i, count_index = 0;
	for (i = avb_index;; i++, count_index++) {
		if (proc_array[i].state == AVAIL) {
			l_pcb = &proc_array[i];
			memset(l_pcb, 0, sizeof(l_pcb));
			/* TODO: Check this */
			proc_array[i].state = READY;
			avb_index = i + 1;
			break;
		}

		if (i == MAX_NO_PROCESS) {
			avb_index = 2;
		}

		if (count_index >= MAX_NO_PROCESS) {
			l_pcb = NULL;
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

pcb_t *get_pcb_from_pid(uint64_t pid)
{
	int i;
	/* Cannot Kill Init process */
	for (i = 2; i < MAX_NO_PROCESS; i++) {
		if (proc_array[i].pid == pid) {
			return &proc_array[i];
		}
	}
	return NULL;
}

pcb_t *get_next_ready_pcb()
{
	pcb_t *l_pcb = NULL;
	int count = 0;
	cur_index++;
	/* TODO: Check for infinite loop */
	for (; ; cur_index++, count++) {
		if (cur_index >= MAX_NO_PROCESS) {
			cur_index = 1;
		}

		if (proc_array[cur_index].state == READY) {
			l_pcb = &proc_array[cur_index];
			break;
		}
		if ((count > (MAX_NO_PROCESS + 10))) {
			proc_array[1].state = READY;
			cur_index = 1;
			return &proc_array[1];
		}
	}
#if 0
	kprintf("Next Ready PID %x\n", l_pcb->pid);
#endif
	return l_pcb;
}

pcb_t *create_user_process(void *func)
{
	pcb_t *l_pcb = get_new_pcb();
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)func;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

	l_pcb->parent = NULL;
	l_pcb->sibling = NULL;

	l_pcb->is_usr = 1;

	set_proc_page_table(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(PG_SIZE);

	l_pcb->state = READY;

	strcpy(l_pcb->current_dir, "/rootfs/bin/");
	dir_t *dir = tarfs_opendir("/rootfs/bin/");
	if(dir != NULL && dir->node != NULL)
		l_pcb->current_node = dir->node;
	return l_pcb;
}

pcb_t *create_clone_for_exec()
{
	pcb_t *l_pcb = get_new_pcb();
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 8]) = (uint64_t)0;
	*((uint64_t *)&l_pcb->kstack[KSTACK_SIZE - 16]) = (uint64_t)l_pcb;
	l_pcb->rsp = (uint64_t)&(l_pcb->kstack[KSTACK_SIZE - 16]);

	l_pcb->is_usr = 1;

	/* PID of new process should be the PID of the existing process */
	l_pcb->pid = cur_pcb->pid;

	set_proc_page_table(l_pcb);

	l_pcb->mm = (mm_struct_t *)kmalloc(PG_SIZE);

	/* Make parent of current process as the parent of new process */
	l_pcb->parent = cur_pcb->parent;

	/* Make siblings of current process as siblings of new process */
	l_pcb->child_head = cur_pcb->child_head;
	l_pcb->sibling = cur_pcb->sibling;

	strcpy(l_pcb->current_dir, cur_pcb->current_dir);
	l_pcb->current_node = cur_pcb->current_node;

	/* Change the sibling link in parent PCB which points to cur_pcb */
	pcb_t *sib = cur_pcb->parent->child_head;
	pcb_t *sib_p = NULL;
	if (sib) {
		while (sib != cur_pcb) {
			sib_p = sib;
			sib = sib->sibling;
		}
		if (sib_p) {
			sib_p->sibling = l_pcb;
		} else {
			cur_pcb->parent->child_head = l_pcb;
		}
	}

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

	/* Assign parent as the current pcb */
	c_pcb->parent = cur_pcb;

	/* Make sibling NULL, Will be reinitialized later */
	c_pcb->sibling = 0;

	/* Make child head NULL */
	c_pcb->child_head = 0;

	/* return child pcb */
	return c_pcb;
}


/* Add all children of parent process as siblings to the init_process and mark them zombie */
void add_all_children_to_init_proc(pcb_t *init_pcb, pcb_t *p_pcb)
{
	pcb_t *sib = p_pcb->child_head;
	if (!sib)
		return;
	/* Attach end of process siblings next to inits childhead */
	pcb_t *head = p_pcb->child_head;
	while (sib->sibling) {
		/* TODO: Should child execute even after parent executes ?  */
		/* sib->state = ZOMBIE; */
		/* add_child_to_siblings(init_pcb, sib); */
		sib->parent = init_pcb;
		sib = sib->sibling;
	}
	sib->parent = init_pcb;
	sib->sibling = init_pcb->child_head;
	if (sib->sibling)
		sib->sibling->parent = init_pcb;
	init_pcb->child_head = head;
}

int kfork()
{
	pcb_t *c_pcb = copy_user_process(cur_pcb);
	/* Add child to parent's siblings */
	add_child_to_siblings(cur_pcb, c_pcb);
	/* return child pid for parent process */
#if 0
	kprintf("FORK:CPID %d PPID %d\n", c_pcb->pid, c_pcb->parent->pid);
#endif
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

	l_pcb->parent = NULL;
	l_pcb->sibling = NULL;

	l_pcb->is_usr = 0;

	l_pcb->state = READY;

	l_pcb->mm = (mm_struct_t *)kmalloc(PG_SIZE);

	set_proc_page_table(l_pcb);

	return l_pcb;
}


/* Yield from process */
void kyield(void)
{
	pcb_t *prv_pcb = cur_pcb;
	cur_pcb = get_next_ready_pcb();
	if (!cur_pcb) {
		kprintf("Yield: Next PCB 0\n");
		while (1);
	}

#if 0
	kprintf("PRV PID %d CUR PID %d\n", prv_pcb->pid, cur_pcb->pid);
#endif

	/* This is hardcoded for now as rsp is not updated after returning from the syscall. TODO: Check this. */
	set_tss_rsp((void *)&cur_pcb->kstack[KSTACK_SIZE - 8]);
	
#ifdef PROC_DEBUG
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
		kyield();
	}
}

void func2()
{
	set_tss_rsp((void *)&usr_pcb_2->kstack[KSTACK_SIZE - 8]);
	usr_pcb_2->entry = (uint64_t)thread2;
	__switch_ring3(usr_pcb_1);
}

void kill_zombie()
{
	return;
	pcb_t *l_pcb = cur_pcb->sibling;
	while (l_pcb) {
		if (l_pcb->state == ZOMBIE) {
#if 0
			kprintf("***Killed Zombie PID %d***\n", l_pcb->pid);
#endif
			l_pcb->state = AVAIL;
			rem_child_from_sibling(cur_pcb, l_pcb);
			/* Free page table pages */
			/* Free all the physical pages allocated to the child and not shared */
			free_page_entry((pml_t *)l_pcb->pml4);
			/* Free PCB struct */
			deallocate_pcb(l_pcb);
		}
		l_pcb = l_pcb->sibling;
	}
}

/* This is a kernel process */
void idle_process()
{
	while (1) {
#if 1
		//kprintf("IDLE\n");
#endif
		kyield();
		__asm__ __volatile__ ("sti");
		__asm__ __volatile__ ("hlt");
		__asm__ __volatile__ ("cli");
	}
}

int load_elf_code(pcb_t *pcb, void *start);

void elf_process()
{
	//kprintf("ELF\n");
	/* TODO: Hardcode ??? */
	/* SBUSH will have init as its parent */
	/* Init process will have it as first child */
	usr_pcb_1->child_head = NULL;
	usr_pcb_1->sibling = NULL;

	/* Set process Name */
	strcpy(usr_pcb_1->proc_name, "/rootfs/bin/init");

	/* struct posix_header_ustar *start = (struct posix_header_ustar *)get_posix_header("/rootfs/bin/sbush"); */

	struct posix_header_ustar *start = (struct posix_header_ustar *)get_posix_header("/rootfs/bin/init");
	load_elf_code(usr_pcb_1, (void *)start);
	set_tss_rsp((void *)&usr_pcb_1->kstack[KSTACK_SIZE - 8]);
	__switch_ring3(usr_pcb_1);
}

char filename[100];
char local_name[100];
/* Try making it on stack */
char kargs[20][100];
/* Try making it on stack */
char kenv[20][100];
/*Array of 10 pointers to be passed to the user */
uint64_t *uargv[10];
uint64_t *uenv[10];

char *pt;

char *get_absolute_path(char *file, char *env[])
{
	struct posix_header_ustar *start = NULL;
	/* Input file is NULL */
	if (!file) {
		return NULL;
	/* Check if user entered absolute path */
	} else {
		start = (struct posix_header_ustar *)get_posix_header(file);
		if (start) {
			return file;
		}
	}
	if (!env) {
		return NULL;
	}

	/* Check with rest of the paths */
	int i = 0, j = 0;

	/* Mark the first entry NULL */
	strcpy(kargs[0], "");

	while (env[i]) {
		strcpy(local_name, env[i]);
		pt = strtok(local_name, "=");
		while (pt) {
			pt = strtok(NULL, ":");
			if (pt) {
				strcpy(filename, pt);
				strcat(filename, "/");
				strcat(filename, file);
#if 0
				kprintf("i = %d %s\n", i, filename);
#endif
				strcpy(kargs[j++], filename);
			}
		}
		i++;
	}

	/* Mark the next entry NULL */
	strcpy(kargs[j], "");

	while (j >= 0) {
		start = (struct posix_header_ustar *)get_posix_header(kargs[j]);
		if (start) {
			if (is_proper_executable((Elf64_Ehdr *)start) == 0) {
#if 0
				kprintf("j = %d %s\n", j, kargs[j]);
#endif
				strcpy(filename, kargs[j]);
				return filename;
			}
		}
		j--;
	}

	return NULL;
}

/* Execve */
uint64_t kexecve(char *in_file, char *argv[], char *env[])
{
	char *file = get_absolute_path(in_file, env);
	if (!file) {
		return -1;
	}

	uint64_t len, *u_rsp;
	int i = 0, ecnt = 0, argc = 0;

	pcb_t * prv_pcb = cur_pcb;
	cur_pcb->state = ZOMBIE;

	cur_pcb = create_clone_for_exec();
	/* Set process Name */
	strcpy(cur_pcb->proc_name, file);

	/* Copy all arguments in kernel memory */
	strcpy(kargs[argc++], file);
	if (argv) {
		while (argv[argc - 1]) {
			strcpy(kargs[argc], argv[argc - 1]);
			argc++;
		}
	}

	/* Copy all env in kernel memory */
	if (env) {
		while (env[ecnt]) {
			strcpy(kenv[ecnt], env[ecnt]);
			ecnt++;
		}
	}

	struct posix_header_ustar *start = (struct posix_header_ustar *)get_posix_header(file);


	/* Switch CR3 so that, whatever page table entries are added through mmap->pagefaults are in the user process */
	__asm__ volatile("mov %0, %%cr3":: "b"(cur_pcb->pml4));

	load_elf_code(cur_pcb, (void *)start);

	/*  TODOG : Hardcode required ??? */
	u_rsp = (uint64_t *)(STACK_TOP - 8);

	/* Copy env values on user stack */
	for (i = ecnt - 1; i >= 0; i--) {
		len = strlen(kenv[i]) + 1;
		u_rsp -= len;
		memcpy((char *)u_rsp, kenv[i], len);
		uenv[i] = u_rsp;
	}

	/* Copy argument values on user stack */
	for (i = argc - 1; i > 0; i--) {
		len = strlen(kargs[i]) + 1;
		u_rsp -= len;
		memcpy((char *)u_rsp, kargs[i], len);
		uargv[i] = u_rsp;
	}


	/* Copy NULL */
	u_rsp--;
	*(uint64_t *)u_rsp = 0;

	/* Copy env pointers on user stack */
	for (i = ecnt - 1; i >= 0; i--) {
		u_rsp--;
		*(uint64_t *)u_rsp = (uint64_t)uenv[i];
	}

	/* Copy NULL */
	u_rsp--;
	*(uint64_t *)u_rsp = 0;

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

	/* Free page table pages */
	/* Free all the physical pages allocated to the child and not shared */
	free_page_entry((pml_t *)prv_pcb->pml4);
	/* Free PCB struct */
	deallocate_pcb(prv_pcb);
	prv_pcb->state = AVAIL;
	prv_pcb->pid = 0;


	__switch_ring3(cur_pcb);

	return 0;
}

void kexit(int status)
{
	if (cur_pcb->pid < 2)
		return;
#if 0
	kprintf("PID %d called EXIT\n", cur_pcb->pid);
	kprintf("Parent %p pid %d\n", cur_pcb->parent, cur_pcb->parent->pid);
#endif
	/* Mark its exit status and change it's state to ZOMBIE */
	cur_pcb->exit_status = 1;
	cur_pcb->state = ZOMBIE;

	/* If parent is in wait state, make it ready */
	if (cur_pcb->parent->state == WAIT) {
		/* If it calls wait or waitpid on child pid */
		/* 0 and 1 both are included as hack. TODO : Cleanup */
		if ((cur_pcb->parent->wait_pid == 0) || (cur_pcb->parent->wait_pid == cur_pcb->pid)) {
#if 0
			kprintf("PID %d is ready \n", cur_pcb->parent->pid);
#endif
			cur_pcb->parent->state = READY;
			cur_pcb->parent->wait_pid = cur_pcb->pid;
		}
	}

	/* Remove it's entry from parent */
	/* Remove it from its parent process siblings list */
	/* rem_child_from_sibling(cur_pcb->parent, cur_pcb); */
	/* Change the sibling link in parent PCB which points to cur_pcb */
	rem_child_from_sibling(cur_pcb->parent, cur_pcb);

	/* Make all children's parent as init process */
	/* If it has any children, add them to the init process siblings list as zombie */
	add_all_children_to_init_proc(&proc_array[1], cur_pcb);

	/* Free page table pages */
	/* Free all the physical pages allocated to the child and not shared */
	free_page_entry((pml_t *)cur_pcb->pml4);
	/* Free PCB struct */
	deallocate_pcb(cur_pcb);
	cur_pcb->state = AVAIL;
	cur_pcb->pid = 0;
	kyield();
}

void kshutdown()
{
	int cur_pid = cur_pcb->pid;
	int i = 0;
	for (i = 2; i < MAX_NO_PROCESS; i++) {
		if (proc_array[i].pid != cur_pid && proc_array[i].state != AVAIL) {
			kkill(proc_array[i].pid);
		}
	}
	//clear();
	kprintf("\n==============================================================");
	kprintf("\n================= OS Shutting down ===========================");
	kprintf("\n==============================================================");
	while (1);
}

void kkill(uint64_t pid)
{
#if 0
	kprintf("Kill for PID %d from PID %d\n", pid, cur_pcb->pid);
#endif
	pcb_t *l_pcb = get_pcb_from_pid(pid);
	if ((l_pcb == 0) || (cur_pcb == 0)) {
		return;
	}

	if (pid < 2) {
		return;
	}

	if (l_pcb == cur_pcb) {
		kexit(0);
	}
	/* Mark its exit status and change it's state to ZOMBIE */
	l_pcb->exit_status = 1;
	l_pcb->state = ZOMBIE;

	/* If parent is in wait state, make it ready */
	if (l_pcb->parent && (l_pcb->parent->state == WAIT)) {
		/* If it calls wait or waitpid on child pid */
		/* 0 and 1 both are included as hack. TODO : Cleanup */
		if ((l_pcb->parent->wait_pid == -1) || (l_pcb->parent->wait_pid == l_pcb->pid)
				|| (l_pcb->parent->wait_pid == 0)) {
			l_pcb->parent->state = READY;
		}
	}

	/* Remove it's entry from parent */
	/* Remove it from its parent process siblings list */
	/* rem_child_from_sibling(cur_pcb->parent, cur_pcb); */
	/* Change the sibling link in parent PCB which points to cur_pcb */
	rem_child_from_sibling(l_pcb->parent, l_pcb);

	/* Make all children's parent as init process */
	/* If it has any children, add them to the init process siblings list as zombie */
	add_all_children_to_init_proc(&proc_array[1], l_pcb);

	/* Free page table pages */
	/* Free all the physical pages allocated to the child and not shared */
	free_page_entry((pml_t *)l_pcb->pml4);
	/* Free PCB struct */
	deallocate_pcb(l_pcb);
	l_pcb->state = AVAIL;
	l_pcb->pid = 0;
}

pid_t kgetpid(void)
{
	return cur_pcb->pid;
}

pid_t kgetppid(void)
{
	if (cur_pcb->parent)
		return cur_pcb->parent->pid;
	else return -1;
}

/* Prints all active process */
void kps()
{
	int i;
	kprintf("\n-----------------------------------------------------------------");
	kprintf("\n   PID   |   PPID   |   Process Name   ");
	kprintf("\n-----------------------------------------------------------------");
	for (i = 1; i < MAX_NO_PROCESS; i++) {
		if ((proc_array[i].state == READY) ||
			(proc_array[i].state == WAIT) ||
			(proc_array[i].state == SLEEP)) {
			if (proc_array[i].parent != 0) {
				kprintf("\n    %d    |    %d     |    %s   ", proc_array[i].pid, proc_array[i].parent->pid, proc_array[i].proc_name);
			} else {
				kprintf("\n    %d    |    --    |    %s   ", proc_array[i].pid, proc_array[i].proc_name);
			}
		}
	}

	kprintf("\n-----------------------------------------------------------------");
	kprintf("\n");
}

void decrement_sleep_count()
{
	int i;
	uint32_t sleep;

	for (i = 1; i < MAX_NO_PROCESS; i++) {
		if (proc_array[i].state == SLEEP) {
			sleep = proc_array[i].sleep_seconds;
			if (sleep > 2) {
				proc_array[i].sleep_seconds--;
#if 0
				kprintf("SLEEP secs %d\n", proc_array[i].sleep_seconds);
#endif
			} else {
#if 0
				kprintf("SLEEP Done\n");
#endif
				proc_array[i].sleep_seconds = 0;
				proc_array[i].state = READY;
			}
		}
	}

}

int sleep_cnt = 1;
void ksleep(uint64_t seconds)
{
#if 0
	kprintf("PCB %d sleep %d seconds \n", cur_pcb->pid, seconds);
#endif
	sleep_cnt++;
	cur_pcb->state = SLEEP;
	cur_pcb->sleep_seconds = seconds;
	kyield();
#if 0
	kprintf("Sleep Done\n");
#endif
	sleep_cnt--;
}

pid_t kwait(pid_t pid)
{
	int flag = 0;
	/* If the parent has no child, return */
	if (!cur_pcb->child_head) {
		return -1;
	}

	cur_pcb->wait_pid = pid;

	/* Mark state as WAIT */
	cur_pcb->state = WAIT;

	/*  Check if there is atleast one ready child */
	pcb_t *sib = cur_pcb->child_head;
	if (pid == 0) {
		while (sib) {
			if (sib->state == READY) {
				flag = 1;
				break;
			}
			sib = sib->sibling;
		}
	} else {
		/* Check if child exists */
		while (sib) {
			if (sib->pid == pid) {
				flag =  1;
				break;
			}
			sib = sib->sibling;
		}
	}

	if (flag == 0) {
		/* Remark state as READY if there was no ready child */
		cur_pcb->state = READY;
		return 0;
	}

	/* Yield to a different process */
	kyield();
	/* It will come here only when this process is marked ready
	 * by either of its children */
	/* Clean up all zombie children */
	/* This is ideally unusefull here as all processess are cleaned up on exit only */
	/* kill_zombie(); */
	return cur_pcb->wait_pid;
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

pcb_t pcb0;
/* Initialise kernel thread creation. */
void process_init()
{
	init_proc_array();
	pcb_t *pcb1 = create_kernel_process(idle_process);

	/* Set process Name */
	strcpy(pcb1->proc_name, "idle");

	usr_pcb_1 = create_user_process(elf_process);

	/* Set to pcb of init process. Required as cur_pcb for first yield */
	cur_pcb = pcb1;
	/* This happens only once and kernel should not return to this stack. */
	__context_switch(&pcb0, pcb1);

	kprintf("We will never return here\n");
	while (1);
}
