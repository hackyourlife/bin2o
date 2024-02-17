#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define	TEXT_NAME	".text"
#define	DATA_NAME	".data"
#define	BSS_NAME	".bss"
#define	RODATA_NAME	".rodata.str1.1"
#define	RODATA_PREFIX	".rodata."
#define	DATA_PREFIX	".data.rel.local."
#define	RELA_PREFIX	".rela.data.rel.local."
#define	SYMTAB_NAME	".symtab"
#define	STRTAB_NAME	".strtab"
#define	SHSTRTAB_NAME	".shstrtab"

void* generate_pointer(void* data, size_t size, const char* label, const char* infilename, int is_text, size_t* elf_size)
{
	size_t shstrings_size = strlen(TEXT_NAME) + 1
		+ strlen(DATA_NAME) + 1
		+ strlen(BSS_NAME) + 1
		+ strlen(RODATA_NAME) + 1
		+ strlen(DATA_PREFIX) + strlen(label) + 1
		+ strlen(RELA_PREFIX) + strlen(label) + 1
		+ strlen(SYMTAB_NAME) + 1
		+ strlen(STRTAB_NAME) + 1
		+ strlen(SHSTRTAB_NAME) + 1
		+ 1;
	size_t strings_size = strlen(infilename) + 1
		+ strlen(label) + 1
		+ 1;

	*elf_size = sizeof(Elf64_Ehdr)
		+ 10 * sizeof(Elf64_Shdr)
		+ 4 * sizeof(Elf64_Sym)
		+ sizeof(Elf64_Rela)
		+ sizeof(Elf64_Addr)
		+ strings_size
		+ shstrings_size
		+ size;

	void* elf = malloc(*elf_size);
	if(!elf) {
		perror("malloc");
		return NULL;
	}

	memset(elf, 0, *elf_size);

	Elf64_Ehdr* ehdr = (Elf64_Ehdr*) elf;
	memset(ehdr, 0, sizeof(Elf64_Ehdr));

	ehdr->e_ident[EI_MAG0] = ELFMAG0;
	ehdr->e_ident[EI_MAG1] = ELFMAG1;
	ehdr->e_ident[EI_MAG2] = ELFMAG2;
	ehdr->e_ident[EI_MAG3] = ELFMAG3;
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELFOSABI_SYSV;
	ehdr->e_ident[EI_ABIVERSION] = 0;
	ehdr->e_type = ET_REL;
	ehdr->e_machine = EM_X86_64;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = 0;
	ehdr->e_phoff = 0;
	ehdr->e_shoff = sizeof(Elf64_Ehdr);
	ehdr->e_flags = 0;
	ehdr->e_ehsize = sizeof(Elf64_Ehdr);
	ehdr->e_phentsize = 0;
	ehdr->e_phnum = 0;
	ehdr->e_shentsize = sizeof(Elf64_Shdr);
	ehdr->e_shnum = 10;
	ehdr->e_shstrndx = 9;

#define	STRTAB_ADD(tab, name, s) \
	size_t name = tab##_ptr; \
	strcpy(tab, s); \
	tab += strlen(s) + 1; \
	tab##_ptr += strlen(s) + 1;

#define	STRTAB_ADD2(tab, name, prefix, s) \
	size_t name = tab##_ptr; \
	strcpy(tab, prefix); \
	tab += strlen(prefix); \
	strcpy(tab, s); \
	tab += strlen(s) + 1; \
	tab##_ptr += strlen(prefix) + strlen(s) + 1;

	Elf64_Shdr* shdr = (Elf64_Shdr*) (ehdr + 1);
	memset(shdr, 0, ehdr->e_shnum * ehdr->e_shentsize);

	Elf64_Sym* symtab = (Elf64_Sym*) &shdr[ehdr->e_shnum];
	size_t symtab_offset = (size_t) ((uintptr_t) symtab - (uintptr_t) elf);

	Elf64_Rela* rela = (Elf64_Rela*) &symtab[4];
	size_t rela_offset = (size_t) ((uintptr_t) rela - (uintptr_t) elf);

	Elf64_Addr* datasec = (Elf64_Addr*) &rela[1];
	memset(datasec, 0, sizeof(Elf64_Addr));
	size_t data_offset = (size_t) ((uintptr_t) datasec - (uintptr_t) elf);

	char* strtab = (char*) &datasec[1];
	size_t strtab_offset = (size_t) ((uintptr_t) strtab - (uintptr_t) elf);
	size_t strtab_ptr = 0;
	STRTAB_ADD(strtab, str_null, "");
	STRTAB_ADD(strtab, str_filename, infilename);
	STRTAB_ADD(strtab, str_label, label);

	char* shstrtab = strtab;
	size_t shstrtab_offset = (size_t) ((uintptr_t) shstrtab - (uintptr_t) elf);
	size_t shstrtab_ptr = 0;
	STRTAB_ADD(shstrtab, shstr_null, "");
	STRTAB_ADD(shstrtab, shstr_text, TEXT_NAME);
	STRTAB_ADD(shstrtab, shstr_data, DATA_NAME);
	STRTAB_ADD(shstrtab, shstr_bss, BSS_NAME);
	STRTAB_ADD(shstrtab, shstr_rodata, RODATA_NAME);
	STRTAB_ADD2(shstrtab, shstr_datastr, DATA_PREFIX, label);
	STRTAB_ADD2(shstrtab, shstr_rela, RELA_PREFIX, label);
	STRTAB_ADD(shstrtab, shstr_symtab, SYMTAB_NAME);
	STRTAB_ADD(shstrtab, shstr_strtab, STRTAB_NAME);
	STRTAB_ADD(shstrtab, shstr_shstrtab, SHSTRTAB_NAME);

#undef STRTAB_ADD
#undef STRTAB_ADD2

	size_t rodata_offset = (size_t) ((uintptr_t) shstrtab - (uintptr_t) elf);
	memcpy(shstrtab, data, size);

	shdr->sh_name = shstr_null;

	Elf64_Shdr* sh_text = &shdr[1];
	sh_text->sh_name = shstr_text;
	sh_text->sh_type = SHT_PROGBITS;
	sh_text->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	sh_text->sh_addr = 0;
	sh_text->sh_offset = rodata_offset;
	sh_text->sh_size = 0;
	sh_text->sh_link = 0;
	sh_text->sh_info = 0;
	sh_text->sh_addralign = 1;
	sh_text->sh_entsize = 0;

	Elf64_Shdr* sh_data = &shdr[2];
	sh_data->sh_name = shstr_data;
	sh_data->sh_type = SHT_PROGBITS;
	sh_data->sh_flags = SHF_WRITE | SHF_ALLOC;
	sh_data->sh_addr = 0;
	sh_data->sh_offset = rodata_offset;
	sh_data->sh_size = 0;
	sh_data->sh_link = 0;
	sh_data->sh_info = 0;
	sh_data->sh_addralign = 1;
	sh_data->sh_entsize = 0;

	Elf64_Shdr* sh_bss = &shdr[3];
	sh_bss->sh_name = shstr_bss;
	sh_bss->sh_type = SHT_NOBITS;
	sh_bss->sh_flags = SHF_WRITE | SHF_ALLOC;
	sh_bss->sh_addr = 0;
	sh_bss->sh_offset = rodata_offset;
	sh_bss->sh_size = 0;
	sh_bss->sh_link = 0;
	sh_bss->sh_info = 0;
	sh_bss->sh_addralign = 1;
	sh_bss->sh_entsize = 0;

	Elf64_Shdr* sh_rodata = &shdr[4];
	sh_rodata->sh_name = shstr_rodata;
	sh_rodata->sh_type = SHT_PROGBITS;
	sh_rodata->sh_flags = SHF_ALLOC | SHF_MERGE;
	if(is_text) {
		sh_rodata->sh_flags |= SHF_STRINGS;
	}
	sh_rodata->sh_addr = 0;
	sh_rodata->sh_offset = rodata_offset;
	sh_rodata->sh_size = size;
	sh_rodata->sh_link = 0;
	sh_rodata->sh_info = 0;
	sh_rodata->sh_addralign = sizeof(Elf64_Addr);
	sh_rodata->sh_entsize = 1;

	Elf64_Shdr* sh_datastr = &shdr[5];
	sh_datastr->sh_name = shstr_datastr;
	sh_datastr->sh_type = SHT_PROGBITS;
	sh_datastr->sh_flags = SHF_WRITE | SHF_ALLOC;
	sh_datastr->sh_addr = 0;
	sh_datastr->sh_offset = data_offset;
	sh_datastr->sh_size = 8;
	sh_datastr->sh_link = 0;
	sh_datastr->sh_info = 0;
	sh_datastr->sh_addralign = sizeof(Elf64_Addr);
	sh_datastr->sh_entsize = 0;

	Elf64_Shdr* sh_rela = &shdr[6];
	sh_rela->sh_name = shstr_rela;
	sh_rela->sh_type = SHT_RELA;
	sh_rela->sh_flags = SHF_INFO_LINK;
	sh_rela->sh_addr = 0;
	sh_rela->sh_offset = rela_offset;
	sh_rela->sh_size = sizeof(Elf64_Rela);
	sh_rela->sh_link = 7;
	sh_rela->sh_info = 5;
	sh_rela->sh_addralign = sizeof(Elf64_Addr);
	sh_rela->sh_entsize = sizeof(Elf64_Rela);

	Elf64_Shdr* sh_symtab = &shdr[7];
	sh_symtab->sh_name = shstr_symtab;
	sh_symtab->sh_type = SHT_SYMTAB;
	sh_symtab->sh_flags = 0;
	sh_symtab->sh_addr = 0;
	sh_symtab->sh_offset = symtab_offset;
	sh_symtab->sh_size = 4 * sizeof(Elf64_Sym);
	sh_symtab->sh_link = 8;
	sh_symtab->sh_info = 3;
	sh_symtab->sh_addralign = 1;
	sh_symtab->sh_entsize = sizeof(Elf64_Sym);

	Elf64_Shdr* sh_strtab = &shdr[8];
	sh_strtab->sh_name = shstr_strtab;
	sh_strtab->sh_type = SHT_STRTAB;
	sh_strtab->sh_flags = 0;
	sh_strtab->sh_addr = 0;
	sh_strtab->sh_offset = strtab_offset;
	sh_strtab->sh_size = strings_size;
	sh_strtab->sh_link = 0;
	sh_strtab->sh_info = 0;
	sh_strtab->sh_addralign = 1;
	sh_strtab->sh_entsize = 0;

	Elf64_Shdr* sh_shstrtab = &shdr[9];
	sh_shstrtab->sh_name = shstr_shstrtab;
	sh_shstrtab->sh_type = SHT_STRTAB;
	sh_shstrtab->sh_flags = 0;
	sh_shstrtab->sh_addr = 0;
	sh_shstrtab->sh_offset = shstrtab_offset;
	sh_shstrtab->sh_size = shstrings_size;
	sh_shstrtab->sh_link = 0;
	sh_shstrtab->sh_info = 0;
	sh_shstrtab->sh_addralign = 1;
	sh_shstrtab->sh_entsize = 0;

	symtab[0].st_name = str_null;
	symtab[0].st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
	symtab[0].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[0].st_shndx = SHN_UNDEF;
	symtab[0].st_value = 0;
	symtab[0].st_size = 0;

	symtab[1].st_name = str_filename;
	symtab[1].st_info = ELF64_ST_INFO(STB_LOCAL, STT_FILE);
	symtab[1].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[1].st_shndx = SHN_ABS;
	symtab[1].st_value = 0;
	symtab[1].st_size = 0;

	symtab[2].st_name = 0;
	symtab[2].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
	symtab[2].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[2].st_shndx = 4;
	symtab[2].st_value = 0;
	symtab[2].st_size = 0;

	symtab[3].st_name = str_label;
	symtab[3].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
	symtab[3].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[3].st_shndx = 5;
	symtab[3].st_value = 0;
	symtab[3].st_size = 8;

	rela->r_offset = 0;
	rela->r_info = ELF64_R_INFO(2, R_X86_64_64);
	rela->r_addend = 0;

	return elf;
}

