#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>

#include <sys/memory.h>
#include <sys/process.h>

/* TODO: Bug : Scheduler schedukes few tasks repetatively. */

/* Head of the running linked list for yield */
pcb_t *head = NULL;

static uint64_t PID = -1;

pcb_t *get_new_pcb()
{
	pcb_t *l_pcb = (pcb_t *)kmalloc(sizeof(pcb_t));
	l_pcb->pid = ++PID;
	return l_pcb;

}

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

void yield()
{
	pcb_t *cur_pcb = head;
	head = head->next;
	kprintf("SCH: PID %d\n", head->pid);
	__context_switch(cur_pcb, head);
}

#if 0
void func2()
{
	int a = 0;
	while (1) {
		kprintf("Hello %d\n", a++);
		yield();
	}
}

void func1()
{
	int b = 0;
	while (1) {
		kprintf("World %d\n", b++);
		yield();
	}
}
#endif

void func1()
{
	while (1) {
		kprintf("func 1\n");
		yield();
	}
}

void func2()
{
	while (1) {
		kprintf("func 2\n");
		yield();
	}
}

void func3()
{
	while (1) {
		kprintf("func 3\n");
		yield();
	}
}

void func4()
{
	while (1) {
		kprintf("func 4\n");
		yield();
	}
}

void func5()
{
	while (1) {
		kprintf("func 5\n");
		yield();
	}
}

void process_init()
{
	pcb_t *pcb0 = get_new_pcb();
	pcb_t *pcb_l = create_kernel_thread(func1);
	create_kernel_thread(func2);
	create_kernel_thread(func3);
	create_kernel_thread(func4);
	create_kernel_thread(func5);
	__context_switch(pcb0, pcb_l);

	kprintf("We will never written here\n");

	while (1);
}
