#ifndef _MEMORY_H
#define _MEMORY_H

#define PG_SIZE 4096
#define KERNBASE        (0xffffffff80000000)

#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PWT         0x008   // Write-Through
#define PTE_PCD         0x010   // Cache-Disable
#define PTE_A           0x020   // Accessed
#define PTE_D           0x040   // Dirty
#define PTE_PS          0x080   // Page Size
#define PTE_MBZ         0x180   // Bits must be zero

#define PML4SHIFT               39
#define PDPESHIFT               30
#define PTXSHIFT                12
#define PDXSHIFT                21

#define VA              (KERNBASE + phys_base)
#define PML4(la)        ((((uint64_t) (la)) >> PML4SHIFT) & 0x1FF)
#define PDPE(la)        ((((uint64_t) (la)) >> PDPESHIFT) & 0x1FF)
#define PDX(la)         ((((uint64_t) (la)) >> PDXSHIFT) & 0x1FF)
#define PTX(la)         ((((uint64_t) (la)) >> PTXSHIFT) & 0x1FF)

//extern char kernmem, physbase;
typedef struct page_dir {
        struct page_dir *next;
        int acc;
} page_disc_t;

typedef uint64_t pml_t;
typedef uint64_t pdpe_t;
typedef uint64_t pte_t;
typedef uint64_t va_t;
typedef uint64_t pa_t;
typedef uint64_t pgdir_t;

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

#endif
