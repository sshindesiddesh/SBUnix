#include <sys/process_management.h>
#include <sys/kprintf.h>
#include <sys/defs.h>
#include <sys/memory.h>
//#include <sys/console.h>
#include <sys/tarfs.h>
//#include <string.h>

PCB *prev;
PCB *next;
//static uint64_t PID=1;
//static uint64_t p_count = 0;

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;

// Some externs are needed to access members in paging.c...
//extern page_directory_t *kernel_directory;
//extern page_directory_t *current_directory;
//extern void alloc_frame(page_t*,int,int);
extern uint64_t initial_esp;
extern uint64_t read_eip();

// The next available process ID.
uint64_t next_pid = 1;

// Copy len bytes from src to dest.
 void memcpy2(uint64_t *dest, const uint64_t *src, uint64_t len)
 {
     const uint64_t *sp = (const uint64_t *)src;
     uint64_t *dp = (uint64_t *)dest;
     for(; len != 0; len--) *dp++ = *sp++;
 }

#if 0
PCB *malloc_pcb(void * func_ptr)
{
	//page_dir_t * pp1 = NULL;
	PCB *new_pcb = (PCB *)kmalloc(sizeof(PCB));
	//ready_queue[++p_count] = (uint64_t)new_pcb;	
	//new_pcb->mm = (struct mm_t *)((char *)(new_pcb + 1); //???????
	
	new_pcb->pid = PID++;
	new_pcb->ppid = 0;
	new_pcb->zombie = 0;
	new_pcb->sleep_time = 0;
	new_pcb->is_waiting = 0;
	//memcpy(new_pcb->p_name, func_ptr, strlen(func_ptr));
	return new_pcb;	
}

#endif
void move_stack(uint64_t new_stack_start, uint64_t size)
{
	uint64_t i;
	// Allocate some space for the new stack.
#if 0
	for( i = (uint64_t)new_stack_start;
			i >= ((uint64_t)new_stack_start-size);
			i -= 0x1000)
	{
		// General-purpose stack is in user-mode.
		alloc_frame( get_page(i, 1, current_directory), 0 /* User mode */, 1 /* Is writable */ );
	}
#endif
	// Flush the TLB by reading and writing the page directory address again.
	uint64_t pd_addr;
	__asm__ volatile("mov %%cr3, %0" : "=r" (pd_addr));
	__asm__ volatile("mov %0, %%cr3" : : "r" (pd_addr));

	// Old ESP and EBP, read from registers.
	uint64_t old_stack_pointer; __asm__ volatile("mov %%rsp, %0" : "=r" (old_stack_pointer));
	uint64_t old_base_pointer;  __asm__ volatile("mov %%rbp, %0" : "=r" (old_base_pointer));
	uint64_t offset            = ((uint64_t)new_stack_start) - initial_esp;
	uint64_t new_stack_pointer = old_stack_pointer + offset;
	uint64_t new_base_pointer  = old_base_pointer  + offset;
	// Copy the stack.
	memcpy2((uint64_t *)new_stack_pointer, (const uint64_t *)old_stack_pointer, initial_esp - old_stack_pointer);
	// Backtrace through the original stack, copying new values into
	// the new stack.
	
	for(i = (uint64_t)new_stack_start; i > (uint64_t)new_stack_start-size; i -= 4)
	{
		uint64_t tmp = * (uint64_t*)i;
		// If the value of tmp is inside the range of the old stack, assume it is a base pointer
		// and remap it. This will unfortunately remap ANY value in this range, whether they are
		// base pointers or not.
		if (( old_stack_pointer < tmp) && (tmp < initial_esp))
		{
			tmp = tmp + offset;
			uint64_t *tmp2 = (uint64_t*)i;
			*tmp2 = tmp;
		}
	}
	// Change stacks.
	__asm__ __volatile__("mov %0, %%rsp;" : : "r" (new_stack_pointer));
	__asm__ __volatile__("mov %0, %%rbp;" : : "r" (new_base_pointer));
}

