#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>
#include <sys/idt.h>

#define PG_SIZE	4096
#define KERNBASE	0xffffffff80000000

#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PWT         0x008   // Write-Through
#define PTE_PCD         0x010   // Cache-Disable
#define PTE_A           0x020   // Accessed
#define PTE_D           0x040   // Dirty
#define PTE_PS          0x080   // Page Size
#define PTE_MBZ         0x180   // Bits must be zero

#define PML4SHIFT		39
#define PDPESHIFT		30
#define PTXSHIFT		12
#define PDXSHIFT		21

#define VA		(KERNBASE + phys_base)
#define PML4(la)	((((uint64_t) (la)) >> PML4SHIFT) & 0x1FF)
#define PDPE(la)	((((uint64_t) (la)) >> PDPESHIFT) & 0x1FF)
#define PDX(la)         ((((uint64_t) (la)) >> PDXSHIFT) & 0x1FF)
#define PTX(la)         ((((uint64_t) (la)) >> PTXSHIFT) & 0x1FF)

//extern char kernmem, physbase;
typedef struct page_dir {
	struct page_dir *next;
	int acc;
} page_dir_t;

uint64_t phys_base;
static uint64_t phys_free;
static uint64_t phys_end;
static page_dir_t *free_list_ptr = 0;
static page_dir_t *pd_prev = 0;
static uint64_t total_pages = 0;
//#define K_DEBUG

uint64_t *get_free_pages(uint64_t n)
{
	if (!free_list_ptr)
		return 0;
	int i = n;
	page_dir_t *ptr = free_list_ptr;
	while (i--) {
		free_list_ptr->acc = 0;
		free_list_ptr = free_list_ptr->next;
	}
	ptr = (page_dir_t *)(((uint64_t)ptr - phys_free)/sizeof(page_dir_t)*PG_SIZE);
	/* TODO: Remove ?? -> ZERO OUT PAGE */
	memset((void *)ptr, 0, PG_SIZE);
#ifdef K_DEBUG
	kprintf(" NP %p ", ptr);
#endif
	return (uint64_t *)ptr;
}

void create_page_disc(uint64_t start, uint64_t length, void *physbase)
{
	page_dir_t *pd_cur = (page_dir_t *)phys_free;
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
			}
			pd_cur[i].acc = 1;
			pd_cur[i].next = 0;
			if (pd_prev)
				pd_prev->next = (pd_cur + i);
			pd_prev = (pd_cur + i);
		}
	}
}

uint64_t *get_pte_from_pgdir(uint64_t *pgdir, uint64_t va)
{
	uint64_t *pte_ptr, *pte;
	#ifdef K_DEBUG
	kprintf(" PDX %x ", PDX(va));
	kprintf(" r pgdir %p ", pgdir);
	#endif
	if (!(pgdir[PDX(va)] & PTE_P)) {
		pte_ptr = get_free_pages(1);
		#ifdef K_DEBUG
		kprintf(" pte %p ", pte_ptr);
		#endif
		pgdir[PDX(va)] = ((uint64_t)pte_ptr | PTE_P | PTE_U | PTE_W);
	} else {
		pte_ptr = (uint64_t *)(pgdir[PDX(va)] & ~(0xFFF));
		#ifdef K_DEBUG
		kprintf(" ISSUE PGDIR %p ", pte_ptr);
		#endif
	}

	pte = &pte_ptr[PTX(va)];
	return pte;
}

uint64_t *get_pte_from_pdpe(uint64_t *pdpe, uint64_t va)
{
	uint64_t *pgdir, *pte;
	#ifdef K_DEBUG
	kprintf(" PDPE %p ", pdpe[PDPE(va)]);
	#endif
	if (!(pdpe[PDPE(va)] & PTE_P)) {
		pgdir = get_free_pages(1);
		#ifdef K_DEBUG
		#endif
		pdpe[PDPE(va)] = ((uint64_t)pgdir | PTE_P | PTE_U | PTE_W);
		kprintf(" 2 :PDPE %p ", pdpe[PDPE(va)]);
	} else {
		pgdir = (uint64_t *)(pdpe[PDPE(va)] & ~(0xFFF));
		#ifdef K_DEBUG
		kprintf(" ISSUE PDPE ");
		#endif
	}

	pte = get_pte_from_pgdir(pgdir, va);
	return pte;
}

uint64_t *get_pte_from_pml(uint64_t *pml, uint64_t va)
{
	uint64_t *pdpe, *pte;

	#ifdef K_DEBUG
	kprintf(" PML %x ", PML4(va));
	#endif
	if (!(pml[PML4(va)] & PTE_P)) {
		pdpe = get_free_pages(1);
		#ifdef K_DEBUG
		kprintf(" pdpe %p ", pdpe);
		#endif
		pml[PML4(va)] = ((uint64_t)pdpe | PTE_P | PTE_U | PTE_W);
	} else {
		pdpe = (uint64_t *)(pml[PML4(va)] & ~(0xFFF));
		#ifdef K_DEBUG
		kprintf(" ISSUE PML %p ", pdpe);
		#endif
	}

	pte = get_pte_from_pdpe(pdpe, va);
	return pte;
}

void map_page_entry(uint64_t *pml, uint64_t va, uint64_t size, uint64_t pa, uint64_t perm)
{
	uint64_t *pte_ptr;
	int i;
	for (i = 0; i < size; i += PG_SIZE) {
		pte_ptr = get_pte_from_pml(pml, va + i);
		if (pte_ptr) {
		#ifdef K_DEBUG
			kprintf(" v %p p %p ptr %p\n", va + i, pa + i, pte_ptr);
		#endif
			*pte_ptr = pa + i;
			*pte_ptr |= (PTE_P | PTE_U | PTE_W);
		}
	}
}
static uint64_t *pml;

void page_table_init()
{
	pml = get_free_pages(1);
	kprintf("pml %p ", pml);
	kprintf(" e%p-p%p %p ", phys_end, phys_base, (phys_end - phys_base));
	map_page_entry(pml, VA, (phys_end - phys_base), (uint64_t)phys_base, 0);
	//map_page_entry(pml, VA, 8 * 4096, (uint64_t)phys_base, 0);
	map_page_entry(pml, KERNBASE + 0xB8000, 4 * 4096, (uint64_t)0xB8000, 0);
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
#if 0
	page_dir_t *ptr = free_list_ptr;
	int i = 0;
	for (i = 0; i < 10; i++) {
		kprintf("%x \n", ptr->next);
		ptr = ptr->next;
	}
#endif	
	page_table_init();
	__asm__ volatile("mov %0, %%cr3":: "b"(pml));
	change_console_ptr();
	kprintf("Hello World\n");
}
