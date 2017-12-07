#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/idt.h>
#include <sys/memory.h>
#include <sys/debug.h>
#include <sys/config.h>
#include <sys/process.h>
#include <sys/kutils.h>

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

page_disc_t *pa2page(pa_t pa)
{
	return (page_disc_t *)((uint64_t)phys_free + ((pa_t)pa*sizeof(page_disc_t)/PG_SIZE) + KERNBASE);
}

va_t pa2va(pa_t pa)
{
	return (pa + (pa_t)KERNBASE);
}

pa_t va2pa(va_t va)
{
	return (va - (va_t)KERNBASE);
}

/* Adds physicall page to the free list */
void add_free_page(pa_t pa)
{
#if 0
	kprintf("ADD PAGE : %p\n", pa);
	kprintf("#ADD PAGE : %p pd :%p*\n", pa, pd);
#endif
	page_disc_t *pd = pa2page(pa);
	/* Page reffered by any process  */
	if (pd->ref_cnt != 0) {
		kprintf("!!Page reference count more than 1!! %d\n", pd->ref_cnt);
		while (1);
	}

	pd->ref_cnt = 0;

	/* Page free bit is set */
	if (pd->free) {
		kprintf("!!Page is already free!!");
		while (1);
	}

	pd->free = 1;

	/* Page not aligned, means still in use */
	if (pa & 0xFFF) {
		kprintf("!!Page bits are set, might create problem in free list !!");
		while (1);
	}

	pd->next = free_list_ptr;
	free_list_ptr = pd;
}

pa_t get_free_pages(uint64_t n)
{
	if (n > 1) {
		kprintf("More than one physical page requested\n");
		while (1);
	}
	if (!free_list_ptr)
		return 0;
#ifdef MALLOC_DEBUG
	kprintf("#GFP:pa %p fp %p*\n", page2pa(free_list_ptr), free_list_ptr);
#endif
	if (free_list_ptr->free == 0) {
		kprintf("!!!Allocated page in the free list!!!\n");
		while (1);
	}
	page_disc_t *ptr = free_list_ptr;
	if (ptr->free) {
		ptr->free = 0;
	} else {
		kprintf("!!! Memory Inconsistency !!!\n");
		while (1);
	}

	if (ptr->ref_cnt != 0) {
		kprintf("!!! Free Page ref count greater than zero !!! %d Page %p\n", ptr->ref_cnt, page2pa(ptr));
		while (1);
	}

	ptr->ref_cnt = 1;

	pa_t pa = page2pa(free_list_ptr);
	free_list_ptr = free_list_ptr->next;

#ifdef MALLOC_DEBUG
	kprintf("GFP:pa %p\n", pa);
	kprintf("GFP end : fp %p\n", free_list_ptr);
#endif
	/* TODO: Remove ?? -> ZERO OUT PAGE */
	memset((void *)pa2va(pa), 0, PG_SIZE);
	return pa;
}

