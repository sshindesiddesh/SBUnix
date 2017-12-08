#ifndef _MEMORY_H
#define _MEMORY_H
#include <sys/tarfs.h>

#define PG_SIZE 4096
#define KERNBASE        (0xffffffff80000000)
#define MAX_FD_CNT 20

#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PWT         0x008   // Write-Through
#define PTE_PCD         0x010   // Cache-Disable
#define PTE_A           0x020   // Accessed
#define PTE_D           0x040   // Dirty
#define PTE_PS          0x080   // Page Size
#define PTE_MBZ         0x180   // Bits must be zero
#define PTE_COW         0x200   // Copy on write

#define PML4SHIFT               39
#define PDPESHIFT               30
#define PTXSHIFT                12
#define PDXSHIFT                21

#define VA              (KERNBASE + phys_base)
#define PML4(la)        ((((uint64_t) (la)) >> PML4SHIFT) & 0x1FF)
#define PDPE(la)        ((((uint64_t) (la)) >> PDPESHIFT) & 0x1FF)
#define PDX(la)         ((((uint64_t) (la)) >> PDXSHIFT) & 0x1FF)
#define PTX(la)         ((((uint64_t) (la)) >> PTXSHIFT) & 0x1FF)

/* round down to nearest multiple of num */
#define round_down(num, base)                \
({                                           \
    uint64_t __num = (uint64_t) (num);       \
    (__num - __num % (base));  \
})

#define round_up(num, base)                              \
({                                                       \
    uint64_t __base = (uint64_t) (base);                 \
    (round_down((uint64_t) (num) + __base - 1, __base)); \
})

#define KSTACK_SIZE	(1024 * 2)
/* 14 is the number of push/pop in context switch */
#define CON_STACK_SIZE	(14*8)

//extern char kernmem, physbase;
typedef struct page_dir {
        struct page_dir *next;
        int ref_cnt;
	int free;
} page_disc_t;

typedef uint64_t pml_t;
typedef uint64_t pdpe_t;
typedef uint64_t pte_t;
typedef uint64_t va_t;
typedef uint64_t pa_t;
typedef uint64_t pgdir_t;
typedef struct mm_struct_t mm_struct_t;

typedef enum vma_type {
	TEXT = 0,
	DATA,
	STACK,
	HEAP,
	OTHER
} vma_type;

typedef struct PCB {
	uint64_t pid;
	/* offset : 8 Do not move this. Used in assembly with offset */
	uint64_t rsp;
	/* offset : 0x10 Do not move this. Used in assembly with offset */
	pml_t pml4;
	/* offset : 0x18 Do not move this. Used in assembly with offset */
	uint64_t entry;
	/* offset : 0x20 Do not move this. Used in assembly with offset */
	/* User Space stack */
	uint64_t u_rsp;
	/* Kernel Space stack */
	uint8_t kstack[KSTACK_SIZE];
	/* State */
	enum {
		AVAIL = 0,
		READY,
		WAIT,
		SLEEP,
		ZOMBIE,
	} state;
	uint8_t is_usr;
	struct mm_struct_t *mm;
	struct PCB *parent;
	struct PCB *child_head;
	struct PCB *sibling;
	fd_t *fd[MAX_FD_CNT];
	char current_dir[100];
	file_entry_t *current_node;
	int exit_status;
	int wait_pid;
	uint32_t sleep_seconds;
	char proc_name[100];
} pcb_t;

typedef struct mm_struct_t {
        struct vma *head, *tail;
        uint32_t vma_cnt;
	uint64_t brk, data_end, t_vm;
} mm_t;

typedef struct vma {
        struct mm_struct_t *vm_mm;
        uint64_t start;
        uint64_t end;
	vma_type type;
        struct vma *next;
        uint64_t flags;
        uint64_t file;
	uint64_t offset;
} vma_t;

typedef enum ret_t {
	ERROR = -1,
	SUCCESS = 0,
} ret_t;

va_t kmalloc(const uint64_t size);
void map_page_entry(pml_t *pml, va_t va, uint64_t size, pa_t pa, uint64_t perm);
vma_t *check_addr_in_vma_list(va_t addr, vma_t *head);
va_t mmap(va_t va_start, uint64_t size, uint64_t flags, uint64_t type);
void __flush_tlb();
va_t pa2va(pa_t pa);
pa_t va2pa(va_t va);
pa_t page2pa(page_disc_t *ptr);
page_disc_t *pa2page(pa_t pa);
pa_t get_free_pages(uint64_t n);
#endif
