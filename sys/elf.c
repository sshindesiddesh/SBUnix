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
#ifndef ELF_LOAD_MMAP
	mm_t *mm = pcb->mm = NULL;
	vma_t *vma;

#ifdef TARFS_DEBUG
	if (!mm)
		kprintf("\tPCB MM empty");
#endif
#endif
	pcb->entry = elf_hdr->e_entry;

	for (int i = 0 ; i < elf_hdr->e_phnum; i++) {
		/* 1 is PT_LOAD, Loadable Segment */
		if (p_hdr->p_type == PT_LOAD) {

			if (p_hdr->p_filesz > p_hdr->p_memsz) {
				kprintf("Error: Incorrect ELF Format\n");
				return -1;
			}
#ifdef ELF_LOAD_MMAP
			va_t *addr = (va_t *)mmap(p_hdr->p_vaddr, p_hdr->p_filesz, PTE_P | p_hdr->p_flags);
			memcpy((void *)addr, (void *)((Elf64_Addr)elf_hdr + p_hdr->p_offset), p_hdr->p_filesz);
#else
			vma = (vma_t *)kmalloc(PG_SIZE);
			vma->start = p_hdr->p_vaddr;
			vma->end = p_hdr->p_vaddr + p_hdr->p_filesz;
			vma->flags = p_hdr->p_flags;
			/* TODO: Check this */
			vma->type = OTHER;
			vma->file = 0;
			vma->vm_mm = mm;

			allocate_vma(pcb, vma);
			memcpy((void *)vma->start, (void *)((Elf64_Addr)elf_hdr + p_hdr->p_offset), p_hdr->p_filesz);
#ifdef TARFS_DEBUG
			kprintf("start:%p size:%x\n", vma->start, p_hdr->p_filesz);
#endif
#endif
		}
		p_hdr++;
	}

	return 0;
}
