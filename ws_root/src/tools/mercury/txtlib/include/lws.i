/****
 *
 *	@(#) k9LWS.i 95/07/12 1.50
 *	Copyright 1994, The 3DO Company
 *
 * Internal types and constants for LWS
 *
 ****/
#ifndef _K9LWS_I
#define _K9LWS_I

#if !defined(__cplusplus) || defined(GFX_C_Bind)
/***
 *
 * LWS is a structure that represents the INTERNAL FORMAT of
 * class LWS in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! LWS
 * attributes should only be accessed by the LWS_XXX functions.
 *
 ***/

#define	LWS_MaxNameSize	 100	/* maximum size (# chars) for LWS name */

typedef struct LWS
   {
    struct LWS*	Parent;			/* -> file which included this file */
    struct LWS*	NextFile;		/* -> next open file */
    int32           Lines;          /* number of lines */
    ByteStream*	    Stream;			/* input stream */
    gfloat          Version;        /* version number */
    int32           Units;          /* unit of measurement */
    gfloat          Scale;          /* amount to scale vertices by */
    uchar*	    BinaryData;		/* pointer to binary data */
    char            FileName[LWS_MaxNameSize];
   } LWS;
#endif

/*
 * case insensitive string equality test
 */
#define SAME(_a, _b)    (strcasecmp(_a,_b) == 0)

/*
 * Error and debug macros
 */
#define	LWS_ERROR(x)	(printf("LWS file: %s line %d: ", cur_LWS->FileName, cur_LWS->Lines), printf x)

#ifdef _DEBUG
#define	LWS_DEBUG(l, x)	if (dbg_LWS >= (l)) printf x;

#else
#define	LWS_DEBUG(l, x)
#endif

/*
 * Class ID values for built-in LWS types that are not objects.
 * Note: these are all negative odd values because we do not want them
 * to conflict with object class IDs (which are 1, 2, 3, ...) in C and
 * memory addresses in C++.
 */
#define LWS_None                0
#define LWS_Function            (-1)
#define LWS_Int                 (-3)
#define LWS_Float               (-5)
#define LWS_Color               (-7)
#define LWS_Point               (-9)
#define LWS_Box                 (-11)
#define LWS_Link 				(-13)

/*
 * Units enumeration values
 */
#define LWS_Meters			0
#define LWS_Kilometers		1
#define LWS_Feet			2
#define LWS_Inches			3
#define LWS_NautMiles		4

/*
 * token types returned from parser
 */
#define T_END       0
#define T_KEYWORD   1
#define T_NUMBER    2
#define T_LBRACE    3
#define T_RBRACE    4
#define T_LPAREN    5
#define T_RPAREN    6
#define T_OR        7
#define T_COMMA     8
#define T_LBRACK    9
#define T_RBRACK   10
#define T_BITFIELD 11
#define T_UNKNOWN  12

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif
/*
void	 	lws_Init(LWS* LWS);
*/
char*		lws_tolower(char*);
int32		lws_get_token(void);
void		lws_unget_token(void);
void		lws_bad_token(int32);
bool		lws_check_token(int32);
bool		lws_read_int(int32*);
bool		lws_read_float(gfloat*);
char*		lws_read_name(char*);
char*		lws_read_string(void);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

/*
 * Internal global data
 */
extern	LWS*	root_LWS;		/* LWS file currently open */
extern	LWS*	cur_LWS;		/* LWS file currently parsing */


extern	int			tokenType;		/* type of input token */
extern	char		token[256];		/* input token buffer */

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#ifdef _DEBUG
extern	int			dbg_LWS;		/* debug level */
#endif

#endif /* K9LWS_I */
