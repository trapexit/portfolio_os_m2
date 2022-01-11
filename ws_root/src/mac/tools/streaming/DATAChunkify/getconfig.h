/****************************************************************************
**
**  @(#) getconfig.h 96/11/20 1.2
**
**  defines for parsing and converting command line arguments according to
**	supplied rules
**
*****************************************************************************/

#ifndef _GET_CONFIG_H
#define _GET_CONFIG_H

#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif


/* variable type identifiers */
enum VarType
{
	UINT32_TYPE		= 1,		/* take next arg as uint32 value */
	INT32_TYPE,					/* take next arg as int32 value */
	FLOAT_TYPE,					/* take next arg as float value */
	STRING_TYPE,				/* take next arg as string value */
	BOOL_W_ARG_TYPE,			/* take next arg as bool value */
	BOOL_TOGGLE_VAL_TYPE,		/* toggle existing bool value */
	COPY_T0_BUFFER				/* "memcpy()" the number of bytes */
								/*  specified in "argFlags" into value */
								/* NOTE: "value" is assumed to be large */
								/*  enough to hold at least "argFlags" bytes */
};

/* struct passed to GetConfigSettings() to identify valid arguments */
typedef struct _OptConvertRule		
{
	char		*argStr;		/* what string to match */
	int16		argType;		/* what type of value to convert arg to */
	int16		argFlags;		/* flags defining how to convert args */
	void		*valueAddr;		/* address to deposit converted value */
} OptConvertRule;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern bool	GetConfigSettings(int *argc, char **argv, OptConvertRule *defTable, 
								bool *verbose, int32 minArgCount, int32 maxArgCount);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _GET_CONFIG_H */
