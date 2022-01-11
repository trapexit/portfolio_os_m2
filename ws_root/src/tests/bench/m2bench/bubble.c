/* @(#) bubble.c 95/10/21 1.2 */

#include <stdio.h>
#include <stdlib.h>

#define srtelements 	  500
#define SEED              74755

static int    sortlist[srtelements+1],
    biggest, littlest,
    top;

    /* Sorts an array using bubblesort */

void bInitarr(void)
{
	int i, temp;
	srand(SEED);
	biggest = 0; littlest = 0;
	for ( i = 1; i <= srtelements; i++ )
	    {
	    temp = rand();
	    sortlist[i] = temp - (temp/100000L)*100000L - 50000L;
	    if ( sortlist[i] > biggest ) biggest = sortlist[i];
	    else if ( sortlist[i] < littlest ) littlest = sortlist[i];
	    }
}

void Bubble(void)
{
    int i, j;
    bInitarr();
    top=srtelements;

    while ( top>1 ) {
		i=1;
		while ( i<top ) {

		    if ( sortlist[i] > sortlist[i+1] ) {
				j = sortlist[i];
				sortlist[i] = sortlist[i+1];
				sortlist[i+1] = j;
			}
		    i=i+1;
		}
		top=top-1;
	}
    if ( (sortlist[1] != littlest) || (sortlist[srtelements] != biggest) )
		printf ( "Error3 in Bubble.\n");
}

