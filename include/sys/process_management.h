#ifndef _PROCESS_MANAGEMENT_H
#define _PROCESS_MANAGEMENT_H

#include <sys/defs.h>
#include <sys/memory.h>

struct mm_t {
	int count;
	uint64_t * pt;
	unsigned long context;
	unsigned long start_code, end_code, start_data, end_data;
	unsigned long start_brk, brk, start_stack, start_mmap;
	unsigned long arg_start, arg_end, env_start, env_end;
	unsigned long rss, total_vm, locked_vm;
	unsigned long def_flags;
	struct vma_t * mmap;
	struct vma_t * mmap_avl;
};

/* VMA struct definition */
struct vma_t {
	struct mm_t * vma_mm;
	uint64_t vma_start;
	uint64_t vma_end;
	uint64_t vma_size;
	unsigned long vma_flags;
	struct vma_t * vma_next;
	uint64_t grows_down;
	uint64_t vm_file;
	uint64_t vma_offset;
};
typedef struct vma_t VMA;

/* PCB structure */
#if 0
struct pcb_t {
	char p_name[100]; /* process name */
	uint64_t state;  /* 0 runnable, -1 non-runnable, > 0 stopped */
	uint64_t pid;
	uint64_t ppid;
	uint64_t zombie;
	uint64_t priority; /* when the process should run */
	uint64_t counter; /* how long the process is run */
	pml_t pml4e; /* will go to cr3 register */
	uint64_t *stack; /* own stack */
	uint64_t *rsp; /* own stack pointer */
	uint64_t rip; /* instruction pointer */
	uint64_t status; /* exit status */
	uint64_t kernel_stack[512];
	int sleep_time;
	int is_waiting;
	struct vma_t *h_vma;
	struct mm_t *mm;
};
typedef struct pcb_t PCB;
#endif

struct pcb_t {
	char kstack[4096];
	uint64_t pid;
	uint64_t rsp;
	uint64_t state;  /* 0 runnable, 1 sleeping, > 1 zombie */
	int exit_status;
	struct pcb_t *next;
};
typedef struct pcb_t PCB;


//uint64_t ready_queue[100];

PCB * malloc_pcb(void * func_ptr);

// This structure defines a 'task' - a process.
typedef struct task
{
   int id;                // Process ID.
   uint64_t esp, ebp;       // Stack and base pointers.
   uint64_t eip;            // Instruction pointer.
   //page_directory_t *page_directory; // Page directory.
   struct task *next;     // The next task in a linked list.
} task_t;

// Initialises the tasking system.
void initialise_tasking();

// Called by the timer hook, this changes the running process.
void task_switch();

// Forks the current process, spawning a new one with a different
// memory space.
int new_fork();

// Causes the current process' stack to be forcibly moved to a new location.
void move_stack(uint64_t new_stack_start, uint64_t size);

// Returns the pid of the current process.
int getpid();


#endif
