#ifndef __DIPIR_RSA_H
#define __DIPIR_RSA_H


/******************************************************************************
**
**  @(#) rsa.h 96/02/20 1.5
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#define MD4BLOCKSIZE	2048
#define TABLESIZE(datasize)	((datasize+MD4BLOCKSIZE-1)/MD4BLOCKSIZE)


#endif /* __DIPIR_RSA_H */
