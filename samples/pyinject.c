#include <stdlib.h>
#include <elf.h>
#include <limits.h>
#include <icrt.h>

const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};

typedef int (*FUNC_PyRun_SimpleString)(const char *command);
typedef int (*FUNC_PyGILState_Ensure)();
typedef void (*FUNC_PyGILState_Release)(int);

int main(int ac, char ** av, char ** env) {
    printf("> inject python\n");

    if (ac < 2) {
        printf("> need python command");
        return 0;
    }

    int fd = _open("/proc/self/exe", O_RDONLY, 0);

    if (fd < 0)
        return 0;

    long fileSize = 0;

    if(!(fileSize = _get_file_size(fd))) {
        _close(fd);
        return 0;
    }

    void *buffer = _mmap(NULL, (size_t)fileSize, PROT_READ, MAP_PRIVATE,
                         fd, 0);

    if (!buffer) {
        _close(fd);
        return 0;
    }

    _close(fd);

    Elf64_Ehdr* elf_hdr = (Elf64_Ehdr*)buffer;

    if (memcmp((void *)elf_hdr->e_ident, (void *)expected_magic, sizeof(expected_magic)) != 0) {
        printf("> target is not an ELF executable\n");
        return 0;
    }

    if (elf_hdr->e_ident[EI_CLASS] != ELFCLASS64) {
        printf("> sorry, only ELF-64 is supported\n");
        return 0;
    }

    if (elf_hdr->e_machine != EM_X86_64) {
        printf("> sorry, only x86-64 is supported\n");
        return 0;
    }

    size_t dynStrOffset = 0;
    size_t dynSymSize = 0;
    size_t dynSymOffset = 0;

    for (uint16_t i = 0; i < elf_hdr->e_shnum; i++) {
        size_t offset = elf_hdr->e_shoff + i * elf_hdr->e_shentsize;
        Elf64_Shdr *sHdr = (Elf64_Shdr*)((unsigned char*)buffer + offset);

        switch (sHdr->sh_type) {
            case SHT_SYMTAB:
            case SHT_STRTAB:
                // TODO: have to handle multiple string tables better
                if (!dynStrOffset) {
                    dynStrOffset = sHdr->sh_offset;
                }

                break;

            case SHT_DYNSYM:
                dynSymSize = sHdr->sh_size;
                dynSymOffset = sHdr->sh_offset;
                break;

            default:
                break;
        }
    }

    unsigned long baseAddress = 0;

    if(elf_hdr->e_type == ET_DYN) {
        int maps_fd = _open("/proc/self/maps", O_RDONLY, 0);

        char baseBuffer[PATH_MAX] = {};

        if (_read(maps_fd, baseBuffer, PATH_MAX - 1) <= 0) {
            _close(maps_fd);
            return 0;
        }

        for (int i = 0; i < PATH_MAX; i++) {
            if (baseBuffer[i] == '-') {
                baseBuffer[i] = 0;
                break;
            }
        }

        baseAddress = strtoul(baseBuffer, NULL, 16);

        _close(maps_fd);
    }

    printf("> base address: 0x%lx\n", baseAddress);

    FUNC_PyGILState_Ensure PyGILState_Ensure = NULL;
    FUNC_PyRun_SimpleString PyRun_SimpleString = NULL;
    FUNC_PyGILState_Release PyGILState_Release = NULL;

    for (size_t i = 0; i * sizeof(Elf64_Sym) < dynSymSize; i++) {
        size_t absOffset = dynSymOffset + i * sizeof(Elf64_Sym);
        Elf64_Sym *sym = (Elf64_Sym*)((unsigned char*)buffer + absOffset);

        char *name = (char*)buffer + dynStrOffset + sym->st_name;
        unsigned long offset = sym->st_value;

        if (strcmp(name, "PyGILState_Ensure") == 0) {
            printf("> find symbol %s %lx\n", name, offset);
            PyGILState_Ensure = (FUNC_PyGILState_Ensure)(baseAddress + offset);
        }

        if (strcmp(name, "PyRun_SimpleString") == 0) {
            printf("> find symbol %s %lx\n", name, offset);
            PyRun_SimpleString = (FUNC_PyRun_SimpleString)(baseAddress + offset);
        }

        if (strcmp(name, "PyGILState_Release") == 0){
            printf("> find symbol %s %lx\n", name, offset);
            PyGILState_Release = (FUNC_PyGILState_Release)(baseAddress + offset);
        }
    }

    if (PyGILState_Ensure && PyRun_SimpleString && PyGILState_Release) {
        int state = PyGILState_Ensure();
        PyRun_SimpleString(av[1]);
        PyGILState_Release(state);
    }

    _munmap(buffer, (size_t)fileSize);

    return 0;
}

void _main(unsigned long * sp) {
    int ac = *sp;
    char **av = (char **)(sp + 1);
    char **env = av + ac + 1;

    _exit(main(ac, av, env));
}

void _start(void) {
    CALL_SP(_main);
}