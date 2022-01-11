/* @(#) descript.c 96/05/09 1.4 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* ------------------------------------------------------------------------- */
/* The standard descriptors for stdin, stdout, and stderr                    */
/* ------------------------------------------------------------------------- */

uint8 stdoutbuffer[BUFSIZ>>4];
uint8 stderrbuffer[BUFSIZ>>4];

struct FILE istdout=
    {&stdoutbuffer[0],  BUFSIZ>>2,  0,  SIOF_CONSOLE | SIOF_TYPE_LINEMODE | SIOF_OWNEDSYSTEM,    NULL,   0}; /* stdout */
struct FILE istdin = 
    {NULL,              0,          0,  SIOF_CONSOLE | SIOF_TYPE_LINEMODE | SIOF_OWNEDSYSTEM,    NULL,   0}; /* stdin */
struct FILE istderr = 
    {&stderrbuffer[0],  BUFSIZ>>2,  0,  SIOF_CONSOLE | SIOF_TYPE_LINEMODE | SIOF_OWNEDSYSTEM,    NULL,   0}; /* stderr */

struct FILE *stdout = &istdout, *stdin = &istdin, *stderr = &istderr;