void* generate_direct(void* data, size_t size, const char* label, const char* infilename, int is_text, int add_size, size_t* elf_size)
{
	size_t shstrings_size = strlen(RODATA_PREFIX) + strlen(label) + 1
		+ strlen(SYMTAB_NAME) + 1
		+ strlen(STRTAB_NAME) + 1
		+ strlen(SHSTRTAB_NAME) + 1
		+ 1;
	size_t strings_size = strlen(infilename) + 1
		+ strlen(label) + 1
		+ 1;

	if(add_size) {
		strings_size += strlen(label) + strlen("_size") + 1;
		strings_size += strlen(label) + strlen("_end") + 1;
	}

	*elf_size = sizeof(Elf64_Ehdr)
		+ 5 * sizeof(Elf64_Shdr)
		+ (add_size ? 6 : 4) * sizeof(Elf64_Sym)
		+ strings_size
		+ shstrings_size
		+ size;

	void* elf = malloc(*elf_size);
	if(!elf) {
		perror("malloc");
		return NULL;
	}

	memset(elf, 0, *elf_size);

	Elf64_Ehdr* ehdr = (Elf64_Ehdr*) elf;
	memset(ehdr, 0, sizeof(Elf64_Ehdr));

	ehdr->e_ident[EI_MAG0] = ELFMAG0;
	ehdr->e_ident[EI_MAG1] = ELFMAG1;
	ehdr->e_ident[EI_MAG2] = ELFMAG2;
	ehdr->e_ident[EI_MAG3] = ELFMAG3;
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELFOSABI_SYSV;
	ehdr->e_ident[EI_ABIVERSION] = 0;
	ehdr->e_type = ET_REL;
	ehdr->e_machine = EM_X86_64;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = 0;
	ehdr->e_phoff = 0;
	ehdr->e_shoff = sizeof(Elf64_Ehdr);
	ehdr->e_flags = 0;
	ehdr->e_ehsize = sizeof(Elf64_Ehdr);
	ehdr->e_phentsize = 0;
	ehdr->e_phnum = 0;
	ehdr->e_shentsize = sizeof(Elf64_Shdr);
	ehdr->e_shnum = 5;
	ehdr->e_shstrndx = 4;

#define	STRTAB_ADD(tab, name, s) \
	size_t name = tab##_ptr; \
	strcpy(tab, s); \
	tab += strlen(s) + 1; \
	tab##_ptr += strlen(s) + 1;

#define	STRTAB_ADD2(tab, name, prefix, s) \
	size_t name = tab##_ptr; \
	strcpy(tab, prefix); \
	tab += strlen(prefix); \
	strcpy(tab, s); \
	tab += strlen(s) + 1; \
	tab##_ptr += strlen(prefix) + strlen(s) + 1;

#define	STRTAB_ADD3(tab, name, prefix, s) \
	name = tab##_ptr; \
	strcpy(tab, prefix); \
	tab += strlen(prefix); \
	strcpy(tab, s); \
	tab += strlen(s) + 1; \
	tab##_ptr += strlen(prefix) + strlen(s) + 1;

	Elf64_Shdr* shdr = (Elf64_Shdr*) (ehdr + 1);
	memset(shdr, 0, ehdr->e_shnum * ehdr->e_shentsize);

	Elf64_Sym* symtab = (Elf64_Sym*) &shdr[ehdr->e_shnum];
	size_t symtab_offset = (size_t) ((uintptr_t) symtab - (uintptr_t) elf);

	char* strtab = (char*) &symtab[add_size ? 6 : 4];
	size_t strtab_offset = (size_t) ((uintptr_t) strtab - (uintptr_t) elf);
	size_t strtab_ptr = 0;
	STRTAB_ADD(strtab, str_null, "");
	STRTAB_ADD(strtab, str_filename, infilename);
	STRTAB_ADD(strtab, str_label, label);
	size_t str_label_size = 0;
	size_t str_label_end = 0;
	if(add_size) {
		STRTAB_ADD3(strtab, str_label_size, label, "_size");
		STRTAB_ADD3(strtab, str_label_end, label, "_end");
	}

	char* shstrtab = strtab;
	size_t shstrtab_offset = (size_t) ((uintptr_t) shstrtab - (uintptr_t) elf);
	size_t shstrtab_ptr = 0;
	STRTAB_ADD(shstrtab, shstr_null, "");
	STRTAB_ADD2(shstrtab, shstr_rodata, RODATA_PREFIX, label);
	STRTAB_ADD(shstrtab, shstr_symtab, SYMTAB_NAME);
	STRTAB_ADD(shstrtab, shstr_strtab, STRTAB_NAME);
	STRTAB_ADD(shstrtab, shstr_shstrtab, SHSTRTAB_NAME);

#undef STRTAB_ADD
#undef STRTAB_ADD2
#undef STRTAB_ADD3

	size_t rodata_offset = (size_t) ((uintptr_t) shstrtab - (uintptr_t) elf);
	memcpy(shstrtab, data, size);

	shdr->sh_name = shstr_null;

	Elf64_Shdr* sh_rodata = &shdr[1];
	sh_rodata->sh_name = shstr_rodata;
	sh_rodata->sh_type = SHT_PROGBITS;
	sh_rodata->sh_flags = SHF_ALLOC | SHF_MERGE;
	if(is_text) {
		sh_rodata->sh_flags |= SHF_STRINGS;
	}
	sh_rodata->sh_addr = 0;
	sh_rodata->sh_offset = rodata_offset;
	sh_rodata->sh_size = size;
	sh_rodata->sh_link = 0;
	sh_rodata->sh_info = 0;
	sh_rodata->sh_addralign = sizeof(Elf64_Addr);
	sh_rodata->sh_entsize = 0;

	Elf64_Shdr* sh_symtab = &shdr[2];
	sh_symtab->sh_name = shstr_symtab;
	sh_symtab->sh_type = SHT_SYMTAB;
	sh_symtab->sh_flags = 0;
	sh_symtab->sh_addr = 0;
	sh_symtab->sh_offset = symtab_offset;
	sh_symtab->sh_size = (add_size ? 6 : 4) * sizeof(Elf64_Sym);
	sh_symtab->sh_link = 3;
	sh_symtab->sh_info = 3;
	sh_symtab->sh_addralign = sizeof(Elf64_Addr);
	sh_symtab->sh_entsize = sizeof(Elf64_Sym);

	Elf64_Shdr* sh_strtab = &shdr[3];
	sh_strtab->sh_name = shstr_strtab;
	sh_strtab->sh_type = SHT_STRTAB;
	sh_strtab->sh_flags = 0;
	sh_strtab->sh_addr = 0;
	sh_strtab->sh_offset = strtab_offset;
	sh_strtab->sh_size = strings_size;
	sh_strtab->sh_link = 0;
	sh_strtab->sh_info = 0;
	sh_strtab->sh_addralign = 1;
	sh_strtab->sh_entsize = 0;

	Elf64_Shdr* sh_shstrtab = &shdr[4];
	sh_shstrtab->sh_name = shstr_shstrtab;
	sh_shstrtab->sh_type = SHT_STRTAB;
	sh_shstrtab->sh_flags = 0;
	sh_shstrtab->sh_addr = 0;
	sh_shstrtab->sh_offset = shstrtab_offset;
	sh_shstrtab->sh_size = shstrings_size;
	sh_shstrtab->sh_link = 0;
	sh_shstrtab->sh_info = 0;
	sh_shstrtab->sh_addralign = 1;
	sh_shstrtab->sh_entsize = 0;

	symtab[0].st_name = str_null;
	symtab[0].st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
	symtab[0].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[0].st_shndx = SHN_UNDEF;
	symtab[0].st_value = 0;
	symtab[0].st_size = 0;

	symtab[1].st_name = str_filename;
	symtab[1].st_info = ELF64_ST_INFO(STB_LOCAL, STT_FILE);
	symtab[1].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[1].st_shndx = SHN_ABS;
	symtab[1].st_value = 0;
	symtab[1].st_size = 0;

	symtab[2].st_name = 0;
	symtab[2].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
	symtab[2].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[2].st_shndx = 1;
	symtab[2].st_value = 0;
	symtab[2].st_size = 0;

	symtab[3].st_name = str_label;
	symtab[3].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
	symtab[3].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
	symtab[3].st_shndx = 1;
	symtab[3].st_value = 0;
	symtab[3].st_size = size;

	if(add_size) {
		symtab[4].st_name = str_label_size;
		symtab[4].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
		symtab[4].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
		symtab[4].st_shndx = SHN_ABS;
		symtab[4].st_value = size;
		symtab[4].st_size = 0;

		symtab[5].st_name = str_label_end;
		symtab[5].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
		symtab[5].st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
		symtab[5].st_shndx = 1;
		symtab[5].st_value = size;
		symtab[5].st_size = 0;
	}

	return elf;
}

