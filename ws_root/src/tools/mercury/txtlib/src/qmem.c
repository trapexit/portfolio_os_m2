/*
	File:		qmem.c

	Contains:	provide quick and basic memory functon. 

	Written by:	Anthony Tai 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <4>	 3/26/95	TMA		Updated comments.
		 <3>	 3/26/95	TMA		Added qMemSkipBits function.
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/

#include "qGlobal.h"

#include <stdio.h>                
#include <stdlib.h>               
#include "qmem.h"

static DWORD mask[33] =
{
	0x00000000,
	0x00000001, 0x00000002,	0x00000004, 0x00000008,
	0x00000010, 0x00000020,	0x00000040, 0x00000080,
	0x00000100, 0x00000200,	0x00000400, 0x00000800,
	0x00001000, 0x00002000,	0x00004000, 0x00008000,
	0x00010000, 0x00020000,	0x00040000, 0x00080000,
	0x00100000, 0x00200000,	0x00400000, 0x00800000,
	0x01000000, 0x02000000,	0x04000000, 0x08000000,
	0x10000000, 0x20000000,	0x40000000, 0x80000000
};

/**********************************************************************************************
 *	qMemNewPtr
 *
 *	the function allocate the memory and  return a pointer
 *
 **********************************************************************************************/
void *qMemNewPtr
	(
	long size
	)
{
	void *ptr;
	ptr = malloc(size);
	if (ptr)
		return(ptr);
	else
	{
		/* set up the memory error message and return the a null */
		return(NULL);
	}
}

/**********************************************************************************************
 *	qMemClearPtr
 *
 *	the function allocate the memory and  return a pointer
 *
 **********************************************************************************************/
void *qMemClearPtr
	(
	long size, long elemSize
	)
{
	void *ptr;
	ptr = calloc(size, elemSize);
	if (ptr)
		return(ptr);
	else
	{
		/* set up the memory error message and return the a null */
		return(NULL);
	}
}

/**********************************************************************************************
 *	qMemReleasePtr
 *
 *	the function release the memory pointed to by a pointer
 *
 **********************************************************************************************/
void qMemReleasePtr
	(
	void *ptr
	)
{
	if (ptr != NULL)
	{
		free(ptr);
		ptr = NULL;
	}
}

/**********************************************************************************************
 *	qMemResizePtr
 *
 *	the function resize the memory pointed to by a pointer
 *
 **********************************************************************************************/
void *qMemResizePtr
	(
	void *ptr,
	long size
	)
{
	void *newptr;

	if (ptr == NULL)
	  newptr = malloc(size);
	else
	  newptr = realloc(ptr, size);

	if (newptr)
		return(newptr);
	else
	{
		/* set up the memory error message and return the a null */
		return(NULL);
	}
}

/**********************************************************************************************
 *	qGetByte
 *
 *	the function is used to retrive a byte from the stream
 *
 **********************************************************************************************/
BYTE qGetByte
	(
	FILE *theFile
	)
{
	register int byte;

	if ((byte = getc(theFile)) != EOF)
		return ((BYTE)byte);
	else {
		fprintf(stderr, "Premature End Of File reading GIF image\n");
		exit(1);
	}
	return (0);
}

/**********************************************************************************************
 *	qGetWord
 *
 *	the function is used to retrive a word from the stream
 *
 **********************************************************************************************/

WORD qGetWord
	(
	FILE *theFile
	)
{
	register int byte, i;
	WORD theWord = 0;
	BYTE *ptr;
	
	ptr = (BYTE *)&theWord;
	for (i = 0; i < 2; i++)
	{
		if ((byte = getc(theFile)) != EOF)
			*ptr = (BYTE)byte;
		else 
		{
			fprintf(stderr, "Premature End Of File reading GIF image\n");
			exit(1);
		}
		ptr++;
	}
	return (theWord);
}


#if 0
WORD qGetWord
(
 FILE *theFile
 )
{
  register int byte = 0, byte1 = 0;
	

  if ((byte = getc(theFile)) != EOF)
    byte1 = byte;
  else 
    {
      fprintf(stderr, "Premature End Of File reading GIF image\n");
      exit(1);
    }
  if ((byte = getc(theFile)) != EOF)
    {
      /*byte = (byte1 << 8) + byte; */
      byte = (byte1) + (byte << 8);
      return((WORD)byte);
    }
  else 
    {
      fprintf(stderr, "Premature End Of File reading GIF image\n");
      exit(1);
    }
  return (0);
}
#endif

