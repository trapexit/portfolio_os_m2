
#include <stdio.h>
#include <stdarg.h>

void
panic(char *format, ...)
	{
	va_list		varg;
	
	va_start(varg, format);
	
	vfprintf(stderr, format, varg);
	
	va_end(varg);
	
	exit(2);
	}
