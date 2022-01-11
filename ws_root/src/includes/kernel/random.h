#ifndef __KERNEL_RANDOM_H
#define __KERNEL_RANDOM_H


/******************************************************************************
**
**  @(#) random.h 95/08/31 1.1
**
**  Hardware-based random number generator.
**
******************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


extern uint32 ReadHardwareRandomNumber(void);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects ReadHardwareRandomNumber
#endif


/*****************************************************************************/


#endif /* __KERNEL_RANDOM_H */
