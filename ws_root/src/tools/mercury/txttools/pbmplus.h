/* pbmplus.h - header file for PBM, PGM, PPM, and PNM
**
** Copyright (C) 1988, 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#ifndef _PBMPLUS_H_
#define _PBMPLUS_H_

#ifndef applec
#include <sys/types.h>
#endif

#include <ctype.h>
#include <stdio.h>

#if ! ( defined(BSD) || defined(SYSV) || defined(MSDOS))
/* CONFIGURE: If your system is >= 4.2BSD, set the BSD option; if you're a
** System V site, set the SYSV option; and if you're IBM-compatible, set
** MSDOS.  If your compiler is ANSI C, you're probably better off setting
** SYSV.
*/
/* #define BSD */
#define SYSV
/* #define MSDOS */
#endif

/* CONFIGURE: If you want to enable writing "raw" files, set this option.
** "Raw" files are smaller, and much faster to read and write, but you
** must have a filesystem that allows all 256 ASCII characters to be read
** and written.  You will no longer be able to mail P?M files without 
** using uuencode or the equivalent, or running the files through pnmnoraw.
** Note that reading "raw" files works whether writing is enabled or not.
*/
#define PBMPLUS_RAWBITS

/* CONFIGURE: On some systems, the putc() macro is broken and will return
** EOF when you write out a 255.  For example, ULTRIX does this.  This
** only matters if you have defined RAWBITS.  To test whether your system
** is broken this way, go ahead and compile things with RAWBITS defined,
** and then try "pbmmake -b 8 1 > file".  If it works, fine.  If not,
** define BROKENPUTC1 and try again - if that works, good.  Otherwise,
** BROKENPUTC2 is guaranteed to work, although it's about twice as slow.
*/
/* #define PBMPLUS_BROKENPUTC1 */
/* #define PBMPLUS_BROKENPUTC2 */

#ifdef PBMPLUS_BROKENPUTC1
#undef putc
/* This is a fixed version of putc() that should work on most Unix systems. */
#define putc(x,p) (--(p)->_cnt>=0? ((int)(unsigned char)(*(p)->_ptr++=(unsigned char)(x))) : _flsbuf((unsigned char)(x),p))
#endif /*PBMPLUS_BROKENPUTC1*/
#ifdef PBMPLUS_BROKENPUTC2
#undef putc
/* For this one, putc() becomes a function, defined in pbm/libpbm1.c. */
#endif /*PBMPLUS_BROKENPUTC2*/

/* CONFIGURE: PGM can store gray values as either bytes or shorts.  For most
** applications, bytes will be big enough, and the memory savings can be
** substantial.  However, if you need more than 8 bits of grayscale resolution,
** then define this symbol.
*/
/* #define PGM_BIGGRAYS */

/* CONFIGURE: Normally, PPM handles a pixel as a struct of three grays.
** If grays are stored in bytes, that's 24 bits per color pixel; if
** grays are stored as shorts, that's 48 bits per color pixel.  PPM
** can also be configured to pack the three grays into a single longword,
** 10 bits each, 30 bits per pixel.
**
** If you have configured PGM with the PGM_BIGGRAYS option, AND you don't
** need more than 10 bits for each color component, AND you care more about
** memory use than speed, then this option might be a win.  Under these
** circumstances it will make some of the programs use 1.5 times less space,
** but all of the programs will run about 1.4 times slower.
**
** If you are not using PGM_BIGGRAYS, then this option is useless -- it
** doesn't save any space, but it still slows things down.
*/
/* #define PPM_PACKCOLORS */

/* CONFIGURE: uncomment this to enable debugging checks. */
/* #define DEBUG */

#ifdef SYSV

#include <string.h>
#define index(s,c) strchr(s,c)
#define rindex(s,c) strrchr(s,c)
#define srandom(s) srand(s)
#define random rand
#define bzero(dst,len) memset(dst,0,len)
#define bcopy(src,dst,len) memcpy(dst,src,len)
#define bcmp memcmp
#if !(defined(applec) || defined(BSD)) 
extern void srand();
#endif
extern int rand();

#else /*SYSV*/

#include <strings.h>
extern void srandom();
extern long random();

#endif /*SYSV*/

#ifndef applec
extern int atoi();
extern void exit();
#endif

extern long time();
extern int write();

/* CONFIGURE: On some systems, malloc.h doesn't declare these, so we have
** to do it.  On other systems, for example HP/UX, it declares them
** incompatibly.  And some systems, for example Dynix, don't have a
** malloc.h at all.  A sad situation.  If you have compilation problems
** that point here, feel free to tweak or remove these declarations.
*/
#if (!defined(applec) && !defined(MSDOS)) 
#include <malloc.h>
#endif

/* CONFIGURE: Some systems don't have vfprintf(), which we need for the
** error-reporting routines.  If you compile and get a link error about
** this routine, uncomment the first define, which gives you a vfprintf
** that uses the theoretically non-portable but fairly common routine
** _doprnt().  If you then get a link error about _doprnt, or
** message-printing doesn't look like it's working, try the second
** define instead.
*/
/* #define NEED_VFPRINTF1 */
/* #define NEED_VFPRINTF2 */

/* End of configurable definitions. */


#undef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#undef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#undef odd
#define odd(n) ((n) & 1)


/* Definitions to make PBMPLUS work with either ANSI C or C Classic. */

#if __STDC__
#define ARGS(alist) alist
#else /*__STDC__*/
#define ARGS(alist) ()
#define const
#endif /*__STDC__*/


/* Initialization. */

void pm_init ARGS(( int* argcP, char* argv[] ));


/* Variable-sized arrays definitions. */

char** pm_allocarray ARGS(( int cols, int rows, int size ));
char* pm_allocrow ARGS(( int cols, int size ));
void pm_freearray ARGS(( char** its, int rows ));
void pm_freerow ARGS(( char* itrow ));


/* Case-insensitive keyword matcher. */

int pm_keymatch ARGS(( char* str, char* keyword, int minchars ));


/* Log base two hacks. */

int pm_maxvaltobits ARGS(( int maxval ));
int pm_bitstomaxval ARGS(( int bits ));


/* Error handling definitions. */

void pm_message ARGS(( char*, ... ));
void pm_error ARGS(( char*, ... ));			/* doesn't return */
void pm_perror ARGS(( char* reason ));			/* doesn't return */
void pm_usage ARGS(( char* usage ));			/* doesn't return */


/* File open/close that handles "-" as stdin and checks errors. */

FILE* pm_openr ARGS(( char* name ));
FILE* pm_openw ARGS(( char* name ));
void pm_close ARGS(( FILE* f ));


/* Endian I/O. */

int pm_readbigshort ARGS(( FILE* in, short* sP ));
int pm_writebigshort ARGS(( FILE* out, short s ));
int pm_readbiglong ARGS(( FILE* in, long* lP ));
int pm_writebiglong ARGS(( FILE* out, long l ));
int pm_readlittleshort ARGS(( FILE* in, short* sP ));
int pm_writelittleshort ARGS(( FILE* out, short s ));
int pm_readlittlelong ARGS(( FILE* in, long* lP ));
int pm_writelittlelong ARGS(( FILE* out, long l ));


#endif /*_PBMPLUS_H_*/
