#include "stdio.h"
#include "elf.h"
#include "stdlib.h"
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

typedef int _main();
void stop() {
}
int main(int argc, char ** argv) {
    int page_size = sysconf(_SC_PAGE_SIZE);
    printf("page size: 0x%x\n", page_size);
    int i;
    for(i=0; i<argc; i++) {
        printf("> %s\n", argv[i]);
    }
    FILE *fptr;
    int num;
    if ((fptr = fopen(argv[0], "r")) == NULL) {
        printf("Error opening %s", argv[0]);
        exit(1);
    }

    Elf64_Ehdr header;
    fread(&header, sizeof(Elf64_Ehdr), 1, fptr);
    Elf64_Addr start = header.e_entry;
    printf("e_entry: %p\n", start);
    printf("OSABI: 0x%x\n", header.e_ident[EI_OSABI]);
    printf("e_type: 0x%x\n", header.e_type);
    printf("e_machine: 0x%x\n", header.e_machine);
    printf("e_shstrndx: %d\n", header.e_shstrndx);

    Elf64_Shdr str_head;
    fseek(fptr, header.e_shoff + header.e_shstrndx * header.e_shentsize, SEEK_SET);
    fread(&str_head, header.e_shentsize, 1, fptr);

    fseek(fptr, str_head.sh_offset, SEEK_SET);
    char* str_tbl = malloc(str_head.sh_size);
    printf("sh_size: %x\n", str_head.sh_size);
    printf("sh_offset: %x\n", str_head.sh_offset);
    fread(str_tbl, str_head.sh_size, 1, fptr);
    fflush(stdout);
    for (i=0; i<10; i++) {
        printf("%c", str_tbl[i]);
    }
    printf("\n");
    fflush(stdout);

    long symtab_idx;
    long strtab_idx;

    // for(i=0; i<header.e_shnum; i++) {
    //     Elf64_Shdr sec_head;
    //     fseek(fptr, header.e_shoff + i * header.e_shentsize, SEEK_SET);
    //     fread(&sec_head, header.e_shentsize, 1, fptr);
    //     switch (sec_head.sh_type) {
    //         printf("===\n");
    //         case SHT_SYMTAB: {
    //             symtab_idx = sec_head.sh_offset;
    //             // printf("SHT_SYMTAB\n"); 
    //             // Elf64_Sym sym[62];
    //             // fseek(fptr, sec_head.sh_offset, SEEK_SET);
    //             // fread(sym, sizeof(Elf64_Sym), 62, fptr);
    //             // for(i =0; i<62; i++) {
    //             //     printf("st_name (%d): %s\n", i, str_tbl + sym[i].st_name);
    //             // }
    //             break;
    //         }
    //         // case SHT_REL: printf("SHT_REL\n"); break;
    //         // case SHT_RELA: {
    //         //     printf("SHT_RELA\n"); 
    //         //     Elf64_Rela rel;
    //         //     fseek(fptr, sec_head.sh_offset, SEEK_SET);
    //         //     fread(&rel, sizeof(Elf64_Rela), 1, fptr);
    //         //     printf("r_offset: 0x%x\n", rel.r_offset);
    //         //     printf("r_info: 0x%x\n", rel.r_info);
    //         //     printf("r_addend: 0x%x\n", rel.r_addend);
    //         //     break;
    //         // }
    //         case SHT_DYNAMIC: printf("SHT_DYNAMIC\n"); break;
    //     }
    //     printf("sh_name (%d): %d %s\n", i, sec_head.sh_name, str_tbl + sec_head.sh_name);
    // }
    
    Elf64_Addr real_entry;

    for(i=0; i<header.e_phnum; i++) {
        Elf64_Phdr pheader;
        fseek(fptr, header.e_phoff + i * header.e_phentsize, SEEK_SET);
        fread(&pheader, header.e_phentsize, 1, fptr);
        
        switch(pheader.p_type){
            case PT_LOAD:
                printf("---\n");
                printf("PT_LOAD\n");
                printf("p_type: 0x%x\n", pheader.p_type);
                printf("p_offset: 0x%llx\n", pheader.p_offset);
                printf("p_vaddr: 0x%llx\n", pheader.p_vaddr);
                printf("p_align: 0x%llx\n", pheader.p_align);
                printf("p_filesz: 0x%llx\n", pheader.p_filesz);
                printf("p_memsz: 0x%llx\n", pheader.p_memsz);
                fseek(fptr, 0, SEEK_SET);

                void * addr = (void *) ((pheader.p_vaddr / 0x1000) * 0x1000);
                long offset = (pheader.p_offset / 0x1000) * 0x1000;

                printf("*addr   : %p\n", addr);
                printf("offset  : 0x%x\n", offset);
                void * segment = mmap(
                    addr, // void * addr
                    pheader.p_memsz, // size_t length,
                    PROT_EXEC | PROT_READ, // int prot
                    MAP_SHARED, //int flags,
                    fileno(fptr), //int fd, 
                    offset); //off_t offset);
                
                // handle errors
                if ((long) segment == -1) {
                    printf("mmap failed %s\n", strerror(errno));
                    switch(errno) {
                    case EACCES: printf("EACCES\n"); break;
                    case EAGAIN: printf("EAGAIN\n"); break;
                    case EBADF: printf("EBADF\n"); break;
                    case EEXIST: printf("EEXIST\n"); break;
                    case EINVAL: printf("EINVAL\n"); break;
                    case ENFILE: printf("ENFILE\n"); break;
                    case ENOMEM: printf("ENOMEM\n"); break;
                    case EPERM: printf("EPERM\n"); break;
                    }
                    exit(1);
                }

                printf("new page: %p\n", segment);

                if (header.e_entry > pheader.p_vaddr && header.e_entry < pheader.p_vaddr + pheader.p_memsz) {
                    // real_entry = (long) (header.e_entry + (segment - pheader.p_vaddr));
                    // printf("real_entry: %p\n", real_entry);
                    real_entry = segment;
                }

                
        };
    }

    _main* _start = (_main*) (real_entry - 0x400000) + (0x486);
    return _start();
    // return 0x111;
}