#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/idt.h>
#include <sys/memory.h>
#include <sys/debug.h>
#include <sys/config.h>

pml_t *pml;
uint64_t phys_base;
uint64_t phys_free;
uint64_t phys_end;
page_disc_t *free_list_ptr = 0;
static page_disc_t *pd_prev = 0;
static uint64_t total_pages = 0;
/* Head of the running linked list for yield */
extern pcb_t *cur_pcb;



pa_t page2pa(page_disc_t *ptr)
{
	return ((pa_t)((((uint64_t)ptr - (uint64_t)phys_free - (uint64_t)KERNBASE)/sizeof(page_disc_t)*PG_SIZE)));
}

va_t pa2va(pa_t pa)
{
	return (pa + (pa_t)KERNBASE);
}

pa_t get_free_pages(uint64_t n)
{
	if (!free_list_ptr)
		return 0;
#ifdef MALLOC_DEBUG
	kprintf("GFP start : fp %p\n", free_list_ptr);
#endif
	int i = n;
	page_disc_t *ptr = free_list_ptr;
	while (i--) {
		ptr->acc = 0;
		ptr = free_list_ptr->next;
	}

	pa_t pa = page2pa(free_list_ptr);
	free_list_ptr = ptr;
#ifdef MALLOC_DEBUG
	kprintf("GFP:pa %p\n", pa);
	kprintf("GFP end : fp %p\n", free_list_ptr);
#endif
	/* TODO: Remove ?? -> ZERO OUT PAGE */
	memset((void *)pa2va(pa), 0, PG_SIZE);
	return pa;
}


/* Assumed Aligned addresses. MMAP should be the only one callling this. */
void allocate_vma(pcb_t *pcb, vma_t *vma)
{
#ifdef MALLOC_DEBUG
		kprintf("va %p, size %x\n", va, size);
#endif
	va_t va_start = (vma->start/PG_SIZE*PG_SIZE);
	uint64_t va_size = vma->end - va_start;

#ifdef MALLOC_DEBUG
	kprintf("va start %x, va size %x\n", va_start, va_size);
#endif
	pa_t pa;
	int i = 0;
	for (; i <= va_size; i += PG_SIZE) {
		pa = get_free_pages(1);
		map_page_entry((pml_t *)pa2va((pa_t)pcb->pml4), (va_t)va_start, 0x1000, (pa_t)pa, PTE_P | vma->flags);
		/* Kernel allocates vmas for the user process. So it does not have the va in page tables.
		 * and hence should not zero out it.
		 * And zero out is done is get_free_pages() only. */
		/* memset((void *)va_start, 0, PG_SIZE); */
#ifdef MALLOC_DEBUG
		kprintf("Address va pa%p\n", pa2va(pa), va);
#endif
		va_start += PG_SIZE;
	}
}

va_t kmalloc(const uint64_t size)
{
	if (size <= 0)
		return 0;

	int pgr = 0;
	uint64_t t_size = size;
	pa_t pa;

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
	va_t start_address = 0;
	int i;
	for (i = 0; i < pgr; i++) {
		pa = get_free_pages(1);
		if (start_address == 0) {
			start_address = pa2va(pa);
		}
		memset((void *)pa2va(pa), 0, PG_SIZE);
#ifdef MALLOC_DEBUG
		kprintf("Address va pa%p\n", pa2va(pa), va);
#endif
	}
	return start_address;
}

/* Currently this is a global variable shared by all user processes.
 * Make it specific to a user. */
va_t start_va = 0x1000;

va_t kmalloc_user(pcb_t *pcb, const uint64_t size)
{
	if (size <= 0)
		return 0;


	int pgr = 0;
	uint64_t t_size = size;
	pa_t pa;

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
	va_t start_address = start_va;
	int i;
	for (i = 0; i < pgr; i++) {
		pa = get_free_pages(1);
		map_page_entry((pml_t *)pa2va((pa_t)pcb->pml4), (va_t)start_va, 0x1000, (pa_t)pa, PTE_P | PTE_W | PTE_U);
		/* Kernel cannot write these pages as these pages are mapped in users page tables. */
		/* memset((void *)start_va, 0, PG_SIZE); */
#ifdef MALLOC_DEBUG
		kprintf("Address va pa%p\n", pa2va(pa), va);
#endif
		start_va += 0x1000;
	}
	return start_address;
}

