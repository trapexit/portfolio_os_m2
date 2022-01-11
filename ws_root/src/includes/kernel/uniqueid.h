#ifndef __KERNEL_UNIQUEID_H
#define __KERNEL_UNIQUEID_H


/******************************************************************************
**
**  @(#) uniqueid.h 96/01/10 1.3
**
**  Unique ID chip interface.
**
******************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


typedef struct
{
    uint32 u_upper;
    uint32 u_lower;
} UniqueID;


Err ReadUniqueID(UniqueID *id);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects ReadUniqueID(1)
#endif


/*****************************************************************************/


#endif /* __KERNEL_UNIQUEID_H */
