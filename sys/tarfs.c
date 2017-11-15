#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/defs.h>
#include <sys/memory.h>
#include <sys/elf64.h>
#include <sys/process.h>
#include <sys/console.h>
#include <sys/idt.h>
#include <sys/kutils.h>

static tarfs_entry_t tarfs_fs[100];

int get_parent_index(char* dir, int type)
{ 
	char name[100];
	int len = strlen(dir);
	strcpy(&name[0], dir);
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
	while(strcmp(&name[0], &(tarfs_fs[i].name[0])) !=  0)
		i++;
	kprintf("\tparent : %d", i);
	return i;
}	

uint64_t check_file_exists(char* filename)
{
	struct posix_header_ustar* tmp_tarfs = (struct posix_header_ustar *)&_binary_tarfs_start;
	int t = 512;
	uint64_t size;
	while(strlen(tmp_tarfs->name) != 0)
	{
		tmp_tarfs = (struct posix_header_ustar *)(&_binary_tarfs_start + t);
		size = octal_to_decimal(stoi(tmp_tarfs->size));
		if (strlen(tmp_tarfs->name) == 0)
			return 999;
		if (!strcmp(tmp_tarfs->name, filename)) {
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

#if 0
pcb_t * load_elf_code(pcb_t * pcb, Elf64_Ehdr * elf_header, char * filename)
{
	Elf64_Phdr* pgm_header;
	pgm_header = (Elf64_Phdr*) ((void*)elf_header + elf_header->e_phoff);
	for (int i = 0; i < elf_header->e_phnum; ++i) {
                if ((int)pgm_header->p_type == 1) { /* 1 is PT_LOAD, specifies a loadable segment */
			memcpy((char*) pgm_header->p_vaddr, (void *) elf_header + pgm_header->p_offset, pgm_header->p_filesz);
			if (pgm_header->p_filesz < pgm_header->p_memsz) {
			
			}
                }
        }
	return pcb;	
}
#endif

void tarfs_init()
{
	struct posix_header_ustar *tarfs_itr = (struct posix_header_ustar *)&_binary_tarfs_start;
	int t = 0, i = 0; //, j, k;
	tarfs_entry_t *new_entry;
	new_entry = (tarfs_entry_t *)kmalloc(sizeof(tarfs_entry_t));
	uint64_t size;
	while(1) {
		tarfs_itr = (struct posix_header_ustar *)(&_binary_tarfs_start + t);
		if(strlen(tarfs_itr->name) == 0) {
			//kprintf("\tFile name empty");
			break;
		}
		strcpy(&new_entry->name[0], tarfs_itr->name);
		size = 0;
		/* detecting the size of file */
		size = octal_to_decimal(stoi(tarfs_itr->size));
		new_entry->size = size;
		new_entry->addr_header = (uint64_t)&_binary_tarfs_start + t;
		if (strcmp(tarfs_itr->typeflag, "5") == 0)
                        new_entry->type = DIRECTORY;
                else
                        new_entry->type = FILE_TYPE;
		
		new_entry->parent_index = get_parent_index(&(new_entry->name[0]), new_entry->type);
		tarfs_fs[i] = *new_entry;
		i++;
		//if (i == 100)
		//	break;
		if (size == 0)
			t += 512;
		else
			t +=  (size % 512 == 0) ? size + 512 : size + (512 - size % 512) + 512;
	}
	kprintf("\nwhile complete");
	int offset = check_file_exists("bin/sbush");
	if (offset == 0 || offset == 999)
		kprintf("\t File not found");
	else
	{
		kprintf("\t sbush at offset : %d",offset);
		//load binary
		Elf64_Ehdr* elf_header = (Elf64_Ehdr *)(&_binary_tarfs_start + offset);
		if (is_proper_executable(elf_header) != -1) {
			kprintf("Elf proper");
			//pcb_t * user_proc = (pcb_t *)kmalloc(sizeof(pcb_t));
			//load_elf_code(user_proc, elf_header, "bin/sbush");			
		}
	}
}

