
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "kernel/types.h"
#include "kernel/nodes.h"
#include "loader/elftypes.h"
#include "loader/elf_ppc.h"
#include "loader/elf.h"
#include "loader/header3do.h"
#include "loader/elf_3do.h"

#define	DEFBUFSIZE	2*1024*1024
#define	MAXLINELEN	2048
#define WHITESPACE	" \t\n\r\f"

#define	NOERRS		0
#define	ERRMISC		1


main(argc, argv) int argc; char **argv; {

	int		filelength,
			BytesTransferred,
			index,
			TextFound,
			DataFound,
			BssFound,
			BinHdrFound;

	char		imagebuffer[DEFBUFSIZE],
			databuffer[MAXLINELEN],
			elffilename[MAXLINELEN],
			datafilename[MAXLINELEN],
			*revision,
			*SecHdrTbl,
			*StrTbl,
			*SecName;

	struct		ROMElfInfo {
		uint32	TextOff;
		uint32	TextSize;
		uint32	TextAddr;
		uint32	DataOff;
		uint32	DataSize;
		uint32	DataAddr;
		uint32	BssOff;
		uint32	BssSize;
		uint32	BssAddr;
		uint8	Version;
		uint8	Revision;
	} ROMElfInfo = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	static char sccsid[] = "@(#) getromelfinfo.c 96/05/03 1.4";

	FILE		*filehandle;

	struct stat	fileinfo;

	Elf32_Ehdr	*ElfHdr;

	Elf32_Shdr	*StrSecHdr,
			*SecHdr;

	_3DOBinHeader	*BinHdr;


	revision = strtok(sccsid, WHITESPACE);
	revision = strtok(NULL, WHITESPACE);
	revision = strtok(NULL, WHITESPACE);
	revision = strtok(NULL, WHITESPACE);

	printf("\n");
	printf("getromelfinfo\n");
	printf("Version 1, revision %s\n", revision);
	printf("By Drew Shell\n");
	printf("The 3DO Company - 1996\n");
	printf("\n");
	if (argc < 3) {
		printf("usage: getromelfinfo elffile datafile\n");
		printf("\n");
		printf("Extracts the following fields from a 3DO ELF file and\n");
		printf("writes them out to a binary data file in the given order\n");
		printf("and with the given field sizes:\n");
		printf("\n");
		printf("  Text offset  (4 bytes)\n");
		printf("  Text size    (4 bytes)\n");
		printf("  Text address (4 bytes)\n");
		printf("  Data offset  (4 bytes)\n");
		printf("  Data size    (4 bytes)\n");
		printf("  Data address (4 bytes)\n");
		printf("  BSS offset   (4 bytes)\n");
		printf("  BSS size     (4 bytes)\n");
		printf("  BSS address  (4 bytes)\n");
		printf("  3DO version  (1 bytes)\n");
		printf("  3DO revision (1 bytes)\n");
		printf("\n");
		exit(NOERRS);
	}

	--argc;
	strcpy(elffilename, *++argv);
	if (strlen(elffilename) >= MAXLINELEN) {
		printf("Filename longer than %i bytes (%s)\n", MAXLINELEN, elffilename);
		exit(ERRMISC);
	}
	if (stat(elffilename, &fileinfo)) {
		printf("Problem getting file info for %s\n", elffilename);
		exit(ERRMISC);
	}
	filelength = fileinfo.st_size;
	if (filelength > DEFBUFSIZE) {
		printf("%s too big for buffer\n", elffilename);
		exit(ERRMISC);
	}
	if (!(filehandle = fopen(elffilename, "rb"))) {
		printf("Problem opening %s\n", elffilename);
		exit(ERRMISC);
	}
	BytesTransferred = fread(imagebuffer, 1, filelength, filehandle);
	if (ferror(filehandle) || BytesTransferred != filelength) {
		printf("Problem reading %s\n", elffilename);
		exit(ERRMISC);
	}
	fclose(filehandle);

	ElfHdr = (Elf32_Ehdr *)imagebuffer;
	SecHdrTbl = imagebuffer + ElfHdr->e_shoff;
	StrSecHdr = (Elf32_Shdr *)( SecHdrTbl + ElfHdr->e_shentsize * ElfHdr->e_shstrndx);
	StrTbl = imagebuffer + StrSecHdr->sh_offset;

	TextFound = 0;
	DataFound = 0;
	BssFound = 0;
	BinHdrFound = 0;
	for (index = 1; index < ElfHdr->e_shnum; index++) {
		SecHdr = (Elf32_Shdr *)( SecHdrTbl + ElfHdr->e_shentsize * index);
		SecName = StrTbl + SecHdr->sh_name;
		if (!strcmp(SecName, ".text")) {
			ROMElfInfo.TextOff = SecHdr->sh_offset;
			ROMElfInfo.TextSize = SecHdr->sh_size;
			ROMElfInfo.TextAddr = SecHdr->sh_addr;
			printf("Text off:  %x\n", ROMElfInfo.TextOff);
			printf("Text size: %x\n", ROMElfInfo.TextSize);
			printf("Text addr: %x\n", ROMElfInfo.TextAddr);
			TextFound = 1;
		}
		if (!strcmp(SecName, ".data")) {
			ROMElfInfo.DataOff = SecHdr->sh_offset;
			ROMElfInfo.DataSize = SecHdr->sh_size;
			ROMElfInfo.DataAddr = SecHdr->sh_addr;
			printf("Data off:  %x\n", ROMElfInfo.DataOff);
			printf("Data size: %x\n", ROMElfInfo.DataSize);
			printf("Data addr: %x\n", ROMElfInfo.DataAddr);
			DataFound = 1;
		}
		if (!strcmp(SecName, ".bss")) {
			ROMElfInfo.BssOff = SecHdr->sh_offset;
			ROMElfInfo.BssSize = SecHdr->sh_size;
			ROMElfInfo.BssAddr = SecHdr->sh_addr;
			printf("BSS off:   %x\n", ROMElfInfo.BssOff);
			printf("BSS size:  %x\n", ROMElfInfo.BssSize);
			printf("BSS addr:  %x\n", ROMElfInfo.BssAddr);
			BssFound = 1;
		}
		if (!strcmp(SecName, ".hdr3do")) {
			BinHdr = (_3DOBinHeader*)( imagebuffer + SecHdr->sh_offset + sizeof(ELF_Note3DO));
			ROMElfInfo.Version = BinHdr->_3DO_Item.n_Version;
			ROMElfInfo.Revision = BinHdr->_3DO_Item.n_Revision;
			printf("Version:   %x\n", ROMElfInfo.Version);
			printf("Revision:  %x\n", ROMElfInfo.Revision);
			BinHdrFound = 1;
		}
	}

	--argc;
	strcpy(datafilename, *++argv);
	if (strlen(datafilename) >= MAXLINELEN) {
		printf("Data filename longer than %i bytes (%s)\n", MAXLINELEN, datafilename);
		exit(ERRMISC);
	}
	if (!(filehandle = fopen(datafilename, "wb"))) {
		printf("Problem opening %s\n", datafilename);
		exit(ERRMISC);
	}
	BytesTransferred = fwrite((char *)&ROMElfInfo, 1, sizeof(ROMElfInfo), filehandle);
	fclose(filehandle);
	if (BytesTransferred < sizeof(ROMElfInfo)) {
		printf("Problem writing to %s\n", datafilename);
		exit(ERRMISC);
	}

	printf("\n");
	exit(NOERRS);
}

