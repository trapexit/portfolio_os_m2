#include <stdio.h>

/* The eight queens problem, solved 50 times. */
/*
type    
    doubleboard =   2..16;
    doublenorm  =   -7..7;
    boardrange  =   1..8;
    aarray      =   array [boardrange] of boolean;
    barray      =   array [doubleboard] of boolean;
    carray      =   array [doublenorm] of boolean;
    xarray      =   array [boardrange] of boardrange;
*/

void Try(int i,int *q,int a[],int b[],int c[],int x[])
    {
    int     j;
    j = 0;
    *q = false;
    while ( (! *q) && (j != 8) )
	{ j = j + 1;
	*q = false;
	if ( b[j] && a[i+j] && c[i-j+7] )
	    { x[i] = j;
	    b[j] = false;
	    a[i+j] = false;
	    c[i-j+7] = false;
	    if ( i < 8 )
		{ Try(i+1,q,a,b,c,x);
		if ( ! *q )
		    { b[j] = true;
		    a[i+j] = true;
		    c[i-j+7] = true;
		    }
		}
	    else *q = true;
	    }
	}
    }

void Doit (void )
{
	int i,q;
	int a[9], b[17], c[15], x[9];
	i = 0 - 7;
	while ( i <= 16 )
	{ 
		if ( (i >= 1) && (i <= 8) ) 
			a[i] = true;
	    	if ( i >= 2 ) 
			b[i] = true;
	    	if ( i <= 7 ) 
			c[i+7] = true;
	    	i = i + 1;
	}

	Try(1, &q, b, a, c, x);
	if ( ! q )
    		printf (" Error in Queens.\n");
}

void Queens (void)
{
	int i;
	for ( i = 1; i <= 50; i++ ) Doit();
}