int main(int argc, char** argv)
{
	int is_text = 0;
	int is_pointer = 0;
	int add_size = 0;
	char* label = NULL;
	const char* infilename = NULL;
	const char* outfilename = NULL;

	for(int i = 1; i < argc; i++) {
		if(*argv[i] == '-') {
			switch(argv[i][1]) {
				case 't':
					if(argv[i][2]) {
						goto unknown_option;
					}
					is_text = 1;
					break;
				case 'b':
					if(argv[i][2]) {
						goto unknown_option;
					}
					is_text = 0;
					break;
				case 'p':
					if(argv[i][2]) {
						goto unknown_option;
					}
					is_pointer = 1;
					break;
				case 's':
					if(argv[i][2]) {
						goto unknown_option;
					}
					add_size = 1;
					break;
				case 'i':
					infilename = &argv[i][2];
					break;
				case 'o':
					outfilename = &argv[i][2];
					break;
				case 'l':
					label = strdup(&argv[i][2]);
					break;
				case 'h':
					printf("Usage: %s [-t|-b] [-s] -l<label> -i<input.bin> -o<output.o>\n", *argv);
					return 0;
				default:
unknown_option:
					printf("Unknown option: %s\n", argv[i]);
					return 1;
			}
		} else {
			printf("Unknown positional argument: %s\n", argv[i]);
			return 1;
		}
	}

	if(!infilename) {
		printf("No input file\n");
		return 1;
	}

	if(!outfilename) {
		printf("No output file\n");
		return 1;
	}

	if(!label) {
		const char* slash = strrchr(infilename, '/');
		if(!slash) {
			slash = infilename;
		}
		label = strdup(slash);
		char* dot = strchr(label, '.');
		if(dot) {
			*dot = 0;
		}
	}

	struct stat statbuf;
	if(stat(infilename, &statbuf) < 0) {
		perror("stat");
		return 1;
	}

	FILE* in = fopen(infilename, "rb");
	if(!in) {
		perror("fopen");
		return 1;
	}

	size_t size = statbuf.st_size;
	if(is_text) {
		size++;
	}

	char* data = (char*) malloc(size);
	if(!data) {
		perror("malloc");
		return 1;
	}

	if(fread(data, statbuf.st_size, 1, in) != 1) {
		perror("fread");
		return 1;
	}

	fclose(in);

	if(is_text) {
		data[statbuf.st_size] = 0;
	}

	void* elf;
	size_t elf_size;

	if(is_pointer) {
		elf = generate_pointer(data, size, label, infilename, is_text, &elf_size);
	} else {
		elf = generate_direct(data, size, label, infilename, is_text, add_size, &elf_size);
	}

	if(!elf) {
		return 1;
	}

	FILE* out = fopen(outfilename, "wb");
	if(!out) {
		perror("fopen");
		return 1;
	}

	if(fwrite(elf, elf_size, 1, out) != 1) {
		perror("fwrite");
		return 1;
	}

	fclose(out);

	free(data);
	free(elf);
	free(label);

	return 0;
}
