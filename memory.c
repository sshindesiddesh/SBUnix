#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/console.h>

struct smap_t {
	uint64_t base, length;
	uint32_t type;
}__attribute__((packed)) *smap;

void memory_init()
{
	while(modulep[0] != 0x9001) modulep += modulep[1]+2;
	for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
		if (smap->type == 1 /* memory */ && smap->length != 0) {
			kprintf("Available Physical Memory [%p-%p] Length %p\n", smap->base, smap->base + smap->length, smap->length);
			if (smap->base == 0) {
				kprintf("Available Pages %d\n", smap->length/PG_SIZE);
			} else {
				kprintf("Available Pages %d\n", (smap->base+smap->length-(uint64_t)physfree)/PG_SIZE);
			}
		}
	}
	kprintf("physfree %p\n", (uint64_t)physfree);
	kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
}