/* Deallocate all page tables */

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
	for (; i < va_size; i += PG_SIZE) {
		/* TODO: kmalloc ??? */
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

void allocate_page_in_vma(pcb_t *pcb, vma_t *vma, va_t faultAddr)
/* Assumed Aligned addresses. MMAP should be the only one callling this. */
{
#ifdef malloc_debug
		kprintf("va %p, size %x\n", va, size);
#endif

#ifdef MALLOC_DEBUG
	kprintf("va start %x, va size %x\n", va_start, va_size);
#endif
	pa_t pa;
	if (faultAddr >= vma->start && faultAddr < vma->end) {
		pa = get_free_pages(1);
		map_page_entry((pml_t *)pa2va((pa_t)pcb->pml4), (va_t)faultAddr, PG_SIZE, (pa_t)pa, PTE_P | vma->flags);
	} else {
		kprintf("!!!Memory Corruption. Allocation request for untracked memory!!!");
		while (1);
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
	/* TODO : Remove this at the end. */
	if (pgr != 1) {
		kprintf("!!!Error : More than 1 page requested. !!!\n");
		while (1);
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
	/* TODO: use pages below physfree */
		if (i*PG_SIZE < (uint64_t)phys_free) {
			pd_cur[i].ref_cnt = 0;
			pd_cur[i].next = 0;
		} else if (i < ((phys_free + end*sizeof(page_disc_t))/PG_SIZE)) {
			pd_cur[i].ref_cnt = 0;
			pd_cur[i].next = 0;
		} else {
			if (free_list_ptr == 0) {
				free_list_ptr = (page_disc_t *)(pd_cur + i);
#ifdef PG_DEBUG
				kprintf("FP %p \n", free_list_ptr);
#endif
			}
			/* This is -1 as kernel maps it once and increments count */
			pd_cur[i].ref_cnt = 0;
			pd_cur[i].next = 0;
			/* This page is free */
			pd_cur[i].free = 1;
			if (pd_prev)
				pd_prev->next = (pd_cur + i);
			pd_prev = (pd_cur + i);
		}
	}
}

pte_t *get_pte_from_pgdir_unmap(pgdir_t *pgdir, va_t va, uint64_t perm)
{
	pte_t *pte_ptr = 0;
	pte_t  *pte = 0;
#ifdef PG_DEBUG
	kprintf(" pgdir %p ", pgdir);
#endif
	if ((pgdir[PDX(va)] & PTE_P)) {
		pte_ptr = (pte_t *)(pgdir[PDX(va)] & ~(0xFFF));
#ifdef PG_DEBUG
		kprintf(" ISSUE PGDIR ");
#endif
		pte_ptr = (pte_t *)pa2va((pa_t)pte_ptr);

		pte = &pte_ptr[PTX(va)];
		return ((pte_t *)pte);
	}
	return 0;
}

pte_t *get_pte_from_pdpe_unmap(pdpe_t *pdpe, va_t va, uint64_t perm)
{
	pgdir_t *pgdir = 0;
	pte_t  *pte = 0;
#ifdef PG_DEBUG
	kprintf(" PDPE %p ", pdpe);
#endif
	if ((pdpe[PDPE(va)] & PTE_P)) {
		pgdir = (pgdir_t *)(pdpe[PDPE(va)] & ~(0xFFF));
#ifdef PG_DEBUG
		kprintf(" ISSUE PDPE ");
#endif
		pte = get_pte_from_pgdir_unmap((pgdir_t *)pa2va((pa_t)pgdir), va, perm);
		return pte;
	}
	return 0;
}

pte_t *get_pte_from_pml_unmap(pml_t *pml, va_t va, uint64_t perm)
{
	pdpe_t *pdpe = 0;
	pte_t  *pte = 0;

#ifdef PG_DEBUG
	kprintf(" PML %x ", PML4(va));
#endif
	if ((pml[PML4(va)] & PTE_P)) {
		pdpe = (pdpe_t *)(pml[PML4(va)] & ~(0xFFF));
#ifdef PG_DEBUG
		kprintf(" ISSUE PML");
#endif
		pte = get_pte_from_pdpe_unmap((pdpe_t *)pa2va((pa_t)pdpe), va, perm);
		return pte;
	}
	return 0;
}

void unmap_page_entry(pml_t *pml, va_t va, uint64_t size, pa_t pa, uint64_t perm)
{
	pte_t *pte_ptr;
	int i;
	for (i = 0; i < size; i += PG_SIZE) {
		pte_ptr = get_pte_from_pml_unmap(pml, va + i, perm);
		if (pte_ptr) {
#ifdef PG_DEBUG
			kprintf(" v %p p %p ptr %p\n", va + i, pa + i, pte_ptr);
#endif
			*pte_ptr = 0;
		}
	}
	/* Flush TLB entries */
	__flush_tlb();
}

pte_t *get_pte_from_pgdir(pgdir_t *pgdir, va_t va, uint64_t perm)
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

pte_t *get_pte_from_pdpe(pdpe_t *pdpe, va_t va, uint64_t perm)
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

pte_t *get_pte_from_pml(pml_t *pml, va_t va, uint64_t perm)
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
	__flush_tlb();
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
	/* 25 * 80 * 2 = 4000 <= PGSIZE */
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)(KERNBASE + 0xB8000), 1 * 0x1000, (pa_t)0xB8000, PTE_P | PTE_W);
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
	kprintf("\n");
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

		/* addr lies in head. cannot map */
		if (addr >= h->start && addr < h->end) {
			return 0;
		/*  */
		} else if (end > h->start && end < h->end) {
			return 0;
		} else if (h->start >= addr && h->start < end) {
			return 0;
		} else if (h->end > addr && h->end < end) {
			return 0;
		/* if entire vma mapping is before current vma */
		} else if ((addr < h->start) && (end <= h->start)) {
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

va_t mmap(va_t va_start, uint64_t size, uint64_t flags, uint64_t type)
{
	/* Kernel chooses */
	if (va_start == 0) {
	}

	uint64_t rstart = round_down(va_start, PG_SIZE);
	uint64_t rend = round_up(va_start + size, PG_SIZE);
	uint64_t rsize = rend - rstart;

	pcb_t *pcb = cur_pcb;
	mm_struct_t *mm = pcb->mm;

	vma_t *vma = get_empty_vma(rstart, rsize, mm);
	if (!vma)
		return 0;

	vma->start = rstart;
	vma->end = rend;
	vma->type = type;
	vma->flags = flags;
	return vma->start;
}

va_t kmmap(va_t va_start, uint64_t size, uint64_t flags, uint64_t type) {
	return mmap(va_start, size, flags, type);
}

va_t munmap(va_t va_start, uint64_t size)
{
	/* Kernel chooses */
	if (va_start == 0) {
	}

	uint64_t addr = round_down(va_start, PG_SIZE);
	uint64_t end = addr + round_up(size, PG_SIZE);

	pcb_t *pcb = cur_pcb;
	mm_struct_t *mm = pcb->mm;

	vma_t *h = mm->head, *vma, *h_p = 0;

	/* Head is NULL */
	if (!h) {
		return -1;
	}

	while (h) {
		/* 4 possible combinations of intersections */
		/* smaller inbetween range : unmap in between existing vma. 
		 * split them into two (h->start:addr) and (end:h->end) */
		if (addr > h->start && end < h->end) {
			/* First vma */
			h->end = addr;
			/*second vma*/
			vma = (vma_t *)kmalloc(PG_SIZE);
			vma->start = end;
			vma->end = h->end;
			vma->flags = h->flags;
			/* update links */
			vma->next = h->next;
			h->next = vma;
		/* exact or larger range : unmap for exact or larger vma. remove it */
		} else if (addr <= h->start && end >= h->end) {
			vma = h;
			/* TODO: kfree */
			/* free range -> (vma->start:vma->end) */
			unmap_page_entry((pml_t *)pa2va((pa_t)cur_pcb->pml4), (va_t)vma->start, vma->end - vma->start, 0, 0);
			if (!h_p) {
				mm->head = h->next;
				h_p = 0;
				h = h->next;
				continue;
			} else {
				h_p->next = h->next;
			}
		/* smaller left range : unmap for lower portion of vma. update the start address of vma.
		 * h->start = end . new vma (end, h->end) */
		} else if (h->start >= addr && h->start <= end) {
			vma = h;
			/* TODO: kfree(vma) */
			/* free range -> (vma->start:end) */
			unmap_page_entry((pml_t *)pa2va((pa_t)cur_pcb->pml4), (va_t)vma->start, end - vma->start, 0, 0);
			h->start = end;
		/* smaller right range : unmap for higher portion of vma. update the end address of vma.
		 * h->end = addr . new vma (h->start, addr) */
		} else if (addr >= h->start && addr <= h->end) {
			vma = h;
			/* TODO: kfree(vma) */
			/* free range -> (addr:vma->end) */
			unmap_page_entry((pml_t *)pa2va((pa_t)cur_pcb->pml4), (va_t)addr, vma->end - addr, 0, 0);
			h->end = addr;
		}
		h_p = h;
		h = h->next;
	}

	return 0;
}

va_t kmunmap(va_t va_start, uint64_t size)
{
	return munmap(va_start, size);
}

pcb_t *create_kernel_process(void *func);

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
			//kprintf("Available Physical Memory [%p-%p] Length %p\n", smap->base, smap->base + smap->length, smap->length);
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
	//kprintf("physbase %p\n", (uint64_t)physbase);
	//kprintf("physfree %p\n", (uint64_t)physfree);
	//kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);

	page_table_init();

	__asm__ volatile("mov %0, %%cr3":: "b"(pml));
	change_console_ptr();
	//kprintf("Hello World\n");

#if	ENABLE_USER_PAGING
#else
	/* No need to map something to user space. Paging betweeen processes is working. */
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)VA, ((pa_t)phys_end - (pa_t)phys_base), (pa_t)phys_base, PTE_P | PTE_W | PTE_U);
	map_page_entry((pml_t *)pa2va((pa_t)pml), (va_t)(KERNBASE + 0xB8000), 4 * 0x1000, (pa_t)0xB8000, PTE_P | PTE_W | PTE_U);
