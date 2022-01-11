/* @(#) main.c 95/11/10 1.6 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <string.h>
#include "bench.h"

#define NUMBERBYTES (1024*1024)
void memtest( char  *src, char *dst, int32 len );
void memsettest( char *dst, char c, int32 len );

int32 num[] = { 7,8,30,31,32,64,127,128,256,512,1023,1024,2048,4096,8192,
		16384,32767,32768,127999,128000 };


int main(void)
{
    char *src, *dst;
    int i;

    printf("\n***** MEMORY BANDWIDTH TESTS *****\n\n");
    printf("\n******** TESTING  MEMCPY **********\n");

    src = AllocMem(NUMBERBYTES+4, MEMTYPE_NORMAL);
    dst = AllocMem(NUMBERBYTES+4, MEMTYPE_NORMAL);

    printf("\n*** SRC and DEST ALIGNED ***\n");
    for(i=0;i<20;i++)
	memtest( src, dst, num[i] );

    printf("\n*** UNALIGNED SRC ***\n");
    for(i=0;i<20;i++)
	memtest( src+1, dst, num[i] );

    printf("\n*** UNALIGNED SRC AND DEST ***\n");
    for(i=0;i<20;i++)
	memtest( src+1, dst+3, num[i] );


    printf("\n******** TESTING  MEMSET **********\n");
    for(i=0;i<12;i++)
	memsettest( dst, 0x55, num[i] );
    for(i=0;i<12;i++)
	memsettest( dst+1, 0x55, num[i] );

    printf("\nAll tests completed\n");

    FreeMem(src, NUMBERBYTES+4);
    FreeMem(dst, NUMBERBYTES+4);

    return 0;
}


void memtest( char  *src, char *dst, int32 len )
{
    int32 i, loops;
    float t;

    loops = NUMBERBYTES / len;

    dtime(1);
    for(i=0; i<loops; i++)
        memcpy(dst, src, len);

    t = dtime(0);
    printf("memcpy %d bytes from %lx to %lx  - %6.3f Mbytes per second\n",
           len,src,dst,(float)(loops*len)/t/1000000.0 );
}

void memsettest( char *dst, char c, int32 len )
{
    int32 i, loops;
    float t;

    loops = NUMBERBYTES / len;

    dtime(1);
    for(i=0; i<loops; i++)
        memset(dst, c, len);

    t = dtime(0);
    printf("memset %d bytes at %lx  - %6.3f Mbytes per second\n",
           len,dst,(float)(loops*len)/t/1000000.0 );
}








