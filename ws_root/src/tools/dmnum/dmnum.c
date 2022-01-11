/*
 * dmnum
 * Take a list of numbers from the command line and 
 * write them in binary form to stdout.
 * Size of the number (byte, word, long) may also be specified.
 * This writes out numbers in big endian format.
 *
 * Usage: dmnum [-w <size>] [-o <file>] [-s <seekpos>] <number>...
 *	<size> must be 1, 2 or 4.
 *	<file> is the name of the output file (default stdout).
 *	<seekpos> is the position to seek to before writing.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int size = 4;	/* Default is longwords */
char *outfile = NULL;
long seekto = -1;

	void
usage(void)
{
	fprintf(stderr, "Usage: dmnum [-w <size>] [-o file] [-s pos] <number>...\n");
	exit(1);
}

	int 
main(char argc, char *argv[])
{
	unsigned long num;
	int ch;
	FILE *fd;

	extern char *optarg;
	extern int optind;

	while ((ch = getopt(argc, argv, "o:s:w:")) != EOF)
	{
		switch (ch)
		{
		case 'o':
			outfile = optarg;
			break;
		case 's':
			seekto = strtol(optarg, NULL, 0);
			break;
		case 'w':
			size = strtol(optarg, NULL, 0);
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();

	if (size != 1 && size != 2 && size != 4)
	{
		fprintf(stderr, "Size must be 1, 2 or 4\n");
		exit(1);
	}

	if (outfile == NULL)
		fd = stdout;
	else
	{
		fd = fopen(outfile, "a+");
		if (fd == NULL)
		{
			fprintf(stderr, "dmnum: cannot open %s\n", outfile);
			exit(1);
		}
	}
	if (seekto >= 0)
	{
		if (fd == stdout)
		{
			fprintf(stderr, "dmnum: cannot use -s with stdout\n");
			exit(1);
		}
		fseek(fd, seekto, 0);
	}
	for ( ;  optind < argc;  optind++)
	{
		num = strtol(argv[optind], NULL, 0);
		switch (size)
		{
		case 1:
			{
				unsigned char num1 = (unsigned char) num;
				fwrite(&num1, size, 1, fd);
				break;
			}
		case 2:
			{
				unsigned short num2 = (unsigned short) num;
				fwrite(&num2, size, 1, fd);
				break;
			}
		case 4:
			{
				unsigned long num4 = (unsigned long) num;
				fwrite(&num4, size, 1, fd);
				break;
			}
		}
	}
	return 0;
}