#endif
}

/* Copy parent's page table's to child. */
void copy_vma_mapping(pcb_t *parent, pcb_t *child)
{
	vma_t *p_vma = parent->mm->head, *c_vma;
	va_t start, end;
	pte_t *pte;
	pa_t pa;

	/* Copy kernel page mappings */
	child->pml4 = (pml_t)get_free_pages(1);
	va_t *v_p_pml = (va_t *)pa2va((va_t)parent->pml4);
	va_t *v_c_pml = (va_t *)pa2va((va_t)child->pml4);
	v_c_pml[511] = v_p_pml[511];

	/* Copy user page mappings */
	while (p_vma) {
		c_vma = get_empty_vma(p_vma->start, p_vma->end - p_vma->start, child->mm);
		start = c_vma->start = p_vma->start;
		end = c_vma->end = p_vma->end;
		c_vma->type = p_vma->type;
		c_vma->flags = p_vma->flags;
		/* Map page by page */
		while (start < end) {
			/* Only last level entries are marked as COW */
			/* Mark parent page table entriess as COW and read only */
			pte = get_pte_from_pml_unmap((pml_t *)pa2va((pa_t)parent->pml4), start, p_vma->flags);
			if (pte && *pte) {
				pa = (pa_t)(*pte & ~(0xFFF));
				pa2page(pa)->ref_cnt++;
#if 0
				kprintf("#COWpa: %p refcnt %d\t*", pa, pa2page(pa)->ref_cnt);
#endif
				*pte = (pa | PTE_P | PTE_U | PTE_COW);
				/* Mark last level child page table entries as COW and read only */
				map_page_entry((pml_t *)pa2va((pa_t)child->pml4), start, PG_SIZE, (pa_t)pa, PTE_P | PTE_U | PTE_W | p_vma->flags);
				pte = get_pte_from_pml_unmap((pml_t *)pa2va((pa_t)child->pml4), start, PTE_P | p_vma->flags);
				if (pte) {
					pa = (pa_t)(*pte & ~(0xFFF));
					*pte = (pa | PTE_P | PTE_U | PTE_COW);
				}
			}
			start += PG_SIZE;
		}
		p_vma = p_vma->next;
	}
}

