#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/idt.h>
#include <sys/memory.h>
#include <sys/debug.h>

static pml_t *pml;
static uint64_t phys_base;
static uint64_t phys_free;
static uint64_t phys_end;
static page_dir_t *free_list_ptr = 0;
static page_dir_t *pd_prev = 0;
static uint64_t total_pages = 0;

pa_t page2pa(page_dir_t *ptr)
{
	return ((pa_t)((((uint64_t)ptr - (uint64_t)phys_free - (uint64_t)KERNBASE)/sizeof(page_dir_t)*PG_SIZE)));
}

va_t pa2va(pa_t pa)
{
	return (pa + KERNBASE);
}

uint64_t *kmalloc(const uint64_t size)
{
	if (size <= 0)
		return 0;

	int pgr = 0;
	uint64_t t_size = size;
	page_dir_t *fp = free_list_ptr;

	if (size < PG_SIZE && size)
		pgr = 1;
	else {
		pgr = t_size/PG_SIZE;
		t_size -= (pgr * PG_SIZE);
		if (t_size > 0)
			pgr++;
	}

#ifdef MALLOC_DEBUG
	kprintf("Page req %d\n", pgr);
#endif

	if (!fp)
		return 0;

	uint64_t *start_address = (uint64_t *)pa2va(page2pa(fp));

	int i;
	for (i = 0; i < pgr; i++) {
		fp->acc = 0;
		memset((void *)pa2va(page2pa(fp)), 0, PG_SIZE);
#ifdef MALLOC_DEBUG
		kprintf("Page Address %p\n", pa2va(page2pa(fp)));
#endif
		fp = fp->next;
	}

	free_list_ptr = fp;

	return start_address;
}

pa_t *get_free_pages(uint64_t n)
{
	if (!free_list_ptr)
		return 0;
	int i = n;
	page_dir_t *ptr = free_list_ptr;
	while (i--) {
		free_list_ptr->acc = 0;
		free_list_ptr = free_list_ptr->next;
	}
	ptr = (page_dir_t *)((((uint64_t)ptr - (uint64_t)phys_free - (uint64_t)KERNBASE)/sizeof(page_dir_t)*PG_SIZE));
	
	/* TODO: Remove ?? -> ZERO OUT PAGE */
	memset((void *)((uint64_t)ptr + KERNBASE), 0, PG_SIZE);
	return (pa_t *)ptr;
}

void create_page_disc(uint64_t start, uint64_t length, void *physbase)
{
	page_dir_t *pd_cur = (page_dir_t *)(phys_free + KERNBASE);
	uint64_t no_page = length/PG_SIZE;
	total_pages += no_page;
	uint64_t end = (start + length)/PG_SIZE;

	uint64_t i = start/PG_SIZE;
	for (; i < no_page; i++) {
		if (i*PG_SIZE < (uint64_t)phys_free) {
			pd_cur[i].acc = 0;
			pd_cur[i].next = 0;
		} else if (i < ((phys_free + end*sizeof(page_dir_t))/PG_SIZE)) {
			pd_cur[i].acc = 0;
			pd_cur[i].next = 0;
		} else {
			if (free_list_ptr == 0) {
				free_list_ptr = (page_dir_t *)(pd_cur + i);
#ifdef PG_DEBUG
				kprintf("FP %p \n", free_list_ptr);
#endif
			}
			pd_cur[i].acc = 1;
			pd_cur[i].next = 0;
			if (pd_prev)
				pd_prev->next = (pd_cur + i);
			pd_prev = (pd_cur + i);
		}
	}
}

pte_t *get_pte_from_pgdir(pgdir_t *pgdir, va_t va)
{
	pte_t *pte_ptr;
	pte_t  *pte;
#ifdef PG_DEBUG
	kprintf(" PDX %x ", PDX(va));
	kprintf(" pgdir %p ", pgdir);
#endif
	if (!(pgdir[PDX(va)] & PTE_P)) {
		pte_ptr = (pte_t *)(pte_t)get_free_pages(1);
#ifdef PG_DEBUG
		kprintf(" pte %p ", pte_ptr);
#endif
		pgdir[PDX(va)] = ((pgdir_t)pte_ptr | PTE_P | PTE_U | PTE_W);
	} else {
		pte_ptr = (pte_t *)(pgdir[PDX(va)] & ~(0xFFF));
#ifdef PG_DEBUG
		kprintf(" UA PGDIR %p ", pte_ptr);
#endif
	}

	pte = &pte_ptr[PTX(va)];
	return pte;
}

