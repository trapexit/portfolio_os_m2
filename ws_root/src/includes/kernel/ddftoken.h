#ifndef __KERNEL_DDFTOKEN_H
#define __KERNEL_DDFTOKEN_H


/******************************************************************************
**
**  @(#) ddftoken.h 96/02/20 1.3
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

/*
 * Expanded version of the tokens in a DDF file.
 */
typedef struct DDFToken
{
	uint8	tok_Type;
	union 	tok_Value
	{
	    uint32	v_Int;
	    char *	v_String;
	    uint8	v_Keyword;
	    uint8	v_Op;
	}	tok_Value;
} DDFToken;

/*
 * Pointer to a sequence of tokens.
 */
typedef struct DDFTokenSeq
{
	uint8 *	tokseq_Ptr;
} DDFTokenSeq;

#define InitTokenSeq(seq,p) ((seq)->tokseq_Ptr = (p))


#endif /* __KERNEL_DDFTOKEN_H */
