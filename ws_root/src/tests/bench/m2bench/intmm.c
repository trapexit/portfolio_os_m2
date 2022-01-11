/* @(#) intmm.c 95/10/21 1.2 */

#include <stdio.h>
#include <stdlib.h>

#define rowsize 40

int ima[rowsize+1][rowsize+1], imb[rowsize+1][rowsize+1], imr[rowsize+1][rowsize+1];

    /* Multiplies two integer matrices. */

void Initmatrix ( int m[rowsize+1][rowsize+1])
{
	int temp, i, j;
	for ( i = 1; i <= rowsize; i++ )
	    for ( j = 1; j <= rowsize; j++ ) {
			temp = rand();
			m[i][j] = temp - (temp/120)*120 - 60;
	  	}
}

void Innerproduct(int *result, int a[rowsize+1][rowsize+1], int b[rowsize+1][rowsize+1],
                  int row, int column)
	/* computes the inner product of A[row,*] and B[*,column] */
{
	int i;
	*result = 0;
	for(i = 1; i <= rowsize; i++ )
		*result = *result+a[row][i]*b[i][column];
}

void Intmm (void)
{
    int i, j;

    srand(74755);
    Initmatrix (ima);
    Initmatrix (imb);
    for ( i = 1; i <= rowsize; i++ )
		for ( j = 1; j <= rowsize; j++ )
			Innerproduct(&imr[i][j],ima,imb,i,j);
}
