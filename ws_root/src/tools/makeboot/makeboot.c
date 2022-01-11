/*
 * @(#) makeboot.c 95/09/27 1.1
 *
 * makeboot [ [-a addr] file ]...
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

char buf[1024];

main(int argc, char **argv)
{
	unsigned long z32 = 0;
	unsigned long z8 = 0;
	unsigned long addr;
	unsigned long size;
	unsigned long wrote;
	char *filename;
	FILE *fd;
	int n;
	struct stat st;

	/* Write output file header. */
	fwrite(&z32, sizeof(z32), 1, stdout);
	fwrite(&z32, sizeof(z32), 1, stdout);

	/* Skip command name. */
	argc--;
	argv++;

	/* Process arguments. */
	while (argc > 0)
	{
		/* See if an address is specified by a -a argument. */
		addr = 0;
		if (argv[0][0] == '-')
		{
			switch (argv[0][1])
			{
			case 'a':
				addr = strtol(argv[0]+2, NULL, 0);
				break;
			default:
				fprintf(stderr, "invalid switch %s\n", argv[0]);
				exit(1);
			}
			argc--;
			argv++;
		}
		/* Get filename and try to open it. */
		filename = argv[0];
		argc--;
		argv++;
		if (stat(filename, &st) < 0)
		{
			fprintf(stderr, "Cannot stat %s\n", filename);
			exit(1);
		}
		/* Round up filesize to multiple of 4. */
		size = (st.st_size + sizeof(unsigned long) - 1) /
			sizeof(unsigned long);
		size *= sizeof(unsigned long);
		/* Write the per-component header. */
		fwrite(&addr, sizeof(addr), 1, stdout);
		fwrite(&size, sizeof(size), 1, stdout);
		/* Copy the file to the output. */
		if ((fd = fopen(filename, "r")) == NULL)
		{
			fprintf(stderr, "Cannot open %s\n", filename);
			exit(1);
		}
		wrote = 0;
		while ((n = fread(buf, 1, sizeof(buf), fd)) > 0)
		{
			fwrite(buf, 1, n, stdout);
			wrote += n;
		}
		fclose(fd);
		/* Pad to multiple of 4 bytes. */
		while ((wrote % 4) != 0)
		{
			fwrite(&z8, 1, 1, stdout);
			wrote += 1;
		}
	}
	/* Write a terminator with addr = 0, size = 0 */
	fwrite(&z32, sizeof(z32), 1, stdout);
	fwrite(&z32, sizeof(z32), 1, stdout);
	return 0;
}
