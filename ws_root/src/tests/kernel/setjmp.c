#include <stdio.h>
#include <setjmp.h>

int main(void)
{
	jmp_buf buf;
	int i;
	
	printf("Testing setjmp: ");
	if ((i = setjmp(buf)) == 0)
		{
		printf("setjmp worked!\n");
		}
	else
		{
		printf ("The value returned from setjmp is %d\n", i);
		/* Let's exit the process */
		return 0;
		}
	printf ("Now we want to try it again...\n");
	/* Now try it again */
	longjmp (buf, 107);
	/* Does not return */
}