/**********************************************************************************************
 *	qGetDWord
 *
 *	the function is used to retrive a dword from the stream
 *
 **********************************************************************************************/
DWORD qGetDWord
	(
	FILE *theFile
	)
{
	register int byte, i;
	DWORD theDWord = 0;
	BYTE *ptr;
	
	ptr = (BYTE *)&theDWord;
	for (i = 0; i < 4; i++)
	{
		if ((byte = getc(theFile)) != EOF)
			*ptr = (BYTE)byte;
		else 
		{
			fprintf(stderr, "Premature End Of File reading GIF image\n");
			exit(1);
		}
		ptr++;
	}
	return (theDWord);
}

/**********************************************************************************************
 *	qGetString
 *
 *	the function is used to retrive a string from the stream
 *
 **********************************************************************************************/
void qGetString
	(
	BYTE *thePtr,
	long theSize, 
	FILE *theFile
	)
{
	register int byte = 0, i;
	

	for (i = 0; i < theSize; i++)
	{
		if ((byte = getc(theFile)) != EOF)
			*thePtr = (BYTE)byte;
		else 
		{
			fprintf(stderr, "Premature End Of File reading GIF image\n");
			exit(1);
		}
		thePtr++;
	}
	return;
}

static long curr_pos = 0;
/**********************************************************************************************
 *	BeginWrite
 *
 *	the function is used to put a byte to the stream
 *
 **********************************************************************************************/
void BeginWrite(void)
{
	curr_pos = 0;	
}

/**********************************************************************************************
 *	GetCurrPos
 *
 *	the function is used to put a byte to the stream
 *
 **********************************************************************************************/
long GetCurrPos(void)
{
	return(curr_pos);	
}

/**********************************************************************************************
 *	qPutByte
 *
 *	the function is used to put a byte to the stream
 *
 **********************************************************************************************/
int qPutByte
	(
	BYTE theByte,
	FILE *theFile
	)
{
	register int byte;
	
	curr_pos++;

	if ((byte = putc((int)theByte, theFile)) != EOF)
		return (byte);
	else {
		fprintf(stderr, "Premature End Of File reading GIF image\n");
		exit(1);
	}
	return (0);
}

/**********************************************************************************************
 *	qPutWord
 *
 *	the function is used to retrive a word from the stream
 *
 **********************************************************************************************/
short qPutWord
	(
	WORD theWord,
	FILE *theFile
	)
{
	BYTE *ptr;
	
	ptr = (BYTE *)&theWord;
	qPutString(ptr, 2, theFile);
	return (0);
}



/**********************************************************************************************
 *	qPutDWord
 *
 *	the function is used to retrive a word from the stream
 *
 **********************************************************************************************/
short qPutDWord
	(
	DWORD theDWord,
	FILE *theFile
	)
{
	BYTE *ptr;
	
	ptr = (BYTE *)&theDWord;
	qPutString(ptr, 4, theFile);
	return (0);
}

/**********************************************************************************************
 *	qPutString
 *
 *	the function is used to put a string to the stream
 *
 **********************************************************************************************/
void qPutString
	(
	BYTE *thePtr,
	long theSize, 
	FILE *theFile
	)
{
	register int byte = 0, i, theByte;
	

	curr_pos+=theSize;

	for (i = 0; i < theSize; i++)
	{
		theByte = (int) *thePtr;
		if ((byte = putc(theByte, theFile)) != EOF)
			thePtr++;
		else 
		{
			fprintf(stderr, "Premature End Of File reading GIF image\n");
			exit(1);
		}
	}
	return;
}


static short	Bits_left;
static BYTE	cur_Byte;
/**********************************************************************************************
 *	qPutBits
 *
 *	the function is used to put a Bits to the stream
 *
 **********************************************************************************************/
