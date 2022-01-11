/*
 * @(#) makeimage.c 96/08/13 1.2
 */

/**
|||	AUTODOC -public -class Dev_Commands -name makeimage
|||	Converts the custom art for a title into M2 boot-image format.
|||
|||	  Synopsis
|||
|||	    makeimage [-b] [-i] [-w width] [-h height] [-o outfile] file
|||
|||	  Description
|||
|||	    This tool takes a custom art file (typically saved from Photoshop
|||	    using a "raw" format, and outputs the artword in M2 "boot-image"
|||	    format.  The output file can then be used as an M2 banner
|||	    screen or device icon.
|||
|||	    A banner screen must be exactly 240 pixels high and 
|||	    640 pixels wide.
|||	    An icon must be less than or equal to 96 pixels high and
|||	    less than or equal to 80 pixels wide.
|||	    All images must be a multiple of 8 pixels wide, and
|||	    each pixel must be 24 bits deep.
|||
|||	  Arguments
|||
|||	    file
|||	        This is the input file to be converted (typically saved from
|||	        Photoshop using a "raw" format.
|||
|||	    -b
|||	        The image is a banner screen.
|||
|||	    -i
|||	        The image is a device icon.
|||
|||	    -w width
|||	        The width of the input image.  Default is 640.
|||
|||	    -h height
|||	        The height of the input image.  Default is 240.
|||
|||	    -o outfile
|||	        The name of the output file.  Default is "Image".
|||
|||	  Return Value
|||
|||	    Prints success or warning if input file size is not what was
|||	    expected.
|||
|||	  Implementation
|||
|||	    Tool
|||
|||	  Associated Files
|||
|||	    None
|||
|||	  See Also
|||
|||	    PhotoShop, Creating M2 Title Document
|||
**/

#include <stdio.h>
#include <dipir/dipirpub.h>

int	height =	240;
int	width =		640;
char *	outfile =	"Image";
char *	infile =	NULL;
bool	banner =	FALSE;
bool	icon =		FALSE;



void usage()
{
	fprintf(stderr, "Usage: makeimage [-b] [-i] [-w width] [-h height] [-o outfile] file\n");
	exit(1);
}

void shortfile(int size)
{
	fprintf(stderr, "Error: file is only %d bytes; should be %d bytes\n",
		size, height * width * 3);
	fprintf(stderr, "       Are you sure that this is a %d X %d Photoshop RAW file (not pict or image)?\n",
		width, height);
	exit(1);
}

void longfile(void)
{
	fprintf(stderr, "Error: file is larger than %d bytes\n",
		height * width * 3);
	fprintf(stderr, "       Are you sure that this is a %d X %d Photoshop RAW file (not pict or image)?\n",
		width, height);
	exit(1);
}

void parse_args(int argc, char **argv)
{
	int ch;
	extern int optind;
	extern char *optarg;

	while ((ch = getopt(argc, argv, "bh:io:w:")) != EOF)  switch (ch)
	{
	case 'b':
		banner = TRUE;
		break;
	case 'h':
		height = atoi(optarg);
		if (height <= 0)
		{
			fprintf(stderr, "Invalid height %d\n", height);
			exit(1);
		}
		break;
	case 'i':
		icon = TRUE;
		break;
	case 'o':
		outfile = optarg;
		break;
	case 'w':
		width = atoi(optarg);
		if (width <= 0 || (width % 8) != 0)
		{
			fprintf(stderr, "Invalid width %d: must be multiple of 8\n", width);
			exit(1);
		}
		break;
	default:
		usage();
	}
	if (optind != argc-1)
		usage();
	infile = argv[optind];
}

void output_header(FILE *out)
{
	unsigned char *p;
	int len;
	VideoImage vi;

	memset(&vi, 0, sizeof(vi));
	vi.vi_Version = 1;
	vi.vi_ImageID = 0;
	vi.vi_Height = height;
	vi.vi_Width = width;
	vi.vi_Depth = 16;
	vi.vi_Size = vi.vi_Height * vi.vi_Width * (vi.vi_Depth / 8);
	vi.vi_Type = VI_DIRECT;
	if (banner)
		memcpy(vi.vi_Pattern, VI_APPBANNER, sizeof(vi.vi_Pattern));
	else if (icon)
		memcpy(vi.vi_Pattern, VI_ICON, sizeof(vi.vi_Pattern));
	else
	{
		fprintf(stderr, "Must have -b or -i\n");
		usage();
	}

	p = (unsigned char *) &vi;
	for (len = 0;  len < sizeof(vi);  len++)
		fputc(*p++, out);
}

void convert_pixels(FILE *in, FILE *out)
{
	int len;
	int red, green, blue, rgb;

	for (len = 0;  len < height * width * 3; )
	{
		/* Read 3 bytes, scrunch them down to 2 bytes. */
		if ((red = getc(in)) == EOF)
			shortfile(len);
		len++;
		if ((green = getc(in)) == EOF)
			shortfile(len);
		len++;
		if ((blue = getc(in)) == EOF)
			shortfile(len);
		len++;
		rgb = (((red >> 3) & 0xFF) << 10) | 
		      (((green >> 3) & 0xFF) << 5) |
		      ((blue >> 3) & 0xFF);
		fputc((rgb>>8), out);
		fputc(rgb, out);
	}

	if (getc(in) != EOF)
		longfile();
}

void output_sig(FILE *out)
{
	int len;

	for (len = 0; len < RSA_KEY_SIZE; len++)
		fputc(0, out);
}

int main(int argc, char **argv)
{
	FILE *in, *out;

	fprintf(stderr, "makeimage (version %s)\n", "1.2");
	parse_args(argc, argv);

	/*
	 * Open input & output files.
	 */
	if ((in = fopen(infile, "r")) == NULL)
	{
		fprintf(stderr, "makeimage: cannot read %s\n", infile);
		exit(1);
	}
	if ((out = fopen(outfile, "w")) == NULL)
	{
		fprintf(stderr, "makeimage: cannot create %s\n", outfile);
		exit(1);
	}

	/*
	 * Output the VideoImage header.
	 */
	output_header(out);

	/*
	 * Output the pixels, converting 24-bit to 16-bit deep.
	 */
	convert_pixels(in, out);

	/*
	 * Output a signature
	 */
	output_sig(out);

	/*
	 * Cleanup and exit.
	 */
	fclose(in);
	fclose(out);
	fprintf(stderr, "makeimage: successfully created %s\n", outfile);
	return 0;
}

