
// xsym.h
// 		definitions for xsym file format
//		which aren't in DebugTypes.h


#ifndef __XSYM_H__	
#define __XSYM_H__	

#define XT_void				0
//#define XT_pointer		??
#define XT_char				7
#define XT_uchar			6
#define XT_schar			8	//??? Reggie had 8 & 9 mapping to 100 - a byte...
#define XT_byte				9	//???
#define XT_short			11
#define XT_ushort			10
//#define XT_int				11
//#define XT_uint				10
#define XT_long				3
#define XT_ulong			2
#define XT_float			12
#define XT_double			4
//#define XT_double			13	//Reggie had 13 for double...
//#define XT_ldouble			4
//#define XT_complex		??
//#define XT_dcomplex		??
#define XT_string			16
#define XT_13				13
#define XT_1				1
#define XT_5				5
#define XT_14				14
#define XT_15				15
#define XT_17				17

enum
{
	ERR_6304_NO_RTE 			= 6304,
	ERR_6308_ZERO_RTE			= 6308,
	ERR_6312_NO_ROOT			= 6312,
	ERR_6332_NTE_ACCESS			= 6332,
	ERR_6336_TTE_ACCESS			= 6336,
	ERR_6340_WRONG_SPACE		= 6340,
	ERR_6344_MTE_ACCESS			= 6344,
	ERR_6348_RTE_ACCESS			= 6348,
	ERR_6352_NO_FREE_TTE_SLOT	= 6352,
	ERR_6356_TTE_READ			= 6356,
	ERR_6360_TTE_CACHE			= 6360,
	ERR_6364_TTE_SADE			= 6364,
	ERR_6368_ALLOCMEM			= 6368,
	ERR_6372_BAD_CONST			= 6372,
	ERR_6376_NOT_SYM_FILE		= 6376,
	ERR_6969_WRONG_VERSION		= 6969
};

#endif /* __XSYM_H__ */
