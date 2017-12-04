#include <sys/tarfs.h>
#include <sys/defs.h>
#include <sys/process.h>
#include <sys/memory.h>
#include <sys/elf64.h>
#include <sys/kprintf.h>
#include <sys/kutils.h>

#define ELF_LOAD_MMAP

#define PT_LOAD	1

void print_buf(uint8_t *buf, uint64_t size)
{
	int i;
	for (i = 0; i < size; i++) {
		if (i != 0 && i%16 == 0)
			kprintf("\n");
		kprintf("%x ", buf[i]);
	}
	kprintf("\n");

}

int load_elf_code(pcb_t *pcb, void *start)
{
	Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *)start;
	Elf64_Phdr *p_hdr = (Elf64_Phdr *)((Elf64_Addr)elf_hdr + elf_hdr->e_phoff);
	pcb->entry = elf_hdr->e_entry;

	if (is_proper_executable(elf_hdr) != 0) {
		return -1;
	}

	for (int i = 0 ; i < elf_hdr->e_phnum; i++) {
		/* 1 is PT_LOAD, Loadable Segment */
		if (p_hdr->p_type == PT_LOAD) {

			if (p_hdr->p_filesz > p_hdr->p_memsz) {
				kprintf("Error: Incorrect ELF Format\n");
				return -1;
			}
			/* mmap the virtual addresses */
			va_t *addr = (va_t *)mmap(p_hdr->p_vaddr, p_hdr->p_memsz, PTE_P | p_hdr->p_flags, HEAP);
			memcpy((void *)addr, (void *)((Elf64_Addr)elf_hdr + p_hdr->p_offset), p_hdr->p_filesz);
			memset((void *)(p_hdr->p_vaddr + p_hdr->p_filesz), 0, p_hdr->p_memsz - p_hdr->p_filesz);
		}
		p_hdr++;
	}

	/* mmap user stack for all the space. Allocate lazily, one by one page */
	mmap(STACK_LIMIT, STACK_TOP - STACK_LIMIT, PTE_P | PTE_W | PTE_U, STACK);
	pcb->u_rsp = ((uint64_t)STACK_TOP - 8);
	pcb->mm->brk = HEAP_START;
	mmap(HEAP_START, HEAP_END-HEAP_START, PTE_P | PTE_W | PTE_U, HEAP);

	return 0;
}
