#ifndef __STDARG_H
#define __STDARG_H


/******************************************************************************
**
**  @(#) stdarg.h 96/02/20 1.10
**
**  Standard C assert definitions
**
******************************************************************************/


#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
	char	__gpr;
	char	__fpr;
	char	*__mem;
	char	*__reg;
} va_list[1];

#pragma pure_function __va_mem, __va_reg, __va_gpr, __va_fpr

extern int __va_gpr(void), __va_fpr(void);
extern char *__va_mem(void), *__va_reg(void);
extern void *__va_arg(va_list, int);

#ifdef __softfp
#define __va_one_gpr(mode)	(sizeof(mode,2) <= 7 || sizeof(mode,2) == 14 || sizeof(mode,2) == 19)
#define __va_two_gpr(mode)	(sizeof(mode,2) <= 9 || sizeof(mode,2) == 15)
#else
#define __va_one_gpr(mode)	(sizeof(mode,2) <= 7 || sizeof(mode,2) == 19)
#define __va_two_gpr(mode)	(sizeof(mode,2) <= 9)
#endif

#define va_start(list,parmN) {\
	(list[0]).__mem = __va_mem();\
	(list[0]).__reg = __va_reg();\
	(list[0]).__gpr = __va_gpr();\
	(list[0]).__fpr = __va_fpr();\
}
#define va_end(list)
#define va_arg(list, mode) (\
	 __va_one_gpr(mode) ? (			/* char-ulint + ptr (float if SWFP) */\
		(*(mode *)((char *)__va_arg(list, 1)+sizeof(int)-sizeof(mode)))\
	) : __va_two_gpr(mode) ? (		/* llint-ullint (double if SWFP) */\
		(*(mode *)__va_arg(list, 2))\
	) : (sizeof(mode,2) == 14) ? (		/* float (602, -Xno-double) */\
		(*(mode *)__va_arg(list, 4))\
	) : (sizeof(mode,2) == 15) ? (		/* double */\
		(*(mode *)__va_arg(list, 3))\
	) : (					/* structs and others passed as pointers */\
		(*(mode *)__va_arg(list, 0))\
	)\
)


#ifdef	__cplusplus
}
#endif


#endif /* __STDARG_H */
