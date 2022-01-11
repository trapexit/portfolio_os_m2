/* @(#) mm.c 95/10/21 1.2 */

#include <stdio.h>
#include <stdlib.h>

#define rowsize 40

float rma[rowsize+1][rowsize+1], rmb[rowsize+1][rowsize+1], rmr[rowsize+1][rowsize+1];

    /* Multiplies two real matrices. */

void rInitmatrix ( float m[rowsize+1][rowsize+1])
{
	int temp, i, j;
	for ( i = 1; i <= rowsize; i++ )
	    for ( j = 1; j <= rowsize; j++ ) {
			temp = rand();
			m[i][j] = (temp - (temp/120)*120 - 60)/3;
		}
}

void rInnerproduct(float *result, float a[rowsize+1][rowsize+1],
                   float b[rowsize+1][rowsize+1], int row, int column)
	/* computes the inner product of A[row,*] and B[*,column] */
{
	int i;
	*result = 0.0;
	for (i = 1; i<=rowsize; i++)
		*result = *result+a[row][i]*b[i][column];
}

void Realmm (void)
{
    int i, j;

    rInitmatrix (rma);
    rInitmatrix (rmb);
    for ( i = 1; i <= rowsize; i++ )
		for ( j = 1; j <= rowsize; j++ )
			rInnerproduct(&rmr[i][j],rma,rmb,i,j);
}