void initialise_tasking()
{
   __asm__ __volatile__("cli;");

   // Relocate the stack so we know where it is.
   //move_stack(0xE0000000, 0x2000);

   // Initialise the first task (kernel task)
   current_task = ready_queue = (task_t *)kmalloc(sizeof(task_t));
   current_task->id = next_pid++;
   current_task->esp = current_task->ebp = 0;
   current_task->eip = 0;
   //current_task->page_directory = current_directory;
   current_task->next = 0;

   // Reenable interrupts.
   //__asm__ __volatile__("sti;");
}

int new_fork()
{
	// We are modifying kernel structures, and so cannot be interrupted.
	__asm__ __volatile__("cli;");

	// Take a pointer to this process' task struct for later reference.
	task_t *parent_task = (task_t*)current_task;

	// Clone the address space.
	//page_directory_t *directory = clone_directory(current_directory);
	// Create a new process.
	task_t *new_task = (task_t*)kmalloc(sizeof(task_t));
	new_task->id = next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	//new_task->page_directory = directory;
	new_task->next = 0;

	// Add it to the end of the ready queue.
	// Find the end of the ready queue...
	task_t *tmp_task = (task_t*)ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;
	// ...And extend it.
	tmp_task->next = new_task;
	// This will be the entry point for the new process.
	uint64_t eip = read_eip();

	// We could be the parent or the child here - check.
	if (current_task == parent_task)
	{
		// We are the parent, so set up the esp/ebp/eip for our child.
		uint64_t esp; __asm__ __volatile__("mov %%rsp, %0;" : "=r"(esp));
		uint64_t ebp; __asm__ __volatile__("mov %%rbp, %0;" : "=r"(ebp));
		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;
		// All finished: Reenable interrupts.
		//__asm__ __volatile__("sti;");
		return new_task->id;
	}
	else
	{
		// We are the child - by convention return 0.
		return 0;
	}
}

void switch_task()
{
	kprintf(" inside switch_task ");
	// If we haven't initialised tasking yet, just return.
	if (!current_task)
		return;
	// Read esp, ebp now for saving later on.
	uint64_t esp, ebp, eip;
	__asm__ __volatile__("mov %%rsp, %0;" : "=r"(esp));
	__asm__ __volatile__("mov %%rbp, %0;" : "=r"(ebp));

	// Read the instruction pointer. We do some cunning logic here:
	// One of two things could have happened when this function exits -
	// (a) We called the function and it returned the EIP as requested.
	// (b) We have just switched tasks, and because the saved EIP is essentially
	// the instruction after read_eip(), it will seem as if read_eip has just
	// returned.
	// In the second case we need to return immediately. To detect it we put a dummy
	// value in EAX further down at the end of this function. As C returns values in EAX,
	// it will look like the return value is this dummy value! (0x12345).
	eip = read_eip();

	// Have we just switched tasks?
	if (eip == 0x12345)
		return;

	// No, we didn't switch tasks. Let's save some register values and switch.
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;
	// Get the next task to run.
	current_task = current_task->next;
	// If we fell off the end of the linked list start again at the beginning.
	if (!current_task) 
		current_task = ready_queue;
	esp = current_task->esp;
	ebp = current_task->ebp;

	kprintf(" before final register update");
	// Here we:
	// * Stop interrupts so we don't get interrupted.
	// * Temporarily put the new EIP location in ECX.
	// * Load the stack and base pointers from the new task struct.
	// * Change page directory to the physical address (physicalAddr) of the new directory.
	// * Put a dummy value (0x12345) in EAX so that above we can recognise that we've just
	// switched task.
	// * Restart interrupts. The STI instruction has a delay - it doesn't take effect until after
	// the next instruction.
	// * Jump to the location in ECX (remember we put the new EIP in there).
#if 0
__asm__ volatile(
     "cli\n" 
     "mov %0, %%rcx\n" 
     "mov %1, %%rsp\n" 
     "mov %2, %%rbp\n" 
     "mov $0x12345, %%rax\n" 
     "jmp *%%rcx\n"
                : : "r"(eip), "r"(esp), "r"(ebp));
#endif
}
