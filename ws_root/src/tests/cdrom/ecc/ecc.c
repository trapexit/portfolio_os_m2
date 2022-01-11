/* @(#) ecc.c 96/03/21 1.2 */

/**************************************************************************
* This code has two distinct runtime environments for totally separate    *
* purposes.  It can run under Unix or Portfolio.  See comments below for  *
* more info.                                                              *
**************************************************************************/

#ifndef UNIX_BUILD
/**************************************************************************
* The 'ecc' app that runs under Portfolio does a variety of things:       *
*                                                                         *
* Namely, it compares the following sector data:  raw, corrected (bad),   *
* pure and displays the differences between Pure & Raw, and Pure & Bad.   *
* One line is printed per byte, along with the offset into the sector.    *
*                                                                         *
* Run with no parameters it compares internal sector arrays rawdata and   *
* puredata.                                                               *
*                                                                         *
* If supplied with 2 input file names a RawSector and a PureSector (both  *
* binary images), it will compare those.                                  *
*                                                                         *
**************************************************************************/
#include <kernel/debug.h>
#include <file/fileio.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

#include "eccdata.h"
#include "eccdata2.h"
#include "eccdata3.h"
#define NUM_SECTORS	3

uint8	*rawSectors[] = { rawdata, rawdata2, rawdata3 };
uint8	*pureSectors[] = { puredata, puredata2, puredata3 };

#define CHK4ERR(x,str)  if ((x)<0) { printf(str); PrintfSysErr(x); return (x); }

extern int32 SectorECC(uint8 *buf);

int main(int32 argc,char *argv[])
{
	RawFile *infile1, *infile2;
	int32	cnt, stuff, diffs, x, dpxr, dpxb;
	uint8	buf1[2352], buf2[2352], buf3[2352];
	Err	err;

	uint8	sector = 0;

	switch (argc)
	{
		case 2:
			sector = strtol(argv[1],0,10) - 1;
			if (sector >= NUM_SECTORS)
			{
				printf("Not that many sectors.\n");
				return (-1);
			}
		case 1:
			memcpy(buf1, rawSectors[sector], 2352);
			memcpy(buf2, pureSectors[sector], 2352);
			break;
		case 3:
			err = OpenRawFile(&infile1, argv[1], FILEOPEN_READ);
			CHK4ERR(err, "Problem opening file1.\n");
			cnt = ReadRawFile(infile1, buf1, 2352);
			CHK4ERR(cnt, "Problem reading file1.\n");

			err = OpenRawFile(&infile2, argv[2], FILEOPEN_READ);
			CHK4ERR(err, "Problem opening file2.\n");
			cnt = ReadRawFile(infile2, buf2, 2352);
			CHK4ERR(cnt, "Problem reading file2.\n");
			break;
		default:
			printf("Usage:  ecc <RawSectorFileName> <PureCompareFileName>\n");
			return (0);
	}

	memcpy(buf3, buf1, 2352);
#if 0
	DebugBreakpoint();
#endif
	stuff = SectorECC(buf3);
	printf("SectorECC() = %08x\n", stuff);
	
	printf("       Raw (buf1)\n");
	printf("          Bad (buf3)\n");
	printf("             Pure (buf2)\n");
	printf("                PxR\n");
	printf("                   PxB\n");

	for (dpxr = dpxb = diffs = 0, x = 0; x < 2340; x++)
	{
		uint8 raw = buf1[x];
		uint8 bad = buf3[x];
		uint8 pure = buf2[x];
		uint8 pxr = pure ^ raw;
		uint8 pxb = pure ^ bad;

		if (pxr | pxb)
		{
			printf("%04d:  %02X %02X ", x, raw, bad);
			printf("%02X %02X %02X\n", pure, pxr, pxb);
			diffs++;
		}
		if (pxr) dpxr++;
		if (pxb) dpxb++;
	}
	printf("diffs = %d\n", diffs);
	printf("pxr diffs = %d\n", dpxr);
	printf("pxb diffs = %d\n", dpxb);
#else
/**************************************************************************
* The 'ecc' app that runs under Unix does multiple things:                *
*                                                                         *
* 1 - converts hex text file to binary image. (given two input parms)     *
* 2 - displays the 256 entry Galois table. (given no parameters)          *
*                                                                         *
* The text -> binary converter takes the input file in the form of hex    *
* text (16 elements per line):                                            *
*                                                                         *
* 24 56 1A F2 24 56 1A F2 24 56 1A F2 24 56 1A F2 24 56 1A F2             *
*                                                                         *
* and generates a binary image output file.                               *
**************************************************************************/
#include <stdio.h>
#include <ctype.h>

main(int, char **);

main(argc, argv)
	int argc;
	char *argv[];
{
	/* help */
	if (argc == 2)
	{
		printf("Usage:  ecc <infile> <outfile>\n");
		return (0);
	}

	/* text -> binary */
	if (argc == 3)
	{
		FILE	*infile, *outfile;
		int		a[16],b;

		outfile = fopen(argv[2], "w");

		infile = fopen(argv[1], "r");
		while (!feof(infile))
		{
			fscanf(infile, "%2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X\n",
			&a[0], &a[1], &a[2], &a[3],
			&a[4], &a[5], &a[6], &a[7],
			&a[8], &a[9], &a[10], &a[11],
			&a[12], &a[13], &a[14], &a[15]);

			for (b = 0; b < 16; b++)
			{
				fputc(a[b], outfile);
				printf("%02X ", a[b]);
			}
			printf("\n");
		}

		fflush(outfile);
		fclose(outfile);
		fclose(infile);
	}
	else	/* secret Galois table display routine */
	{
		long gf_log[256], v, i;
	
		for (v = 1, i = 0; i <= 255; i++)
		{
			gf_log[v] = i;
			printf("gf_log[%d] = %3d\n", v, gf_log[v]);
	
			v = v << 1;
			if (v & 0x100)
				v ^= 0x11D;
		}
	
		printf("============================\n");
	
		for (v = 0; v < 255; v++)
		{
			printf("gf_log[%d] = %3d\n", v, gf_log[v]);
		}
	}
#endif
	return (0);
}
