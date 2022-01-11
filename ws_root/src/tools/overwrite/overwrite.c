/*
 * @(#) overwrite.c 95/07/19 1.1
 * Copyright 1995, The 3DO Company
 *
 * overwrite [-s offset] file
 * Overwrite some bytes in a file.
 * Copies standard input into the named file, starting at the offset
 * specified by -s (default 0).
 */
#include <stdio.h>
#include <strings.h>

void
usage(void)
{
	fprintf(stderr, "usage: overwrite [-s offset] file\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int ch;
	FILE *outf;
	long offset = 0;

	extern int optind;
	extern char *optarg;

	while ((ch = getopt(argc, argv, "s:")) != EOF)  switch (ch)
	{
		case 's':
			offset = strtol(optarg, NULL, 0);
			break;
		default:
			usage();
	}

	if (optind != argc-1)
		usage();

	outf = fopen(argv[optind], "a+");
	if (outf == NULL)
	{
		fprintf(stderr, "Cannot open %s\n", argv[optind]);
		exit(1);
	}

	if (fseek(outf, offset, 0) < 0)
	{
		fprintf(stderr, "Cannot seek to %ld\n", offset);
		exit(1);
	}
	while ((ch = getc(stdin)) != EOF)
		putc(ch, outf);
	fclose(outf);
	return 0;
}
