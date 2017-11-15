#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/defs.h>
#include <sys/memory.h>
#include <sys/elf64.h>
#include <sys/process.h>
#include <sys/console.h>
#include <sys/idt.h>

static tarfs_entry_t tarfs_fs[100]; /* tarfs entries can be at max 100 presently */
void *memcpy(void *dest, const void *src, int n);
void *memset(void *ptr, int value, size_t len);
char* strcpy1(char *dst, const char *src)
{
        char *temp = dst;
        if(!src) {
                *dst = '\0';
                return dst;
        }
        while(*src) {
                *temp = *src;
                temp++;
                src++;
        }
        *temp = *src;
        return dst;
}

size_t strlen1(const char *buf)
{
        size_t len = 0;
        while (*buf++ != '\0') len++;
        return len;
}

size_t strcmp1(const char *s1, const char *s2)
{
        if (!s1 || !s2)
                return -1;

        if (strlen1(s1) != strlen1(s2))
                return -1;

        size_t cnt = 0, len = strlen1(s1);

        while (len--)
                if (*s1++ != *s2++)
                        cnt++;
        return cnt;
}

long stoi(const char *s)
{
    long i;
    i = 0;
    while(*s >= '0' && *s <= '9')
    {
        i = i * 10 + (*s - '0');
        s++;
    }
    return i;
}

uint64_t power(uint64_t num, int e)
{
    if (e == 0) return 1;
    return num * power(num, e-1);
}

uint64_t octal_to_decimal(uint64_t octal)
{
    uint64_t dec = 0, i = 0;
    while(octal != 0){
        dec = dec + (octal % 10) * power(8,i++);
        octal = octal/10;
    }
    return dec;
}

int get_parent_index(char* dir, int type)
{ 
	char name[100];
	int len = strlen1(dir);
	strcpy1(&name[0], dir);
	if (type == FILE_TYPE)
		len = len - 2;
	else
		len = len - 1;
	while(name[len] != '/') {
        	len--;
        	if(len == 0)
            		return 9999;
    	}
	name[++len] = '\0';
	int i = 0;
	while(strcmp1(&name[0], &(tarfs_fs[i].name[0])) !=  0)
		i++;
	//kprintf("\tparent : %d", i);
	return i;
}	

uint64_t check_file_exists(char* filename)
{
	struct posix_header_ustar* tmp_tarfs = (struct posix_header_ustar *)&_binary_tarfs_start;
	int t = 512;
	uint64_t size;
	while(strlen1(tmp_tarfs->name) != 0)
	{
		tmp_tarfs = (struct posix_header_ustar *)(&_binary_tarfs_start + t);
		size = octal_to_decimal(stoi(tmp_tarfs->size));
		kprintf("\tfile name:%s size:%d", tmp_tarfs->name, size);
		if (strlen1(tmp_tarfs->name) == 0)
			return 999;
		if (!strcmp1(tmp_tarfs->name, filename)) {
			kprintf("\t file found.!!");
			return t + 512;
		}
		if (size == 0)
                        t += 512;
                else
                        t +=  (size % 512 == 0) ? size + 512 : size + (512 - size % 512) + 512;
	}
	return 0;
}

int is_proper_executable(Elf64_Ehdr* header)
{
	if (header == NULL)
		return -1;
	else {
		if (header->e_ident[1] == 'E' && header->e_ident[2] == 'L' && header->e_ident[3] == 'F')
			return 0;
	}
	return -1;
}

vma_t *malloc_vma(mm_t *mm)
{
        vma_t *vm_tail;
        char  *tmp;
        tmp = (char *)kmalloc(sizeof(vma_t));
        if(mm->vma_list == NULL) {
                tmp = (char *)kmalloc(sizeof(vma_t));
                vm_tail = (vma_t *)tmp;
                mm->vma_list = vm_tail;
        }
        else {
                kprintf("\t not first VMA");
                vm_tail = mm->vma_list;
                while (vm_tail->next != NULL)
                        vm_tail = vm_tail->next;
                tmp = (char *)vm_tail + sizeof(vma_t);
                vm_tail->next = (vma_t *)tmp;
        }
        mm->vma_count += 1;
        return (vma_t *)tmp;
}

