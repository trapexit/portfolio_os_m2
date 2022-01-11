#include <stdio.h>
#include <math.h>

float dtime( int32 );

void main(void)
{
    float t1,t2,t3;
    int32 i;

    dtime(1);
    for(i=0;i<100;i++)
	puts("12345");
    t1 = dtime(0);

    dtime(1);
    for(i=0;i<100;i++)
	puts("1234567890123456789012345678901234567890");
    t2 = dtime(0);

    dtime(1);
    for(i=0;i<100;i++)
	puts("12345678901234567890123456789012345678901234567890123456789012345678901234567890");
    t3 = dtime(0);

    printf("\n\n100 lines of 5 chars in %f seconds\n",t1);
    printf("100 lines of 40 chars in %f seconds\n",t2);
    printf("100 lines of 80 chars in %f seconds\n",t3);
}


