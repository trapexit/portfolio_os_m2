/* @(#) strtof.c 96/02/07 1.11 */

#include <kernel/types.h> 
#include <ctype.h>


/*****************************************************************************/

#define MAX10	6

static float tentothe2tothe[MAX10] = 
{ 1.0e1, 1.0e2, 1.0e4, 1.0e8, 1.0e16, 1.0e32 };

static float pow10( int exp )
{
    int i=MAX10, j=32, minus;
    float f=1.0;
    
    if (minus = (exp < 0) )
	exp = -exp;

    while( --i >= 0 ) {	
	if( exp >= j ) {       
	    f *= tentothe2tothe[i];
	    exp -= j;
	}
	j >>= 1;
    }

    return (minus) ? (1.0 / f) : f;
}                                                



float strtof(const char *str, char **endScan)
{
    float x=0.0, div;
    int negsign, exp=0, expsign;

    /* eat any leading whitespace */
    while( isspace(*str) )
	str++;

    if( negsign = (*str == '-') )
	str++;
    else if( *str == '+' )
	str++;
   
    /* now read in the first part of the number */
    while( isdigit( *str ) ) 
	x = 10.0 * x + (*str++ - '0' );

    /* if we hit a period, do the decimal part now */
    if( *str == '.' ) {
	str++;
	div = 10.0;
	while( isdigit(*str) ) {
	    x += (*str++ - '0') / div;
	    div *= 10.0;
	}
    }

    /* check for an exponent */
    if( (*str == 'e') || (*str == 'E') ) {
	str++;

	if( expsign = (*str == '-') )
	    str++;
	else if( *str == '+' )
	    str++;

	/* handle leading zeros, such as in 1.0e-07 or 1.0e001 */
	while( *str == '0' )
	    str++;

	/* now do the exponent */
	while( isdigit( *str ) ) 
	    exp = 10 * exp + (*str++ - '0' );
	if( expsign ) 
	    exp = -exp;

	if( exp )
	    x *= pow10( exp );
    }
    
    if( negsign )
	x = -x;

    if (endScan)
	*endScan = (char *)str;

    return x;
}