/* Deallocate physical pages allocated to a VMA */
void deallocate_vma(pcb_t *pcb, vma_t *vma)
{
	va_t va_start = vma->start;
	uint64_t va_size = vma->end - va_start;

	int i = 0;
	for (; i < va_size; i += PG_SIZE) {
		pte_t *pte = get_pte_from_pml_unmap((pml_t *)pcb->pml4, va_start + i, 0);
		if (*pte) {
			add_free_page((pa_t)*pte);
		}
		va_start += PG_SIZE;
	}
}

/* Deallocate all VMAs */
/* TODO: If this process has child or parent, VMAs are shared, so do not deallocate them */
void deallocate_all_vmas(pcb_t *pcb)
{
	mm_struct_t *mm = pcb->mm;
	if (!mm)
		return;

	vma_t *vma = mm->head, *l_vma;

	while (vma) {
		l_vma = vma;
		/* This is not required as all pages are freed from the page tables itself */
		/* deallocate_vma(pcb, vma); */
		vma = vma->next;

		pa2page(va2pa((va_t)l_vma))->ref_cnt--;
		/* Free VMA itself*/
		add_free_page(va2pa((va_t)l_vma));
	}
}

/* Deallocate PCB struct */
void deallocate_pcb(pcb_t *pcb)
{
	/* Deallocate all VMAs */
	deallocate_all_vmas(pcb);
	pa2page(va2pa((va_t)pcb->mm))->ref_cnt--;
	/* Deallocate mm */
	add_free_page(va2pa((va_t)pcb->mm));
	/* TODO : Deallocate current node ??? */
	/* add_free_page(pa2va((va_t)pcb->current_node)); */
}

