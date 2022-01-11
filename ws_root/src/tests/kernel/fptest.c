/* @(#) fptest.c 95/10/21 1.13 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/kernel.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define testfn(fname, start, end, ntests) dotest(#fname, fname, start, end, ntests)

typedef float (*floatfn)(float);


void dotest(const char *name, floatfn fn, float start, float end, int ntests)
{
  int i;
  float current, result;

  for(i = 0; i < ntests; i++)
    {
      current = start + ((end - start) / ntests) * i;
      result = (*fn)(current);

      printf("%s(%f) = %f (%d of %d)\n", name, current, result, i + 1, ntests);
    }
}



#define M_2PI (M_PI*2)

int main (void)
{
  float a = 7.0,
  b = 2.31,
  c = -134.767;

  printf("Hello world.\n");
  printf("The number 7: %f\n", a);
  printf("The number 7 (as parts): %x,%x\n", ((uint32 *) &a)[0]);
  printf("The number 7 (rounded): %x\n", (int)a);
  printf("The number 2.31: %f\n", b);
  printf("The number -134.767: %f\n", c);
  printf("Testing functions...\n");

  testfn(sqrtf, 1, 100, 13);
  testfn(expf, 1, 10, 13);
  testfn(logf, 2, 10, 13);
  testfn(log10f, 2, 10, 13);
  testfn(acosf, 0.1, 0.9, 10);
  testfn(asinf, 0.1, 0.9, 10);
  testfn(atanf, 0.1, 0.9, 10);

  testfn(tanf, M_PI / 2 / 10, M_PI / 2, 10);
  testfn(sinf, 0, M_2PI, 16);
  testfn(cosf, 0, M_2PI, 16);
  testfn(tanhf, M_PI / 2 / 10, M_PI / 2, 10);
  testfn(sinhf, 0, M_2PI, 16);
  testfn(coshf, 0, M_2PI, 16);

  return 0;
}