pcb_t * load_elf_code(pcb_t * pcb, Elf64_Ehdr * elf_header, char * filename)
{
	Elf64_Phdr* pgm_header;
	mm_t *mms = pcb->mm;
	if (!mms)
		kprintf("\tPCB MM empty");
	vma_t *new_node;
	pgm_header = (Elf64_Phdr *) ((void *)elf_header + elf_header->e_phoff);
	for (int i = 0 ; i < elf_header->e_phnum; ++i) {
		if ((int)pgm_header->p_type == 1) { /* 1 is PT_LOAD, specifies a loadable segment */
			if (pgm_header->p_filesz > pgm_header->p_memsz)
				kprintf("something wrong in Elf binary..");
#if 0
			else
				memset((char*) pgm_header->p_vaddr + pgm_header->p_filesz, 0, pgm_header->p_memsz - pgm_header->p_filesz);
			memcpy((char*) pgm_header->p_vaddr, (void *) elf_header + pgm_header->p_offset, pgm_header->p_filesz);
#endif
			new_node = malloc_vma(mms);
			if (!new_node)
				kprintf("\t VMA not allocated");
			new_node->vma_start = pgm_header->p_vaddr;
			new_node->vma_end = new_node->vma_start + pgm_header->p_memsz;
			new_node->vma_flags = pgm_header->p_flags;
			new_node->vma_file = (uint64_t)elf_header;
			new_node->vma_offset = pgm_header->p_offset;
		}
		pgm_header = pgm_header + 1;
	}
	pcb->entry = elf_header->e_entry;
	pcb->heap_vma = (vma_t *)kmalloc(sizeof(vma_t));
	vma_t *tmp = pcb->mm->vma_list;
	while(tmp->next != NULL)
		tmp = tmp->next;
	pcb->heap_vma->vma_start = pcb->heap_vma->vma_end = ALIGN_DOWN((uint64_t)(tmp->vma_end + 0x1000));
	pcb->heap_vma->vma_size = 0x1000;

	//(pcb, (void *)pcb->heap_vma->vm_start, pcb->heap_vma->vm_mmsz);
	return pcb;	
}

void tarfs_init()
{
	struct posix_header_ustar *tarfs_itr = (struct posix_header_ustar *)&_binary_tarfs_start;
	int t = 0, i = 0; //, j, k;
	tarfs_entry_t *new_entry;
	new_entry = (tarfs_entry_t *)kmalloc(sizeof(tarfs_entry_t));
	uint64_t size;
	while(1) {
		tarfs_itr = (struct posix_header_ustar *)(&_binary_tarfs_start + t);
		if(strlen1(tarfs_itr->name) == 0) {
			//kprintf("\tFile name empty");
			break;
		}
		kprintf("\t %s", tarfs_itr->name);
		strcpy1(&new_entry->name[0], tarfs_itr->name);
		size = 0;
		/* detecting the size of file */
		size = octal_to_decimal(stoi(tarfs_itr->size));
		new_entry->size = size;
		new_entry->addr_header = (uint64_t)&_binary_tarfs_start + t;
		if (strcmp1(tarfs_itr->typeflag, "5") == 0)
                        new_entry->type = DIRECTORY;
                else
                        new_entry->type = FILE_TYPE;
		
		new_entry->parent_index = get_parent_index(&(new_entry->name[0]), new_entry->type);
		tarfs_fs[i] = *new_entry;
		i++;
		if (i == 100)
			break;
		if (size == 0)
			t += 512;
		else
			t +=  (size % 512 == 0) ? size + 512 : size + (512 - size % 512) + 512;
	}
	for (i = 0; i < 20; i++)
		kprintf("\t new netry %s", tarfs_fs[i].name);
	//kprintf("\nwhile complete");
}

pcb_t * create_elf_process(char *filename, char *argv[])
{
	int offset = check_file_exists(filename);
	if (offset == 0 || offset == 999)
		kprintf("\tFile not found");
	else
	{
		kprintf("\tFile found at offset : %d",offset);
		/* load binary */
		Elf64_Ehdr* elf_header = (Elf64_Ehdr *)(&_binary_tarfs_start + offset);
		if (is_proper_executable(elf_header) != -1) {
			kprintf("\tElf proper");
			pcb_t * user_proc = (pcb_t *)kmalloc(sizeof(pcb_t));
			mm_t * new_mm = (mm_t *)kmalloc(sizeof(mm_t));
			user_proc->mm = new_mm;
			//return NULL;
			return load_elf_code(user_proc, elf_header, filename);
		}
	}
	return NULL;
}
