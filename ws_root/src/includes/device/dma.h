#ifndef __DEVICE_DMA_H
#define __DEVICE_DMA_H


/******************************************************************************
**
**  @(#) dma.h 96/11/09 1.6
**
**  Definitions for the device driver DMA utility functions.
**
******************************************************************************/


extern Err StartDMA(uint32 dmaAddr, void *vMemAddr, uint32 len,
	uint32 flags, uint32 dchan,
	void (*Callback)(void *arg, Err err), void *callbackArg);

/* Bits in the flags arguments */
#define DMA_READ        0x00000001	/* Read from device */
#define DMA_WRITE       0x00000002	/* Write to device */
#define	DMA_SYNC	0x00000010	/* Synchronous transfer */
#define DMA_FORCE 	0x00000040	/* Immediately starts DMA, no queuing. */
#define	DMA_8BIT	0x00000100	/* 8 bit transfer */
#define	DMA_16BIT	0x00000200	/* 16 bit transfer */
#define	DMA_32BIT	0x00000400	/* 32 bit transfer */

/* Values in the dchan argument */
#define	DMA_CHANNEL_ANY	127		/* Use any DMA channel */

extern int32 AbortDMA(void (*Callback)(void *arg, Err err), void *callbackArg);

extern void InitDMA(void);

extern uint32 AllocDMAChannel(uint32 flags);
extern Err FreeDMAChannel(uint32 dchan);


#endif /* __DEVICE_DMA_H */