void qPutBits
	(
	DWORD theValue,
	BYTE bitnum,
	FILE *theFile
	)
{
	register int byte;

	if (Bits_left)
	{
		while (bitnum)
		{
			if (theValue & mask[bitnum])
			{
				cur_Byte = cur_Byte | mask[Bits_left];
			}
			bitnum--;
			Bits_left--;
			if (Bits_left == 0)
			{
				curr_pos++;
				if ((byte = putc((int)cur_Byte, theFile)) == EOF)
				{
					fprintf(stderr, "Premature End Of File reading GIF image\n");
					exit(1);
				}
				Bits_left = 8;
				cur_Byte = 0;
			}
		}
	}
	
}

/**********************************************************************************************
 *	BeginPutBits
 *
 *	the function begin to put a Bits to the stream
 *
 **********************************************************************************************/
void BeginPutBits(void)
{
	Bits_left = 8;
	cur_Byte = 0;
}

/**********************************************************************************************
 *	EndPutBits
 *
 *	the function end to put a Bits to the stream
 *
 **********************************************************************************************/
void EndPutBits(FILE *theFile)
{
	register int byte;

	if (Bits_left < 8)
	{
		curr_pos++;
		if ((byte = putc((int)cur_Byte, theFile)) == EOF)
		{
			fprintf(stderr, "Premature End Of File reading GIF image\n");
			exit(1);
		}
	}
	Bits_left = 0;
	cur_Byte = 0;
}

static short	Write_Bits_left;
static BYTE	Write_cur_Byte;
static BYTE *Write_Byte_Ptr;
static long Write_Byte_Count;
static long Write_Byte_Fence;

/**********************************************************************************************
 *	qMemPutBits
 *
 *	the function is used to put a Bits to the stream
 *
 **********************************************************************************************/
void qMemPutBits
	(
	DWORD theValue,
	BYTE bitnum
	)
{
	if (Write_Byte_Count == -1)
		return;
	if (Write_Bits_left)
	{
		while (bitnum)
		{
			if (theValue & mask[bitnum])
			{
				Write_cur_Byte = Write_cur_Byte | mask[Write_Bits_left];
			}
			bitnum--;
			Write_Bits_left--;
			if (Write_Bits_left == 0)
			{
				*Write_Byte_Ptr = Write_cur_Byte;
				Write_Byte_Count++;
				Write_Byte_Ptr++;
				Write_Bits_left = 8;
				Write_cur_Byte = 0;
				if (Write_Byte_Fence < Write_Byte_Count)
					Write_Byte_Count = -1;
			}
		}
	}
	
}

/**********************************************************************************************
 *	BeginMemPutBits
 *
 *	the function begin to put a Bits to the stream
 *
 **********************************************************************************************/
void BeginMemPutBits
	(
	BYTE *thePtr,
	long bytecount
	)
{
	Write_Byte_Ptr = thePtr;
	Write_Bits_left = 8;
	Write_cur_Byte = 0;
	Write_Byte_Count = 0;
	Write_Byte_Fence = bytecount;
}

/**********************************************************************************************
 *	EndMemPutBits
 *
 *	the function end to put a Bits to the stream
 *
 **********************************************************************************************/
void EndMemPutBits(void)
{
	if (Write_Bits_left < 8)		/* pad the remaining bits with zero */
		*Write_Byte_Ptr = Write_cur_Byte;
	Write_Bits_left = 0;
	Write_Byte_Ptr = NULL;
}

/**********************************************************************************************
 *	qMemPutString
 *
 *	the function is used to put Bytes to the memory stream
 *
 **********************************************************************************************/
void qMemPutString
	(
	BYTE *ptr,
	long bytecount
	)
{
	long i;
	
	if (Write_Byte_Count < 0)
		return;
	for (i = 0; i < bytecount; i++)
	{
		*Write_Byte_Ptr = *ptr;
		Write_Byte_Ptr++;
		ptr++;
		Write_Byte_Count++;
		if (Write_Byte_Fence < Write_Byte_Count)
			Write_Byte_Count = -1;
	}
}

/**********************************************************************************************
 *	qMemPutBytes
 *
 *	the function is used to put Bytes to the memory stream
 *
 **********************************************************************************************/
