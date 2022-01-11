/* @(#) fft.c 95/10/21 1.2 */

#include <stdio.h>

#define SEED 74755

#define fftsize 	 256
#define fftsize2 	 129
struct    complex { float rp, ip; } ;
float value;
float    fixed,floated;
struct complex    z[fftsize+1], w[fftsize+1],
    e[fftsize2+1];
float    zr, zi;

float Cos (float x)
/* computes cos of x (x in radians) by an expansion */
{
int i, factor;
float    result,power;

   result = 1.0; factor = 1;  power = x;
   for ( i = 2; i <= 10; i++ ) {
      factor = factor * i;  power = power*x;
      if ( (i & 1) == 0 )  {
        if ( (i & 3) == 0 ) result = result + power/factor;
	else result = result - power/factor;
      }
   }
   return (result);
}

int Min0( int arg1, int arg2)
    {
    if ( arg1 < arg2 )
	return (arg1);
    else
	return (arg2);
    }

void Printcomplex(  int arg1, int arg2, struct complex zarray[],
                    int start, int finish, int increment)
{
    int i;

    TOUCH(arg1);
    TOUCH(arg2);

    printf("\n") ;

    i = start;
    do {
	printf("  %15.3e%15.3e",zarray[i].rp,zarray[i].ip) ;
	i = i + increment;
	printf("  %15.3e%15.3e",zarray[i].rp,zarray[i].ip) ;
	printf("\n");
	i = i + increment ;
    } while ( i <= finish );
}

void Uniform11(int iy, float yfl)
{
    iy = (4855*iy + 1731) & 8191;
    yfl = iy/8192.0;
    TOUCH(yfl);
} /* uniform */

Exptab(int n, struct complex e[])
{ /* exptab */
    float theta, divisor, h[26];
    int i, j, k, l, m;

    theta = 3.1415926536;
    divisor = 4.0;
    for ( i=1; i <= 25; i++ )
	{
	h[i] = 1/(2*Cos( theta/divisor ));
	divisor = divisor + divisor;
	}

    m = n / 2 ;
    l = m / 2 ;
    j = 1 ;
    e[1].rp = 1.0 ;
    e[1].ip = 0.0;
    e[l+1].rp = 0.0;
    e[l+1].ip = 1.0 ;
    e[m+1].rp = -1.0 ;
    e[m+1].ip = 0.0 ;

    do {
	i = l / 2 ;
	k = i ;

	do {
	    e[k+1].rp = h[j]*(e[k+i+1].rp+e[k-i+1].rp) ;
	    e[k+1].ip = h[j]*(e[k+i+1].ip+e[k-i+1].ip) ;
	    k = k+l ;
	} while ( k <= m );

	j = Min0( j+1, 25);
	l = i ;
    } while ( l > 1 );

} /* exptab */

void Fft( int n, struct complex z[], struct complex w[], struct complex e[],
          float sqrinv)
{
    int i, j, k, l, m, index;
    m = n / 2 ;
    l = 1 ;

    do {
	k = 0 ;
	j = l ;
	i = 1 ;

	do {

	    do {
		w[i+k].rp = z[i].rp+z[m+i].rp ;
		w[i+k].ip = z[i].ip+z[m+i].ip ;
		w[i+j].rp = e[k+1].rp*(z[i].rp-z[i+m].rp)
		-e[k+1].ip*(z[i].ip-z[i+m].ip) ;
		w[i+j].ip = e[k+1].rp*(z[i].ip-z[i+m].ip)
		+e[k+1].ip*(z[i].rp-z[i+m].rp) ;
		i = i+1 ;
	    } while ( i <= j );

	    k = j ;
	    j = k+l ;
	} while ( j <= m );

	/*z = w ;*/ index = 1;
	do {
	    z[index] = w[index];
	    index = index+1;
	} while ( index <= n );
	l = l+l ;
    } while ( l <= m );

    for ( i = 1; i <= n; i++ )
	{
	z[i].rp = sqrinv*z[i].rp ;
	z[i].ip = -sqrinv*z[i].ip;
	}

}

void Oscar(void)
{ /* oscar */
	int i;
	Exptab(fftsize,e) ;
	for ( i = 1; i <= fftsize; i++ )
	    {
	    Uniform11( 5767, zr );
	    Uniform11( 5767, zi );
	    z[i].rp = 20.0*zr - 10.0;
	    z[i].ip = 20.0*zi - 10.0;
	    }


	for ( i = 1; i <= 20; i++ ) {
	   Fft(fftsize,z,w,e,0.0625) ;
	   /* Printcomplex( 6, 99, z, 1, 256, 17 ); */
	}
} /* oscar */