void create_page_disc(uint64_t start, uint64_t length, void *physbase)
{
	page_disc_t *pd_cur = (page_disc_t *)(phys_free + KERNBASE);
	uint64_t no_page = length/PG_SIZE;
	total_pages += no_page;
	uint64_t end = (start + length)/PG_SIZE;

	uint64_t i = start/PG_SIZE;
	for (; i < no_page; i++) {
		if (i*PG_SIZE < (uint64_t)phys_free) {
			pd_cur[i].acc = 0;
			pd_cur[i].next = 0;
		} else if (i < ((phys_free + end*sizeof(page_disc_t))/PG_SIZE)) {
			pd_cur[i].acc = 0;
			pd_cur[i].next = 0;
		} else {
			if (free_list_ptr == 0) {
				free_list_ptr = (page_disc_t *)(pd_cur + i);
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

pte_t *get_pte_from_pgdir(pgdir_t *pgdir, va_t va, uint8_t perm)
{
	pte_t *pte_ptr;
	pte_t  *pte;
#ifdef PG_DEBUG
	kprintf(" pgdir %p ", pgdir);
#endif
	if (!(pgdir[PDX(va)] & PTE_P)) {
		pte_ptr = (pte_t *)(pte_t)get_free_pages(1);
		pgdir[PDX(va)] = ((pgdir_t)pte_ptr | perm);
	} else {
		pte_ptr = (pte_t *)(pgdir[PDX(va)] & ~(0xFFF));
		pgdir[PDX(va)] = ((pgdir_t)pte_ptr | perm);
#ifdef PG_DEBUG
		kprintf(" ISSUE PGDIR ");
#endif
	}

	pte_ptr = (pte_t *)pa2va((pa_t)pte_ptr);

	pte = &pte_ptr[PTX(va)];
	return ((pte_t *)pte);
}

pte_t *get_pte_from_pdpe(pdpe_t *pdpe, va_t va, uint8_t perm)
{
	pgdir_t *pgdir;
	pte_t  *pte;
#ifdef PG_DEBUG
	kprintf(" PDPE %p ", pdpe);
#endif
	if (!(pdpe[PDPE(va)] & PTE_P)) {
		pgdir = (pgdir_t *)get_free_pages(1);
		pdpe[PDPE(va)] = ((pdpe_t)pgdir | perm);
	} else {
		pgdir = (pgdir_t *)(pdpe[PDPE(va)] & (0xFFFFFFFFfffff000));
		pdpe[PDPE(va)] = ((pdpe_t)pgdir | perm);
#ifdef PG_DEBUG
		kprintf(" ISSUE PDPE ");
#endif
	}

	pte = get_pte_from_pgdir((pgdir_t *)pa2va((pa_t)pgdir), va, perm);
	return pte;
}

pte_t *get_pte_from_pml(pml_t *pml, va_t va, uint8_t perm)
{
	pdpe_t *pdpe;
	pte_t  *pte;

#ifdef PG_DEBUG
	kprintf(" PML %x ", PML4(va));
#endif
	if (!(pml[PML4(va)] & PTE_P)) {
		pdpe = (pdpe_t *)get_free_pages(1);
		pml[PML4(va)] = ((pml_t)pdpe | perm);
	} else {
		pdpe = (pdpe_t *)(pml[PML4(va)] & ~(0xFFF));
		pml[PML4(va)] = ((pml_t)pdpe | perm);
#ifdef PG_DEBUG
		kprintf(" ISSUE PML");
#endif
	}
	pte = get_pte_from_pdpe((pdpe_t *)pa2va((pa_t)pdpe), va, perm);
	return pte;
}

void map_page_entry(pml_t *pml, va_t va, uint64_t size, pa_t pa, uint64_t perm)
{
	pte_t *pte_ptr;
	int i;
	for (i = 0; i < size; i += PG_SIZE) {
		pte_ptr = get_pte_from_pml(pml, va + i, perm);
		if (pte_ptr) {
#ifdef PG_DEBUG
			kprintf(" v %p p %p ptr %p\n", va + i, pa + i, pte_ptr);
#endif
			*pte_ptr = pa + i;
			*pte_ptr = ((*pte_ptr) | perm);
		}
	}
}

void page_table_init()
{
	pml = (pml_t *)get_free_pages(1);
#ifdef PG_DEBUG
	kprintf("pml %p ", pml);
#endif
	/* TODO: Map only these regions ? */
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)VA, ((pa_t)phys_end - (pa_t)phys_base), (pa_t)phys_base, PTE_P | PTE_W);
	/* TODO: 2 pages required ? */
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)(KERNBASE + 0xB8000), 4 * 0x1000, (pa_t)0xB8000, PTE_P | PTE_W);
}

