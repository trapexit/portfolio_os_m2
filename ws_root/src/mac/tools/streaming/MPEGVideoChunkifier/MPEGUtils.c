/******************************************************************************
**
**  @(#) MPEGUtils.c 96/03/29 1.1
**
******************************************************************************/

/****************************************************************************
*																			*
*       File:           MPEGUtils.c											*
*                       Version 1.0											*
*       Date:           02-16-1995											*
*       Written by:     Philippe Cassereau									*
*																			*
****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "MPEGUtils.h"

#define BITSET(x,y)  (x&(1L<<y))
#define SETBIT(x,y) (x = (x| (1<<y)))
#define UNSETBIT(x,y) (x = (x & (0xFF^(1<<y))))

static long bitcount=0;	/* this is total byte count of bytes sent out, cannot be altered (zeroed) */

/****************************************************************************/
/*	char *strstrn(															*/
/*				char *ptr1,		source string								*/
/*				long len1,		length of source string						*/
/*				char *ptr2,		string to match								*/
/*				long len2		length of string to match					*/
/*				)															*/
/*																			*/
/*	Finds a match in source string for match string, if it exists.  Searches*/
/*	up to last position in source that could match second string, allowing	*/
/*	zero character values within given lengths of each string.				*/
/****************************************************************************/

char *strstrn(char *ptr1, long len1, char *ptr2, long len2)
{
	long len, start;
	char *ret=NULL;

	/* avoid any work if second string can't possibly be in first */
	if (len2 <= len1)
	{
		for(start=0,len=0; (start <= (len1-len2)) && (len!=len2); start++)
		{
			ret = ptr1 + start;
			for (len = 0; (len<len2) && (ret[len]==ptr2[len]); len++)
				;
		}
		if (len!=len2)
			ret = NULL;
	}
	return(ret);
}


/********************************************************************/
/*	These are a collection of utilities for sending out bits.  The	*/
/*	idea is to send a word/byte/string and have this software		*/
/*	worry about actually putting the bits out.						*/
/*																	*/
/*	There are also marker routines that allow you to keep count		*/
/*	of how many bits thus far sent, with the ability to zero the	*/
/*	user defined counters.											*/
/********************************************************************/

#undef DEBUG

/***************/
/* Output bits */
/***************/

long outbits(long val, 	/* If whichin is 0 then read this long as input */
		long whichin, 	/* controls input from val or ptr */
		char *ptr, 		/* if whichin is !0 read this string as input */
		long howmany, 	/* how many bits to read from the input */
		long startbit,	/* starting bit to start reading from */
		FILE *outfile)
{

	/* If ptr is NULL then a 32 bit long was passed to this function
	howmany controls how many bits to read from this long, startbit
	determines the place in the long to start.

	whichin controls which input to use the 32bit long or the byte pointer

	For example
		a call with the following params
		(10,NULL,14,15)
		will take the number 10 as a 32 bit long 0x000A and start
		with bit number 15 and output 14 bits so the output is
		'00000000000010'

	If the number to output goes past the rightmost bit (2^0) then
	zero's are padded.  The same is true if you want to start to the
	left of the the left most (2^31).

	The char *ptr variable is used as an alternate way to send
	long 'strings' of things to send out.  If not NULL this
	takes precedence over the long 'val'. */


	long i;
	static char buf;
	static long bitct=0;	/* counter of how many currently in buffer */

	if ( whichin != 1)
		{
		i = 0;
		while (i<howmany)
			{
			while((bitct != 8)&&(i < howmany))
				{
				if (BITSET(val, (startbit-i)))
					SETBIT(buf,7-bitct);
				else
					UNSETBIT(buf,7-bitct);

				bitct++; i++;
				}
			if (bitct == 8)
				{
				/* output the byte */
#ifdef DEBUG
				fprintf(stdout,"%2.2x ",0x00FF & buf);
#else
				fwrite(&buf,1,1,outfile);
#endif
				bitct = 0;
				}
			}

		}
	else
		{
#ifdef DEBUG
		fprintf(stdout,"\n\t\tDATA %d bytes\n",howmany/8);
#else
		fwrite(ptr, (unsigned int) (howmany/8), 1, outfile);
#endif
		}

	/* Update our counters */
	bitcount = bitcount + howmany;	/* update the total bit count */
	
	return(0);
}



/* routines for user counters */
/* The user must supply a place to hold temporary counts, the user
should not and does not need to do anything with this variable */

/* userbitcount is used interally to hold the state of the static
counter when markbits was first called */

/* WARNING these counters are 32 bit counters only, I have NOT coded them
to work around the 32 bit boundaries (this probably should be changed) */

/***********************************************************/
/* Return the current bit count (used to zero the counter) */
/***********************************************************/

long markbits(long *userbitcount ) /*  Read current bit count */
{
	/* Set the counter to current number of bits output */
	*userbitcount = bitcount;
	return(0);
}

/**************************************************/
/* Return the number of bits since a markbit call */
/**************************************************/

long getUserbitcount(long *userbitcount) /* return bit count since markbit call */
{
	/* return the total number of bits sent to output since the
	last call to markbits */

	return (bitcount - *userbitcount);
}

/**********************************************************/
/* Return total number of bits output since the beginning */
/**********************************************************/

long getbitcount(void)
{
	/* Return total number of bits sent to output */
	return bitcount;
}
