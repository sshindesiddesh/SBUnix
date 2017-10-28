#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>
#include <sys/tarfs.h>

#define PG_SIZE	4096

extern char kernmem, physbase;

typedef struct page_dir {
	struct page_dir *next;
	int acc;
} page_dir_t;

static uint64_t phys_free;
static page_dir_t *free_list_ptr = 0;
static page_dir_t *pd_prev = 0;
static uint64_t total_pages = 0;

page_dir_t *get_free_pages(uint64_t n)
{
	if (!free_list_ptr)
		return 0;
	int i = n + 1;
	page_dir_t *ptr = free_list_ptr;
	while (i--) {
		free_list_ptr->acc = 0;
		free_list_ptr = free_list_ptr->next;
	}
	ptr = (page_dir_t *)(((uint64_t)ptr - phys_free)/sizeof(page_dir_t)*PG_SIZE);
	return ptr;
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
				/* TODO Remove this */
				break;
			}
		}
	}
	kprintf("physfree %p\n", (uint64_t)physfree);
	kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
	page_dir_t *ptr = free_list_ptr;
	int i = 0;
	for (i = 0; i < 10; i++) {
		kprintf("%x \n", ptr->next);
		ptr = ptr->next;
	}
	get_free_pages(1);
}
