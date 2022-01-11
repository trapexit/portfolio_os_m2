/* @(#) sniff_mem.c 96/03/27 1.4 */
/***************************************************************
**
** Write random patterns to memory and then check them to see
** if anybody is clobbering memory.
**
** By:  Phil Burk
**
** Copyright (c) 1994, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

#define DUR_SECONDS   (10)
#define NUM_BYTES     (1000000)
#define PATTERN(n)    ((n) + ((n)<<16) + 0x12345678)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}


/***********************************************************************/
int main(int argc, char *argv[])
{
	int32   Result, i;
	int32   numBytes, numLongs;
	Item    ioreq;
	int32  *data;
	int32   index = 0;
	uint32  testCount = 0, errorCount = 0;

	printf("Usage: sniff_mem numbytes\n");

	numBytes = (argc > 1) ? atoi(argv[1]) : NUM_BYTES;
	numLongs = numBytes/sizeof(int32);

	ioreq = CreateTimerIOReq();
	CHECKRESULT(ioreq, "CreateTimerIOReq" );

	data = (int32 *) malloc( numBytes );

	while(1)
	{
		int32 testIndex;
/* Fill memory with recognizable pattern. */
		testIndex = index;
		for( i=0; i<numLongs; i++ )
		{
			data[i] = PATTERN(index);
			index++;
		}

/* microsecond timer unit utilities */
 		Result = WaitTime( ioreq, DUR_SECONDS, 0);
		CHECKRESULT(Result, "WaitTime" );

/* Check memory for recognizable pattern. */
		index = testIndex;
		for( i=0; i<numLongs; i++ )
		{
			if( data[i] != PATTERN(index) )
			{
				PRT(("Data mismatch: i = 0x%x, index = 0x%x\n", i, index ));
				PRT(("     address = 0x%x, was = 0x%x, now = 0x%x\n",
					&data[i], PATTERN(index), data[i] ));
				errorCount++;
				PRT(("HANG FOR DEBUG\n")); TOUCH(errorCount); while(1);
			}
			index++;
		}
		PRT(("%s #tests = %d, #errs = %d\n", argv[0], testCount, errorCount ));
		testCount++;
	}

cleanup:
	DeleteTimerIOReq(ioreq);
	return (int) Result;
}