pte_t *get_pte_from_pdpe(pdpe_t *pdpe, va_t va)
{
	pgdir_t *pgdir;
	pte_t  *pte;
#ifdef PG_DEBUG
	kprintf(" PDPE %p ", pdpe[PDPE(va)]);
#endif
	if (!(pdpe[PDPE(va)] & PTE_P)) {
		pgdir = (pgdir_t *)(pgdir_t)get_free_pages(1);
		pdpe[PDPE(va)] = ((pdpe_t)pgdir | PTE_P | PTE_U | PTE_W);
	} else {
		pgdir = (pgdir_t *)(pdpe[PDPE(va)] & ~(0xFFF));
#ifdef PG_DEBUG
		kprintf(" ISSUE PDPE ");
#endif
	}

	pte = get_pte_from_pgdir(pgdir, va);
	return pte;
}

pte_t *get_pte_from_pml(pml_t *pml, va_t va)
{
	pdpe_t *pdpe;
	pte_t  *pte;

#ifdef PG_DEBUG
	kprintf(" PML %x ", PML4(va));
#endif
	if (!(pml[PML4(va)] & PTE_P)) {
		pdpe = (pdpe_t *)(pdpe_t)get_free_pages(1);
#ifdef PG_DEBUG
		kprintf(" pdpe %p ", pdpe);
#endif
		pml[PML4(va)] = ((pml_t)pdpe | PTE_P | PTE_U | PTE_W);
	} else {
		pdpe = (pdpe_t *)(pml[PML4(va)] & ~(0xFFF));
#ifdef PG_DEBUG
		kprintf(" ISSUE PML %p ", pdpe);
#endif
	}
	pte = get_pte_from_pdpe(pdpe, va);
	return pte;
}

void map_page_entry(pml_t *pml, va_t va, uint64_t size, pa_t pa, uint64_t perm)
{
	pte_t *pte_ptr;
	int i;
	for (i = 0; i < size; i += PG_SIZE) {
		pte_ptr = get_pte_from_pml(pml, va + i);
		if (pte_ptr) {
#ifdef PG_DEBUG
			kprintf(" v %p p %p ptr %p\n", va + i, pa + i, pte_ptr);
#endif
			*pte_ptr = pa + i;
			*pte_ptr |= (PTE_P | PTE_U | PTE_W);
		}
	}
}

void page_table_init()
{
	pml = (pml_t *)(pml_t)get_free_pages(1);
#ifdef PG_DEBUG
	kprintf("pml %p ", pml);
#endif
	map_page_entry(pml, (va_t)VA, (phys_end - phys_base), (pa_t)phys_base, 0);
	/* TODO: 4 pages required ? */
	map_page_entry(pml, (va_t)(KERNBASE + 0xB8000), 4 * 4096, (pa_t)0xB8000, 0);
}

void memory_init(uint32_t *modulep, void *physbase, void *physfree)
{

	struct smap_t {
		uint64_t base, length;
		uint32_t type;
	}__attribute__((packed)) *smap;

	phys_free = (uint64_t)physfree;
	while(modulep[0] != 0x9001) modulep += modulep[1]+2;
	for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
		if (smap->type == 1 /* memory */ && smap->length != 0) {
			kprintf("Available Physical Memory [%p-%p] Length %p\n", smap->base, smap->base + smap->length, smap->length);
			if (smap->base == 0) {
				create_page_disc(smap->base, smap->length, physbase);
			} else {
				create_page_disc(smap->base, smap->length, physbase);
				phys_end = smap->base + smap->length;
				/* TODO Remove this */
				break;
			}
		}
	}
	phys_base = (uint64_t)physbase;
	kprintf("physbase %p\n", (uint64_t)physbase);
	kprintf("physfree %p\n", (uint64_t)physfree);
	kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
	page_table_init();
	__asm__ volatile("mov %0, %%cr3":: "b"(pml));
	change_console_ptr();
	kprintf("Hello World %p \n");
	kprintf("Alloc 1 %p \n", kmalloc(1));
	kprintf("Alloc 5000 %p \n", kmalloc(5000));
	kprintf("Alloc 10000 %p \n", kmalloc(10000));
}
