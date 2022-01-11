#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <kernel/mem.h>

#define NUM 10000

#define MAX(a,b)	( ((a) > (b)) ? (a) : (b) )

float dtime( int32 );

int main( void );

int main()
{
    float a,b,c,d,*f, *g, *h, *e;
    int i, j, errors, *x, *y;
    RawFile *fp;
    int bad=0;
    float t, max_error, epsilon;

    f = (float *)AllocMem(NUM*sizeof(float),MEMTYPE_NORMAL);
    g = (float *)AllocMem(NUM*sizeof(float),MEMTYPE_NORMAL);
    h = (float *)AllocMem(NUM*sizeof(float),MEMTYPE_NORMAL);
    e = (float *)AllocMem(NUM*sizeof(float),MEMTYPE_NORMAL);
    x = (int *)AllocMem(NUM*sizeof(int),MEMTYPE_NORMAL);
    y = (int *)AllocMem(NUM*sizeof(int),MEMTYPE_NORMAL);

    if( x==NULL || y==NULL || f==NULL || g==NULL || e==NULL ) {
	printf("ERROR allocating memory.\n");
	exit(1);
    }

    /* now read in reference arrays */    
    if( OpenRawFile(&fp,"floatspeed.dat",FILEOPEN_READ) < 0 ) {
	printf("ERROR:  cannot open floatspeed.dat.\n");
	exit(1);
    }

    if( ReadRawFile( fp, g, sizeof(float)*NUM ) != sizeof(float)*NUM ) {
	printf("ERROR reading floatspeed.dat\n");
	exit(1);
    }

    if( ReadRawFile( fp, y, sizeof(int)*NUM ) != sizeof(int)*NUM ) {
	printf("ERROR reading floatspeed.dat\n");
	exit(1);
    }	
    if( ReadRawFile( fp, h, sizeof(float)*NUM ) != sizeof(float)*NUM ) {
	printf("ERROR reading floatspeed.dat\n");
	exit(1);
    }
    if( ReadRawFile( fp, e, sizeof(float)*NUM ) != sizeof(float)*NUM ) {
	printf("ERROR reading floatspeed.dat\n");
	exit(1);
    }

    printf("\n\nThis program measures the time required to perform\n");
    printf("a function on every member of an array of %d numbers.\n",NUM);
    printf("The results are stored in another array and the results\n");
    printf("are verified against the results in the file floatspeed.dat\n\n");

    /* TEST 0 - COPY */
    printf("TEST 0:  copy\n");
    dtime(1);
    for(j=0;j<100;j++)
	for(i=0;i<NUM;i++)
	    f[i] = g[i];    
    t = dtime(0);

    errors = 0;
    for(i=0;i<NUM;i++)
	if( f[i] != g[i] )
	    errors++;
    
    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    if( errors ) {
	printf("%d numbers were copied incorrectly!!!!!\n",errors);
	bad++;
    }

    /* TEST 1 - FLOAT to INTEGER */
    printf("TEST 1:  ftoi\n");
    dtime(1);
    for(j=0;j<100;j++) 
	for(i=0;i<NUM;i++)
	    x[i] = (int)g[i];    
    t = dtime(0);

    errors = 0;
    for(i=0;i<NUM;i++)
	if( x[i] != y[i] )
	    errors++;
    
    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    if( errors ) {
	printf("%d numbers were incorrectly converted.\n",errors);
	bad++;
    }

    /* TEST 2 - INTEGER to FLOAT */
    printf("TEST 2:  itof\n");
    dtime(1);
    for(j=0;j<100;j++) 
	for(i=0;i<NUM;i++)
	    f[i] = (float)y[i];
    t = dtime(0);

    errors = 0;
    for(i=0;i<NUM;i++)
	if( f[i] != h[i] ) {
	    errors++;
	    /* printf("%d %f %f %f\n",i,f[i],h[i],f[i]-h[i]);  */
	}

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    if( errors ) {
	printf("%d numbers were incorrectly converted.\n",errors);
	bad++;
    }

    /* TEST 3 - FLOPS and ACCURACY */
    printf("TEST 3:  FLOPS\n");
    dtime(1);
    a = 3.141597;
    b = 1.7839032e4;
    c = 0.0;
    for(d=0.1;d<1000000.0;d+=1.0) {
	c += a*b/d;
	b += .3;
    }
    t = dtime(0);
    if( (int)c != 2291327 )
        printf("ERROR: c = %f (should be 2291327.0)\n",c);
    printf("                 MFLOPS = %f\n",6/t);


    /* TEST 4 - FABS TEST */
    printf("TEST 4:  FABS\n");
    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++)
            h[i] = fabsf(g[i]);
    t = dtime(0);

    errors = 0;
    for(i=0;i<NUM;i++)
        if( (h[i] != g[i]) && (h[i] != -g[i]) )
            errors++;

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    if( errors ) {
        printf("%d numbers were incorrectly converted.\n",errors);
        bad++;
    }

    /* TEST 5 - SQRT TEST */

    printf("TEST 5A:  SQRTFFF\n");

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    f[i] = sqrtfff( h[i] );
    t = dtime(0);

    max_error = 0.0;
    for(i=0;i<NUM;i++) {
	if( e[i] )
	    max_error = MAX( fabsf(f[i]-e[i])/e[i]*100.0, max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    printf("                 maximum error: %f%%\n",max_error);

    printf("TEST 5B:  SQRTFF\n");

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    f[i] = sqrtff( h[i] );
    t = dtime(0);

    max_error = 0.0;
    for(i=0;i<NUM;i++) {
	if( e[i] )
	    max_error = MAX( fabsf(f[i]-e[i])/e[i]*100.0, max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    printf("                 maximum error: %f%%\n",max_error);

    printf("TEST 5C:  SQRTF\n");

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    f[i] = sqrtf( h[i] );
    t = dtime(0);

    max_error = 0.0;
    for(i=0;i<NUM;i++)
	if( e[i] )
	    max_error = MAX( fabsf(f[i]-e[i])/e[i]*100.0, max_error );

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    printf("                 maximum error: %f%%\n",max_error);


    /* TEST 6 - RSQRT TEST */

    printf("TEST 6A:  RSQRTFFF\n");

    /* strip out 0.0 from input array */
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    if( h[i] == 0.0 )
		h[i] = 1.0;

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    f[i] = rsqrtfff( h[i] );

    t = dtime(0);
    max_error = 0.0;
    for(i=0;i<NUM;i++) {
	if( e[i] == 0.0 )
	    a = 1.0;
	else
	    a = 1.0 / e[i];
	if( a )
	    max_error = MAX( 100.0 * fabsf(a-f[i])/a, max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    printf("                 maximum error: %f%%\n",max_error);
    printf("TEST 6B:  RSQRTFF\n");

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    f[i] = rsqrtff( h[i] );

    t = dtime(0);
    max_error = 0.0;
    for(i=0;i<NUM;i++) {
	if( e[i] == 0.0 )
	    a = 1.0;
	else
	    a = 1.0 / e[i];
	if( a )
	    max_error = MAX( 100.0 * fabsf(a-f[i])/a, max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    printf("                 maximum error: %f%%\n",max_error);

    printf("TEST 6C:  RSQRTF\n");
    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++) 
	    f[i] = rsqrtf( h[i] );

    t = dtime(0);
    max_error = 0.0;
    for(i=0;i<NUM;i++) {
	if( e[i] == 0.0 )
	    a = 1.0;
	else
	    a = 1.0 / e[i];
	if( a )
	    max_error = MAX( 100.0 * fabsf(a-f[i])/a, max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    printf("                 maximum error: %f%%\n",max_error);

    /* TEST 7 - SIN TEST */
    printf("TEST 7:  SIN\n");
    if( ReadRawFile( fp, e, sizeof(float)*NUM ) != sizeof(float)*NUM ) {
        printf("ERROR reading floatspeed.dat\n");
        exit(1);
    }

    for(i=0;i<NUM;i++)
        h[i] = 8.0 * M_PI * (((float)i/(float)NUM)-0.5);

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++)
            f[i] = sinf( h[i] );
    t = dtime(0);

    errors = 0;
    max_error = 0.0;
    for(i=0;i<NUM;i++) {
        epsilon = .00001;  /* error threshold */
        if( (f[i] < (e[i]-epsilon)) || (f[i] > (e[i]+epsilon)) ) 
            errors++;
	if( e[i] )
	    max_error = MAX( fabsf(e[i]-f[i]), max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    if( errors ) {
        printf("%d numbers were incorrectly converted.\n",errors);
        bad++;
    }
    printf("                 maximum error: %f\n",max_error);

    /* TEST 8 - COS TEST */
    printf("TEST 8:  COS\n");
    if( ReadRawFile( fp, e, sizeof(float)*NUM ) != sizeof(float)*NUM ) {
        printf("ERROR reading floatspeed.dat\n");
        exit(1);
    }

    for(i=0;i<NUM;i++)
        h[i] = 8.0 * M_PI * (((float)i/(float)NUM)-0.5);

    dtime(1);
    for(j=0;j<100;j++)
        for(i=0;i<NUM;i++)
            f[i] = cosf( h[i] );
    t = dtime(0);

    errors = 0;
    max_error = 0.0;
    epsilon = .00001;  /* error threshold */

    for(i=0;i<NUM;i++) {
        if( (f[i] < (e[i]-epsilon)) || (f[i] > (e[i]+epsilon)) ) 
            errors++;
	if( e[i] != 0.0 ) 
	    max_error = MAX( fabsf(e[i]-f[i]) , max_error );
    }

    printf("                 %8.1f per second\n",(float)NUM*100.0/t);
    if( errors ) {
        printf("%d numbers were incorrectly converted.\n",errors);
        bad++;
    }
    printf("                 maximum error: %f\n",max_error);

    CloseRawFile( fp );

    if( bad )
	printf("\nFAILED %d TESTS\n",bad);
    else
	printf("\nPASSED ALL TESTS\n");

    return 0;
}





