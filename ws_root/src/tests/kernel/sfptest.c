/* @(#) sfptest.c 96/05/15 1.16 */

#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <hardware/PPCasm.h>

#define ASINT(f)	(* (int *) ((void *) &f))

extern void printf(const char *str, ...);
extern void Superkprintf(const char *str, ...);


#define BROKEN_FLOAT double

float   returnarg(int n, ...)
{
    va_list a;
    float r;

	TOUCH(n);

    va_start(a, n);
    r = va_arg(a, BROKEN_FLOAT);

    va_end(a);

    return r;
}




float	testfn(float f)
{
    return f + 1.0;
}


static int printing = 0;
static float f = 1.0;

void	count(void)
{
    int numruns = 10000;
    int i;

    if(printing)
	printf("Pretest, f = %x\n", ASINT(f));
    for(i = 0; i < numruns; i++)
	f = testfn(f);
    if(printing)
	printf("Posttest, f = %x\n", ASINT(f));
    f -= numruns;
    if(printing)
	printf("Normalized, f = %x\n", ASINT(f));
}


void looserfunct(float g);



long turtle = 100;

int main(void)
{
  int i;
  int *x= &i;

  printf("Welcome to 602 FPU test stack at %x\n", x);

  f = 2.0f;

  printf("This is a float as hex %x\n", ASINT(f));

  f = returnarg(10, f);
  printf("This is a munged float %x\n", ASINT(f));
  printf("This is the same float %f\n", f);

  printf("Here is a number as a float %f\n", (float) turtle);
  printf("Here is a big number as a float %f\n", (float) LONG_MAX);

  if(f * 100 > (float) (LONG_MAX-1))
      {
      float bigf = (float) LONG_MAX - 1;

      printf("What! %f larger than %x\n", f * 100, LONG_MAX-1);
      printf("As a float %x\n", ASINT(bigf));
      }
  else
      printf("Everything workd\n");

#if 0
  for(i = 0; i < 10; i++)
      count();
  printing = 1;
  count();
#endif

  printf("Exiting 602 FPU test\n");

  return 0;
}



void looserfunct(float g)
{
  eieio();
  f = g;
}
