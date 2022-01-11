/* @(#) quick.c 95/10/21 1.2 */

#include <stdio.h>
#include <stdlib.h>

#define SEED 74755
#define sortelements 5000

static int    sortlist[sortelements+1],
    biggest, littlest;

    /* Sorts an array using quicksort */

void Initarr(void)
{
	int i, temp;
	srand(SEED);
	biggest = 0; littlest = 0;
	for ( i = 1; i <= sortelements; i++ )
	    {
	    temp = rand();
	    sortlist[i] = temp - (temp/100000L)*100000L - 50000L;
	    if ( sortlist[i] > biggest ) biggest = sortlist[i];
	    else if ( sortlist[i] < littlest ) littlest = sortlist[i];
	    }
}

void Quicksort( int a[], int l, int r)
	/* quicksort the array A from start to finish */
{
	int i,j,x,w;

	i=l; j=r;
	x=a[(l+r) / 2];
	do {
	    while ( a[i]<x ) i = i+1;
	    while ( x<a[j] ) j = j-1;
	    if ( i<=j ) {
		w = a[i];
		a[i] = a[j];
		a[j] = w;
		i = i+1;    j= j-1;
		}
	} while ( i<=j );
	if ( l <j ) Quicksort(a,l,j);
	if ( i<r ) Quicksort(a,i,r);
}


void Quick (void)
{
    Initarr();
    Quicksort(sortlist,1,sortelements);
    if ( (sortlist[1] != littlest) || (sortlist[sortelements] != biggest) )
	printf ( " Error in Quick.\n");
}