/* This function sets kernel and console page tables for a user process. */
void set_proc_page_table(pcb_t *pcb)
{
	if (pcb->is_usr) {
#if	ENABLE_USER_PAGING
		pml_t *usr_pml = (pml_t *)get_free_pages(1);
		uint64_t *v_usr_pml = (uint64_t *)pa2va((uint64_t)usr_pml);
		uint64_t *v_kern_pml = (uint64_t *)pa2va((uint64_t)pml);
		v_usr_pml[511] = v_kern_pml[511];
		pcb->pml4 = (uint64_t)usr_pml;
	/* If use paging is not enabled, set every processes pml as kernel's pml */
#else
		pcb->pml4 = (uint64_t)pml;
#endif
	} else {
		pcb->pml4 = (uint64_t)pml;
	}
}

vma_t *check_addr_in_vma_list(va_t addr, vma_t *head)
{
	if (!head)
		return 0;
	vma_t *vma = head;
	while (vma) {
		if (addr >= vma->start && addr < vma->end)
			return vma;
		vma = vma->next;
	}
	return 0;
}

/* Helper function for debugging */
void print_vmas(vma_t *head)
{
	while (head) {
		kprintf("st:%x en:%x\t", head->start, head->end);
		head = head->next;
	}
}

vma_t *get_empty_vma(va_t addr, uint64_t size, mm_struct_t *mm)
{
	vma_t *h = mm->head, *vma, *h_p = 0;
	uint64_t end = addr + size;

	/* Head is NULL */
	if (!h) {
		vma = (vma_t *)kmalloc(PG_SIZE);
		vma->next = 0;
		mm->head = vma;
		return vma;
	}

	while (h) {
		/* Address already mapped, so kernel cannot map */
		/* 4 possible combinations of intersections */
		if (addr >= h->start && addr < h->end) {
			return 0;
		} else if (end >= h->start && (end < h->end)) {
			return 0;
		} else if (h->start >= addr && h->start < end) {
			return 0;
		} else if (h->end >= addr && h->end < end) {
			return 0;
		/* if entire vma mapping is before current vma */
		} else if ((addr < h->start) && ((addr + size) < h->start)) {
			vma = (vma_t *)kmalloc(PG_SIZE);
			/* Before head*/
			if (!h_p) {
				mm->head = vma;
				vma->next = h;
				return vma;
			/* Somewhere Middle */
			} else {
				h_p->next = vma;
				vma->next = h;
				return vma;
			}
		}
		h_p = h;
		h = h->next;
	}

	/* vma needs to appended at the end */
	vma = (vma_t *)kmalloc(PG_SIZE);
	h_p->next = vma;
	vma->next = 0;
	return vma;
}

va_t mmap(va_t va_start, uint64_t size, uint64_t flags)
{
	/* Kernel chooses */
	if (va_start == 0) {
	}

	uint64_t rstart = round_down(va_start, PG_SIZE);
	uint64_t rsize = round_up(size, PG_SIZE);

	pcb_t *pcb = cur_pcb;
	mm_struct_t *mm = pcb->mm;

	vma_t *vma = get_empty_vma(rstart, rsize, mm);
	if (!vma)
		return 0;

	vma->start = rstart;
	vma->end = vma->start + rsize;
	vma->type = HEAP;
	vma->flags = flags;
	return vma->start;
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
	kprintf("Hello World\n");

#if	ENABLE_USER_PAGING
#else
	/* No need to map something to user space. Paging betweeen processes is working. */
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)VA, ((pa_t)phys_end - (pa_t)phys_base), (pa_t)phys_base, PTE_P | PTE_W | PTE_U);
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)(KERNBASE + 0xB8000), 4 * 0x1000, (pa_t)0xB8000, PTE_P | PTE_W | PTE_U);
#endif

#if 0
	/* Kmalloc_user signature has changed, write more concrete kmalloc_user test cases */
	va_t *ptr;
	ptr = (va_t *)kmalloc_user(0x1000);
	ptr[0] = 'h';
	ptr[1] = '\0';
	kprintf("Alloc Done %s\n", ptr);
	ptr = (va_t *)kmalloc_user(0x1000);
	ptr[0] = 'w';
	ptr[1] = '\0';
	kprintf("Alloc Done %s\n", ptr);
	ptr = (va_t *)kmalloc(0x1000);
	ptr[0] = 'H';
	ptr[1] = '\0';
	kprintf("Alloc Done %s\n", ptr);
	ptr = (va_t *)kmalloc(0x1000);
	ptr[0] = 'W';
	ptr[1] = '\0';
	kprintf("Alloc Done %s\n", ptr);
#endif
}
