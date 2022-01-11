/* diskspeed - this program tests the speed of basic disk operations
** using the RawFile functions. No attempt is made to verify their accuracy.
*/

#include <stdio.h>
#include <math.h>
#include <file/filefunctions.h>
#include <kernel/mem.h>
#include <string.h>
#include <stdlib.h>

#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

#define NUM 131072
#define MAXFILESIZE 13200000

int32 BufSize[] = { 256, 1024, 8192, 32768, 131072 };

float dtime( int32 );

void main(void)
{
    RawFile *fp;
    char *buf, name[32];
    int32 i,j,k;
    int32 bufsize, number, err;
    float t;
    
    buf = (char *)AllocMem(NUM,MEMTYPE_NORMAL);

    if( buf==NULL ) {
	printf("ERROR allocating memory.\n");
	exit(1);
    }

    strcpy( name, "tmp00");
    dtime(1);
    if( CreateFile( name ) < 0 ) {
	printf("ERROR creating file %s\n",name);
    }
    t = dtime(0);
    DeleteFile( name );

    number = min((int)(1.0/t+1),10);

    printf("CREATING %d FILES\n",number*10);
    dtime(1);
    for(j=0;j<number;j++) {
	for(k=0;k<10;k++) {
	    if( CreateFile( name ) < 0 ) {
		printf("ERROR creating file %s\n",name);
	    }
	    name[4]++;
	}
	name[3]++;
	name[4]= '0';
    }
    t = dtime(0);
    printf("%f files per second\n",(float)number*10.0/t);


    printf("DELETING %d FILES\n",number*10);
    strcpy( name, "tmp00");
    dtime(1);
    for(j=0;j<number;j++) {
	for(k=0;k<10;k++) {
	    if( DeleteFile( name ) < 0 ) {
		printf("ERROR deleting file %s\n",name);
	    }
	    name[4]++;
	}
	name[3]++;
	name[4]= '0';
    }
    t = dtime(0);
    printf("%f files per second\n",(float)number*10.0/t);
    
    if( OpenRawFile(&fp,"tmpfile",FILEOPEN_READWRITE_NEW) < 0 ) {
	printf("ERROR:  cannot open tmpfile\n");
	exit(1);
    }
    if( SetRawFileSize( fp, MAXFILESIZE ) < 0 ) {
	printf("WARNING:  SetRawFileSize( %d ) failed\n",MAXFILESIZE);
	ClearRawFileError( fp );
    }

    printf("WRITES\n");
    printf("buffer   KB/sec\n");
    for(i=0;i<5;i++) {
	if( SeekRawFile( fp, 0, FILESEEK_START ) < 0 ) 
	    printf("WARNING:  SeekRawFile() failed\n");
	bufsize = BufSize[i];
	dtime(1);
 	for(j=0;j<100;j++) {
	    err = WriteRawFile( fp, buf, bufsize );
	    if( err < 0 ) {
		PrintfSysErr( err );
		printf("\nYour version of Comm3DO or 3DODebug evidently does\n");
		printf("not support writes.  You may want to see if there is\n");
		printf("a newer version available.\n");
		FreeMem( buf, NUM );
		CloseRawFile( fp );
		DeleteFile( "tmpfile" );
		exit(1);
	    }
	}
	t = dtime(0);
	printf("%6d  %7d\n",bufsize,(int)((float)bufsize*100.0/t));
    }

    printf("\nREADS\n");
    printf("buffer   KB/sec\n");
    for(i=0;i<5;i++) {
	if( SeekRawFile( fp, 0, FILESEEK_START ) < 0 ) 
	    printf("WARNING:  SeekRawFile() failed\n");
	bufsize = BufSize[i];
	dtime(1);
	for(j=0;j<100;j++) {
	    err = ReadRawFile( fp, buf, bufsize );
	    if( err < 0 )
		PrintfSysErr( err );
	}
	t = dtime(0);
	printf("%6d  %7d\n",bufsize,(int)((float)bufsize*100.0/t));
    }

    FreeMem( buf, NUM );
    err = CloseRawFile( fp );
    if( err < 0 )
	PrintfSysErr( err );
    if( DeleteFile( "tmpfile" ) < 0 ) {
	printf("ERROR deleting tmpfile\n");
    }
}





