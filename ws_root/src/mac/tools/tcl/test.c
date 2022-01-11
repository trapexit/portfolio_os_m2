
#include <stdio.h>
#include <math.h>
#include <sane.h>

main()
{
	printf ( "This is \\n -> %d[x%02X] \\r -> %d[x%02X]\n",
			'\n', '\n', '\r', '\r');
	
#ifdef NEVER_DEFINED
double a, b, c;
long ex;

	a = 0.0;
	b = 5.0;
	
	c = a / b;
	if ( ex = testexception( INVALID | UNDERFLOW | OVERFLOW | DIVBYZERO | INEXACT) )
		{
		fprintf(stderr, "ONE: HAVE EXCEPTION\n");
		}
	fprintf(stderr, "%lf / %lf = %lf\n", a, b, c);
	
	c = b / a;
	if ( ex = testexception( INVALID | UNDERFLOW | OVERFLOW | DIVBYZERO | INEXACT) )
		{
		fprintf(stderr, "TWO: HAVE EXCEPTION x%08lX [%ld]\n", ex, ex);
		if (testexception( INVALID ))
			fprintf(stderr, "INVALID\n");
		else if (testexception( UNDERFLOW ))
			fprintf(stderr, "UNDERFLOW\n");
		else if (testexception( OVERFLOW ))
			fprintf(stderr, "OVERFLOW\n");
		else if (testexception( DIVBYZERO ))
			fprintf(stderr, "DIVBYZERO\n");
		else if (testexception( INEXACT ))
			fprintf(stderr, "INEXACT\n");
		}
	fprintf(stderr, "%lf / %lf = %lf\n", a, b, c);
#endif

	exit(0);
	}


#ifdef NEVER_DEFINED

C {active}
Link -t MPST -c 'MPS ' ¶
	{active}.o		¶
	{Libraries}RunTime.o ¶
	{Libraries}Interface.o ¶
	{CLibraries}StdCLib.o ¶
	{CLibraries}CSANELib.o
Link.out

This is \n -> 13[x0D] \r -> 10[x0A]

0.000000 / 5.000000 = 0.000000
TWO: HAVE EXCEPTION x00000001 [1]
DIVBYZERO
0.000000 / 5.000000 = INF
0.000000 / 5.000000 = 0.000000
0.000000 / 5.000000 = INF

#endif
