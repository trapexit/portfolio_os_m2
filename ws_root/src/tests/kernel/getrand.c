#include <stdio.h>
#include <stdlib.h>
#include <kernel/random.h>

void	main(void)
{
    printf("A random number is %d\n", ReadHardwareRandomNumber());
    exit(0);
}

