#ifndef __KERNEL_DDF_H
#define __KERNEL_DDF_H


/******************************************************************************
**
**  @(#) ddf.h 96/02/20 1.6
**
**  Device Description File definitions.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

/*
 * Structure of a DDF file.
 * A DDF file is an IFF file.
 * It consists of a sequence of IFF chunks.
 *
 * The description of each device starts with a DEVN chunk.
 * The data in a DEVN chunk is the name of the device.
 * Following the DEVN chunk is a NEED chunk and a PROV chunk.
 *
 * The data in a NEED or PROV chunk is a sequence of tokens.
 * Each token consists of one or more bytes.  A token starts with
 * a token byte, which identifies the type of the token.
 * Depending on the token type, there may be more bytes following
 * the token byte which are part of the token.
 */

/*
 * Token types: stored in the high order 4 bits of the token byte.
 */
#define	TOK_INT1	1	/* Integer: value in low 4 bits of token byte */
#define	TOK_INT2	2	/* 2 byte integer */
#define	TOK_INT3	3	/* 3 byte integer */
#define	TOK_INT4	4	/* 4 byte integer */
#define	TOK_INT5	5	/* 5 byte integer */
#define	TOK_INT		6	/* reserved */
#define	TOK_STRING	8	/* String: chars follow token byte */
#define	TOK_KEYWORD	9	/* Keyword: see below */
#define	TOK_OP		10	/* Operator: see below */

#define	TOK_INT_NEG	(1<<3)	/* Sign bit, for TOK_INT1 - TOK_INT5 */

/*
 * Keywords: stored in the low order 4 bits of the token byte,
 * if the token type is TOK_KEYWORD.
 */
#define	K_END_PROVIDE		1
#define	K_END_PROVIDE_SECTION	2
#define	K_END_NEED		3
#define	K_END_NEED_SECTION	4
#define	K_OR 			5

/*
 * Operators: stored in the low order 4 bits of the token byte,
 * if the token type is TOK_OP.
 */
#define	OP_EQ		0x1	/* Equal */
#define	OP_GT		0x2	/* Greater than */
#define	OP_LT		0x3	/* Less than */
#define	OP_DEFINED	0x4	/* Attribute name defined */

#define	OP_NOT		0x8	/* Negate */

#endif /* __KERNEL_DDF_H */
