/*
	File:		filter.c

	Contains:	Filtered Image Rescaling 

	Written by:	Anthony Tai (modify the code from Dale Schumacher) 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 5/30/95	TMA		Fixed filtering misalignment and roundoff problems.
		<1+>	 1/16/95	TMA		Added prototypes and error checking.

	To Do:
*/

#include "qGlobal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "GraphicsGems.h"
#include "filter.h"
#include "ReSample.h"

static char	_Program[] = "fzoom";
static char	_Version[] = "0.20";
static char	_Copyright[] = "null";

#ifndef EXIT_SUCCESS
#define	EXIT_SUCCESS	(0)
#define	EXIT_FAILURE	(1)
#endif

typedef	unsigned char	Pixel;

typedef struct 
{
	int	xsize;		/* horizontal size of the image in Pixels */
	int	ysize;		/* vertical size of the image in Pixels */
	Pixel *	data;		/* pointer to first scanline of image */
	int	span;		/* byte offset between two scanlines */
} Image;

#define	WHITE_PIXEL	(255)
#define	BLACK_PIXEL	(0)

/*
 *	generic image access and i/o support routines
 */

static char *
next_token(	FILE *f	)
{
	static char delim[] = " \t\r\n";
	static char *t = NULL;
	static char lnbuf[256];
	char *p;

	while(t == NULL)
	{			/* nothing in the buffer */
		if(fgets(lnbuf, sizeof(lnbuf), f))
		{	/* read a line */
			if((p = strchr(lnbuf, '#'))!=NULL)
			{	/* clip any comment */
				*p = '\0';
			}
			t = strtok(lnbuf, delim);	/* get first token */
		} 
		else 
		{
			return(NULL);
		}
	}
	p = t;
	t = strtok(NULL, delim);			/* get next token */
	return(p);
}

	/* create a blank image */
static Image *new_image(int xsize, int ysize)	
{
	Image *image;

	image = (Image *)malloc(sizeof(Image));
	if (image != NULL)
	{
		image->data = (Pixel *)calloc(ysize*xsize, sizeof(Pixel));
		if((image->data)!=NULL)
		 {
			image->xsize = xsize;
			image->ysize = ysize;
			image->span = xsize;
		}
	}
	return(image);
}

static void free_image(Image *image)
{
	free(image->data);
	free(image);
}

static Image *load_image(FILE *f)		/* read image from file */
{
	char *p;
	int width, height;
	Image *image;

	if((((p = next_token(f))!=NULL) && (strcmp(p, "Bm") == 0))
	&& (((p = next_token(f))!=NULL) && ((width = atoi(p)) > 0))
	&& (((p = next_token(f))!=NULL) && ((height = atoi(p)) > 0))
	&& (((p = next_token(f))!=NULL) && (strcmp(p, "8") == 0))
	&& ((image = new_image(width, height))!=NULL)
	&& (fread(image->data, width, height, f) == height))
	{
		return(image);		/* load successful */
	}
	else
	{
		return(NULL);		/* load failed */
	}
}

/* write image to file */
static int save_image(FILE *f, Image *image)
{
	fprintf(f, "Bm # PXM 8-bit greyscale image\n");
	fprintf(f, "%d %d 8 # width height depth\n",
		image->xsize, image->ysize);
	if(fwrite(image->data, image->xsize, image->ysize, f) == image->ysize) 
	{
		return(0);		/* save successful */
	} 
	else 
	{
		return(-1);		/* save failed */
	}
}

static Pixel get_pixel(Image *image,int x,int y)
{
	static Image *im = NULL;
	static int yy = -1;
	static Pixel *p = NULL;

	if((x < 0) || (x >= image->xsize) || (y < 0) || (y >= image->ysize)) 
	{
		return(0);
	}
	if((im != image) || (yy != y)) 
	{
		im = image;
		yy = y;
		p = image->data + (y * image->span);
	}
	return(p[x]);
}