#define PAGE_TABLE_SIZE	512

/* All pages allocated for page tables and actual usage are freed except pages which have COW bit set */
/* Input pcb->pml4 : physical address */
/* If a child exits and COW bit of few pages is on, decrease the reference count of those. */
/* Also decrease reference count of others as the mapping is undone */
void free_page_entry(pml_t *pml)
{
	pdpe_t *pdpe;
	pgdir_t *pgdir;
	pte_t *pte_ptr;
	pa_t pa;

	pml = (pml_t *)pa2va((pa_t)pml);

	int i, j, k, l;

	/* 1st level : PML */
	for (i = 0; i < (PAGE_TABLE_SIZE - 1); i++) {
		if (pml[i] & PTE_P) {
			pdpe = (pdpe_t *)(pml[i] & ~(0xFFF));
			pdpe = (pdpe_t *)(pa2va((pa_t)pdpe));
			/* 2nd level : PDPE */
			for (j = 0; j < PAGE_TABLE_SIZE; j++) {
				if (pdpe[j] & PTE_P) {
					pgdir = (pgdir_t *)(pdpe[j] & ~(0xFFF));
					pgdir = (pgdir_t *)pa2va((pa_t)(pgdir));
					/* 3rd level : PGDIR */
					for (k = 0; k < PAGE_TABLE_SIZE; k++) {
						if (pgdir[k] & PTE_P) {
							pte_ptr = (pte_t *)(pgdir[k] & ~(0xFFF));
							pte_ptr = (pte_t *)(pa2va((pa_t)pte_ptr));
							/* 4th level actual page table */
							for (l = 0; l < PAGE_TABLE_SIZE; l++) {
								pa = pte_ptr[l];
								if (pa & PTE_P) {
									pa = pa & ~(0xFFF);
									if (!pa)
										continue;
#if 0
									if (pa & PTE_COW) {
										pa = pa & ~(0xFFF);
									}
									kprintf("#pa %p ref_cnt %d\t*", pa, pa2page(pa)->ref_cnt);
									/* As page table entry is removed */
									/* Page is still used by another process If */
									/* COW is set and reference count is greater than 1 */
									if (pa & PTE_COW) {
										if (pa2page(pa)->ref_cnt == 0) {
											add_free_page(pa & ~(0xFFF));
										}
									} else {
										add_free_page(pa & ~(0xFFF));
									}
#endif
									if (pa2page(pa)->ref_cnt > 0) {
										pa2page(pa)->ref_cnt--;
									} else {
										kprintf("ref cnt less than zero %p\n", pa);
										while (1);
									}
									if (pa2page(pa)->ref_cnt == 0) {
										add_free_page(pa & ~(0xFFF));
									}
								}
							}
							pa2page(va2pa((va_t)pte_ptr))->ref_cnt--;
							add_free_page(va2pa((va_t)pte_ptr));
						}
					}
					pa2page(va2pa((va_t)pgdir))->ref_cnt--;
					add_free_page(va2pa((va_t)pgdir));
				}
			}
			pa2page(va2pa((va_t)pdpe))->ref_cnt--;
			add_free_page(va2pa((va_t)pdpe));
		}
	}
	__flush_tlb();
}