void qMemPutBytes
	(
	BYTE *ptr,
	long bytecount
	)
{
	long i;
	
	if (Write_Byte_Count < 0)
		return;
	for (i = 0; i < bytecount; i++)
	{
		*Write_Byte_Ptr = *ptr;
		Write_Byte_Ptr++;
		ptr++;
		Write_Byte_Count++;
		if (Write_Byte_Fence < Write_Byte_Count)
			Write_Byte_Count = -1;
	}
}

/**********************************************************************************************
 *	BeginMemPutBytes
 *
 *	the function begin to put Bytes to the memory stream
 *
 **********************************************************************************************/
void BeginMemPutBytes
	(
	BYTE *thePtr,
	long bytecount
	)
{
	Write_Byte_Ptr = thePtr;
	Write_Byte_Count = 0;
	Write_Byte_Fence = bytecount;
}

/**********************************************************************************************
 *	EndMemPutBytes
 *
 *	the function end to put Bytes to the memory stream
 *
 **********************************************************************************************/
void EndMemPutBytes(void)
{
	Write_Byte_Ptr = NULL;
	Write_Byte_Count = -1;
}

/**********************************************************************************************
 *	GetCurrMemPutPos
 *
 *	the function is used to get the memory position as we put bytes to the stream
 *
 **********************************************************************************************/
long GetCurrMemPutPos(void)
{
	return(Write_Byte_Count);	
}
long GetCurrMemPutBitPos(void)
{
	return(8-Write_Bits_left);	
}


static short	Read_Bits_left;
static BYTE	Read_cur_Byte;
static BYTE *Read_Byte_Ptr;
/**********************************************************************************************
 *	qMemGetBits
 *
 *	the function is used to get Bits from the memory stream
 *
 **********************************************************************************************/
DWORD qMemGetBits
	(
	BYTE bitnum
	)
{
	DWORD theDWord = 0;
	
	if (Read_Bits_left)
	{
		while (bitnum)
		{
			if (Read_cur_Byte & mask[Read_Bits_left])
				theDWord = theDWord | mask[bitnum];
			Read_Bits_left--;
			bitnum--;
			if (Read_Bits_left == 0)
			{
				Read_Bits_left = 8;
				Read_Byte_Ptr++;
				Read_cur_Byte = *Read_Byte_Ptr;
			}
		}
	}
	return(theDWord);
}

/**********************************************************************************************
 *	BeginMemGetBits
 *
 *	the function begin to get Bits from the memory stream
 *
 **********************************************************************************************/
void BeginMemGetBits
	(
	BYTE *theBuff
	)
{
	Read_Byte_Ptr = theBuff;
	Read_Bits_left = 8;
	Read_cur_Byte = *Read_Byte_Ptr;
}

/**********************************************************************************************
 *	EndMemGetBits
 *
 *	the function end to get Bits the memory stream
 *
 **********************************************************************************************/
void EndMemGetBits(void)
{
	Read_Byte_Ptr = NULL;
}

/**********************************************************************************************
 *	qMemGetBytes
 *
 *	the function is used to get Bytes from the memory stream
 *
 **********************************************************************************************/
void qMemGetBytes
	(
	BYTE *ptr, 
	long bytecount
	)
{
	long i;
	
	for (i = 0; i < bytecount; i++)
	{
		*ptr = *Read_Byte_Ptr;
		ptr++;
		Read_Byte_Ptr++;
	}
}

/**********************************************************************************************
 *	BeginMemGetBytes
 *
 *	the function begin to get Bytes from the memory stream
 *
 **********************************************************************************************/
void BeginMemGetBytes
	(
	BYTE *theBuff
	)
{
	Read_Byte_Ptr = theBuff;
}

/**********************************************************************************************
 *	EndMemGetBytes
 *
 *	the function end to get Bytes the memory stream
 *
 **********************************************************************************************/
void EndMemGetBytes(void)
{
	Read_Byte_Ptr = NULL;
}

void qMemSkipBits
	(
	long bitnum
	)
{	
	if (Read_Bits_left)
	{
		Read_Byte_Ptr += bitnum;
		Read_cur_Byte = *Read_Byte_Ptr;
		bitnum = bitnum % 8;
		Read_Bits_left -= bitnum;
		if (!Read_Bits_left)
			Read_Bits_left = 8;
	}
}
