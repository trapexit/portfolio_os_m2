#ifndef __ERRNO_H
#define __ERRNO_H

/******************************************************************************
**
**  @(#) errno.h 96/05/09 1.3
**
**  Standard C error codes
**
******************************************************************************/

#include <kernel/operror.h>

#define EDOM   1
#define ERANGE 2

extern int errno;

/* STDIO functions in the C lib return portfolio error codes in errno;
 * those inherited from OS calls and the following particular to the 
 * library itself.
 */

/*****************************************************************************/

#define ER_CLIB			MakeErrId('C','L')
#define MakeCLibErr(svr,class,err) MakeErr(ER_LINKLIB,ER_CLIB,svr,ER_E_SSTM,class,err)

/* Operation not supported on this descriptor's medium */
#define C_ERR_NOTSUPPORTED		MakeCLibErr(ER_SEVERE, ER_C_NSTND, 1)

/* Descriptor owned by system */
#define C_ERR_SYSTEMDESCRIPT	MakeCLibErr(ER_SEVERE, ER_C_NSTND, 2)

/*****************************************************************************/

#endif /* __ERRNO_H */

