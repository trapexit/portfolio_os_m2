/*	Implementation module : va_arg.c

	Copyright 1995 Diab Data, Inc.

	Description :
	Implemention of PowerPC ABI function
	void *__va_arg(va_list argp, int type)


	History :
	When	Who	What
	950523	teve	initial
*/

/**************	Imported modules ********************************/

#undef __EABI__
#define __EABI__	1

#include <stdarg.h>

/**************	Local data, types, fns and macros ***************/


/**************	Implementation of exported functions ************/

void *__va_arg(va_list argp, int type)
{
	int index;
	char *rp;

	if (type == 1) {	/* arg_WORD	*/
		index = argp->__gpr;
		if (index < 8) {
			argp->__gpr = index + 1;
			return argp->__reg + index*4;
		} else {
			rp = argp->__mem;
			argp->__mem = rp + 4;
			return rp;
		}
	} else if (type == 3) {	/* arg_ARGREAL	*/
		index = argp->__fpr;
		if (index < 8) {
			argp->__fpr = index + 1;
			return argp->__reg + index*8 + 32;
		} else {
			rp = argp->__mem;
			rp = (char *)(((long)rp + 7) & ~7);
			argp->__mem = rp + 8;
			return rp;
		}
	} else if (type == 4) {	/* arg_ARGSINGLE */
		index = argp->__fpr;
		if (index < 8) {
			argp->__fpr = index + 1;
			return argp->__reg + index*4 + 32;
		} else {
			rp = argp->__mem;
			argp->__mem = rp + 4;
			return rp;
		}
	} else if (type == 2) {	/* arg_DOUBLEWORD */
		index = argp->__gpr;
		index = (index + 1) & ~1;
		if (index < 7) {
			argp->__gpr = index + 2;
			return argp->__reg + index*4;
		} else {
			rp = argp->__mem;
			argp->__gpr = index;
			rp = (char *)(((long)rp + 7) & ~7);
			argp->__mem = rp + 4;
			return rp;
		}
	} else if (type == 0) {	/* arg_ARGPOINTER */
		index = argp->__gpr;
		if (index < 8) {
			argp->__gpr = index + 1;
			return *(void **)(argp->__reg + index*4);
		} else {
			rp = argp->__mem;
			argp->__mem = rp + 4;
			return *(void **)rp;
		}
	}
	return 0;
}