static void get_row(Pixel *row, Image *image, int y)
{
	if((y < 0) || (y >= image->ysize)) 
	{
		return;
	}
	memcpy(row,image->data + (y * image->span),(sizeof(Pixel) * image->xsize));
}

static void get_column(Pixel *column, Image *image, int x)
{
	int i, d;
	Pixel *p;

	if((x < 0) || (x >= image->xsize)) 
	{
		return;
	}
	d = image->span;
	for(i = image->ysize, p = image->data + x; i-- > 0; p += d) 
	{
		*column++ = *p;
	}
}

static Pixel put_pixel(Image *image, int x,	int y, Pixel data)
{
	static Image *im = NULL;
	static int yy = -1;
	static Pixel *p = NULL;

	if((x < 0) || (x >= image->xsize) || (y < 0) || (y >= image->ysize)) 
	{
		return(0);
	}
	if((im != image) || (yy != y)) 
	{
		im = image;
		yy = y;
		p = image->data + (y * image->span);
	}
	return(p[x] = data);
}


/*
 *	filter function definitions
 */

static double filter_support = 1.0;

static double filter(double t)
{
	/* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
	if(t < 0.0) 
		t = -t;
	if(t < filter_support) 
		return((2.0 * t - 3.0) * t * t + 1.0);
	return(0.0);
}

static double box_support = 0.5;

static double box_filter(double t)
{
	if((t > -box_support) && (t <= box_support)) 
		return(1.0);
	return(0.0);
}

static double triangle_support = 1.0;

static double triangle_filter(double t)
{
	if(t < 0.0) 
		t = -t;
	if(t < triangle_support) 
		return(triangle_support - t);
	return(0.0);
}

static double bell_support = 1.5;

static double
bell_filter(double t)		/* box (*) box (*) box */
{
	if(t < 0) 
		t = -t;
	if(t < .5) 
		return(.75 - (t * t));
	if(t < bell_support) 
	{
		t = (t - bell_support);
		return(.5 * (t * t));
	}
	return(0.0);
}

static double B_spline_support = 2.0;

/* box (*) box (*) box (*) box */
static double B_spline_filter(double t)
{
	double tt;

	if(t < 0) 
		t = -t;
	if(t < (B_spline_support / 2)) 
	{
		tt = t * t;
		return((.5 * tt * t) - tt + (2.0 / 3.0));
	} 
	else if(t < B_spline_support) 
	{
		t = B_spline_support - t;
		return((1.0 / 6.0) * (t * t * t));
	}
	return(0.0);
}

static double sinc(double x)
{
	x *= M_PI;
	if(x != 0) 
		return(sin(x) / x);
	return(1.0);
}

static double Lanczos3_support = 3.0;

static double Lanczos3_filter(double t)
{
	if(t < 0) 
		t = -t;
	if(t < Lanczos3_support) 
		return(sinc(t) * sinc(t/3.0));
	return(0.0);
}

static double Mitchell_support = 2.0;

#define	B	(1.0 / 3.0)
#define	C	(1.0 / 3.0)

static double Mitchell_filter(double t)
{
	double tt;

	tt = t * t;
	if(t < 0) 
		t = -t;
	if(t < (Mitchell_support / 2)) 
	{
		t = (((12.0 - 9.0 * B - 6.0 * C) * (t * tt))
		   + ((-18.0 + 12.0 * B + 6.0 * C) * tt)
		   + (6.0 - 2 * B));
		return(t / 6.0);
	} 
	else if(t < Mitchell_support) 
	{
		t = (((-1.0 * B - 6.0 * C) * (t * tt))
		   + ((6.0 * B + 30.0 * C) * tt)
		   + ((-12.0 * B - 48.0 * C) * t)
		   + (8.0 * B + 24 * C));
		return(t / 6.0);
	}
	return(0.0);
}

double	GetFilterWidth	(long type)
{
	double fwidth;
	switch (type)
	{
		case AVERAGE_SAMPLE:
			fwidth = box_support;
			break;
		case WEIGHT_SAMPLE:
			fwidth = triangle_support;
			break;
		case SINH_SAMPLE:
			fwidth = B_spline_support;
			break;
		case LANCZS3_SAMPLE:
			fwidth = Lanczos3_support;
			break;
		case MICHELL_SAMPLE:
			fwidth = Mitchell_support;
			break;
		default:
			fwidth = filter_support;
			break;
	}
	return(fwidth);
}

double				/* old filter width */
SetFilterWidth
	(
	long type,		/* filter type */
	double fwidth	/* new filter width */
	)
{
	long filterwidth;
	switch (type)
	{
		case AVERAGE_SAMPLE:
			filterwidth = box_support;
			box_support = fwidth;
			break;
		case WEIGHT_SAMPLE:
			filterwidth = triangle_support;
			triangle_support = fwidth;
			break;
		case SINH_SAMPLE:
			filterwidth = B_spline_support;
			B_spline_support = fwidth;
			break;
		case LANCZS3_SAMPLE:
			filterwidth = Lanczos3_support;
			Lanczos3_support = fwidth;
			break;
		case MICHELL_SAMPLE:
			filterwidth = Mitchell_support;
			Mitchell_support = fwidth;
			break;
		default:
			filterwidth = filter_support;
			filter_support = fwidth;
			break;
	}
	return(filterwidth);
}

void ResetFilterWidth(void)
{
	box_support = 0.5;
	triangle_support = 1.0;
	B_spline_support = 2.0;
	Lanczos3_support = 3.0;
	Mitchell_support = 2.0;
	filter_support = 1.0;
}

/*
 *	image rescaling routine
 */

typedef struct {
	int	pixel;
	double	weight;
} CONTRIB;

typedef struct {
	int	n;		/* number of contributors */
	CONTRIB	*p;		/* pointer to list of contributions */
} CLIST;

CLIST	*contrib;		/* array of contribution lists */

static void
zoom
	(
	Image *dst,					/* destination image structure */
	Image *src,					/* source image structure */
	double (*filter)(double),	/* filter function */
	double fwidth				/* filter width (support) */
	)
{
	Image *tmp;						/* intermediate image */
	double xscale, yscale;			/* zoom scale factors */
	int i, j, k;					/* loop variables */
	int n;							/* pixel number */
	double center, left, right;		/* filter calculation variables */
	double width, fscale, weight;	/* filter calculation variables */
	Pixel *raster;					/* a row or column of pixels */
	BYTE *ptrCONTRIB;
	double sum;

	/* create intermediate image to hold horizontal zoom */
	tmp = new_image(dst->xsize, src->ysize);
	xscale = (double) dst->xsize / (double) src->xsize;
	yscale = (double) dst->ysize / (double) src->ysize;

	/* pre-calculate filter contributions for a row */
	contrib = (CLIST *)calloc(dst->xsize, sizeof(CLIST));
	if(xscale < 1.0) 
	{
		width = fwidth / xscale;
		fscale = 1.0 / xscale;
		ptrCONTRIB = (BYTE *)malloc(dst->xsize * (int)(width * 2 + 1) * sizeof(CONTRIB));
		for(i = 0; i < dst->xsize; ++i) 
		{
			contrib[i].n = 0;
			contrib[i].p = (CONTRIB *)(ptrCONTRIB + i * (int)(width * 2 + 1) * sizeof(CONTRIB));
			center = (double) i / xscale + fscale / 2.0 - 0.5;
			left = ceil(center - width);
			right = floor(center + width);
			sum = 0.0;
			for(j = left; j <= right; ++j) 
			{
				weight = center - (double) j;
				weight = (*filter)(weight / fscale) / fscale;
				sum+=weight;
				if(j < 0) 
				{
					n = -j;
				} 
				else if(j >= src->xsize) 
				{
					n = (src->xsize - j) + src->xsize - 1;
				} 
				else 
				{
					n = j;
				}
				k = contrib[i].n++;
				contrib[i].p[k].pixel = n;
				contrib[i].p[k].weight = weight;
			}
			for(k = 0; k < contrib[i].n; k++) 
				contrib[i].p[k].weight = contrib[i].p[k].weight / sum;
		}
	} 
	else 
	{
		ptrCONTRIB = (BYTE *)malloc(dst->xsize * (int)(fwidth * 2 + 1) * sizeof(CONTRIB));

		for(i = 0; i < dst->xsize; ++i) 
		{
			contrib[i].n = 0;
			contrib[i].p = (CONTRIB *) (ptrCONTRIB + i * (int)(fwidth * 2 + 1) * sizeof(CONTRIB));
			center = (double) i / xscale;
			left = ceil(center - fwidth);
			right = floor(center + fwidth);
			for(j = left; j <= right; ++j) 
			{
				weight = center - (double) j;
				weight = (*filter)(weight);
				if(j < 0) 
				{
					n = -j;
				} 
				else if(j >= src->xsize) 
				{
					n = (src->xsize - j) + src->xsize - 1;
				} else {
					n = j;
				}
				k = contrib[i].n++;
				contrib[i].p[k].pixel = n;
				contrib[i].p[k].weight = weight;
			}
		}
	}

	/* apply filter to zoom horizontally from src to tmp */
	raster = (Pixel *)calloc(src->xsize, sizeof(Pixel));
	for(k = 0; k < tmp->ysize; ++k) 
	{
		get_row(raster, src, k);
		for(i = 0; i < tmp->xsize; ++i) 
		{
			weight = 0.0;
			for(j = 0; j < contrib[i].n; ++j) 
			{
				weight += raster[contrib[i].p[j].pixel]
					* contrib[i].p[j].weight;
			}
			weight += 0.5;		/* remove round off error */
			put_pixel(tmp, i, k,
				(Pixel)CLAMP(weight, BLACK_PIXEL, WHITE_PIXEL));
		}
	}
	free(raster);

	/* free the memory allocated for horizontal filter weights */
	free(ptrCONTRIB);
	free(contrib);

	/* pre-calculate filter contributions for a column */
	contrib = (CLIST *)calloc(dst->ysize, sizeof(CLIST));
	if(yscale < 1.0) 
	{
		width = fwidth / yscale;
		fscale = 1.0 / yscale;
		ptrCONTRIB = (BYTE *)malloc(dst->ysize * (int)(width * 2 + 1) * sizeof(CONTRIB));
		for(i = 0; i < dst->ysize; ++i) 
		{
			contrib[i].n = 0;
			contrib[i].p = (CONTRIB *)(ptrCONTRIB + i * (int)(width * 2 + 1) * sizeof(CONTRIB));
			center = (double) i / yscale + fscale / 2.0 - 0.5;
			left = ceil(center - width);
			right = floor(center + width);\
			sum = 0.0;
			for(j = left; j <= right; ++j) 
			{
				weight = center - (double) j;
				weight = (*filter)(weight / fscale) / fscale;
				sum += weight;
				if(j < 0) 
				{
					n = -j;
				} 
				else if(j >= tmp->ysize) 
				{
					n = (tmp->ysize - j) + tmp->ysize - 1;
				} 
				else 
				{
					n = j;
				}
				k = contrib[i].n++;
				contrib[i].p[k].pixel = n;
				contrib[i].p[k].weight = weight;
			}
			for(k = 0; k < contrib[i].n; k++) 
				contrib[i].p[k].weight = contrib[i].p[k].weight / sum;
		}
	} 
	else 
	{
		ptrCONTRIB = (BYTE *)malloc(dst->ysize * (int)(fwidth * 2 + 1) * sizeof(CONTRIB));

		for(i = 0; i < dst->ysize; ++i) 
		{
			contrib[i].n = 0;
			contrib[i].p = (CONTRIB *) (ptrCONTRIB + i * (int)(fwidth * 2 + 1) * sizeof(CONTRIB));
			center = (double) i / yscale;
			left = ceil(center - fwidth);
			right = floor(center + fwidth);
			for(j = left; j <= right; ++j) 
			{
				weight = center - (double) j;
				weight = (*filter)(weight);
				if(j < 0) 
				{
					n = -j;
				} 
				else if(j >= tmp->ysize) 
				{
					n = (tmp->ysize - j) + tmp->ysize - 1;
				} 
				else 
				{
					n = j;
				}
				k = contrib[i].n++;
				contrib[i].p[k].pixel = n;
				contrib[i].p[k].weight = weight;
			}
		}
	}

	/* apply filter to zoom vertically from tmp to dst */
	raster = (Pixel *)calloc(tmp->ysize, sizeof(Pixel));
	for(k = 0; k < dst->xsize; ++k) 
	{
		get_column(raster, tmp, k);
		for(i = 0; i < dst->ysize; ++i) 
		{
			weight = 0.0;
			for(j = 0; j < contrib[i].n; ++j) 
			{
				weight += raster[contrib[i].p[j].pixel]
					* contrib[i].p[j].weight;
			}
			weight += 0.5;		/* remove round off error */
			put_pixel(dst, k, i,
				(Pixel)CLAMP(weight, BLACK_PIXEL, WHITE_PIXEL));
		}
	}
	free(raster);

	/* free the memory allocated for vertical filter weights */
	free(ptrCONTRIB);
	free(contrib);

	free_image(tmp);
}

/*
 *	command line interface
 */

static void
usage()
{
	fprintf(stderr, "usage: %s [-options] input.bm output.bm\n", _Program);
	fprintf(stderr, "\
options:\n\
	-x xsize		output x size\n\
	-y ysize		output y size\n\
	-f filter		filter type\n\
{b=box, t=triangle, q=bell, B=B-spline, h=hermite, l=Lanczos3, m=Mitchell}\n\
");
	exit(1);
}

static void
banner()
{
	printf("%s v%s -- %s\n", _Program, _Version, _Copyright);
}


/****************************************************************************************
 *	preparezoom
 *
 *	prepare for zooming
 ****************************************************************************************/
void preparezoom
	(
	BYTE *srcptr,
	long srcwidth,
	long srcheight,
	BYTE *dstptr,
	long dstwidth,
	long dstheight,
	long filtertype
	)
{
	Image dst, src;
	double (*f)(double) = filter;
	double s = filter_support;

	src.data = (Pixel *)srcptr;
	src.xsize = srcwidth;
	src.ysize = srcheight;
	src.span = srcwidth;

	dst.data = (Pixel *)dstptr;
	dst.xsize = dstwidth;
	dst.ysize = dstheight;
	dst.span = dstwidth;

	switch(filtertype) 
	{
		case BOX_FILTER:
			f=box_filter;
			s=box_support;
			break;
		case TRIANGLE_FILTER:
			f=triangle_filter;
			s=triangle_support;
			break;
		case BELL_FILTER:
			f=bell_filter;
			s=bell_support;
			break;
		case B_SPLINE_FILTER:
			f=B_spline_filter;
			s=B_spline_support;
			break;
		case _FILTER:
			f=filter;
			s=filter_support;
			break;
		case LANCZS3_FILTER:
			f=Lanczos3_filter;
			s=Lanczos3_support;
			break;
		case MICHELL_FILTER:
			f=Mitchell_filter;
			s=Mitchell_support;
			break;
		default:
			f=B_spline_filter;
			s=B_spline_support;
			break;
	}

	zoom(&dst, &src, f, s);
}
