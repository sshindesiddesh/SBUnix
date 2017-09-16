#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>

#define INITIAL_STACK_SIZE 4096
uint8_t initial_stack[INITIAL_STACK_SIZE]__attribute__((aligned(16)));
uint32_t* loader_stack;
extern char kernmem, physbase;

void init_idt(void);
void pic_init(void);
void timer_init(void);
void intr_enable(void);
void keyboard_init(void);

void start(uint32_t *modulep, void *physbase, void *physfree)
{
  struct smap_t {
    uint64_t base, length;
    uint32_t type;
  }__attribute__((packed)) *smap;
  while(modulep[0] != 0x9001) modulep += modulep[1]+2;
  for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
    if (smap->type == 1 /* memory */ && smap->length != 0) {
      kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
#if 0
      kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
      kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
      kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
      kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
#endif
    }
  }
#if 0
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
  kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
  kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
  kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
  kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
#endif
#if 0
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("physfree %p\n", (uint64_t)physfree);
#endif
}

void boot(void)
{
  // note: function changes rsp, local stack variables can't be practically used
  register char *temp1, *temp2;

  for(temp2 = (char*)0xb8001; temp2 < (char*)0xb8000+160*25; temp2 += 2) *temp2 = 7 /* white */;
  __asm__(
    "cli;"
    "movq %%rsp, %0;"
    "movq %1, %%rsp;"
    :"=g"(loader_stack)
    :"r"(&initial_stack[INITIAL_STACK_SIZE])
  );
	kprintf("GDT INIT ");
  init_gdt();
	kprintf("IDT INIT ");
  init_idt();
	kprintf("TIMER INIT \n");
  timer_init();
	kprintf("Keyboard Init\n");
  keyboard_init();
	kprintf("PIC INIT\n");
  pic_init();
	kprintf("INT ENABLE \n");

  start(
    (uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
    (uint64_t*)&physbase,
    (uint64_t*)(uint64_t)loader_stack[4]
  );
  while (1);
  for(
    temp1 = "!!!!! start() returned !!!!!", temp2 = (char*)0xb8000;
    *temp1;
    temp1 += 1, temp2 += 2
  ) *temp2 = *temp1;
  while(1);
}
