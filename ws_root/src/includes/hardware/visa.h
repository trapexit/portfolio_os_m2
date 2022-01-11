#ifndef __HARDWARE_VISA_H
#define __HARDWARE_VISA_H


/******************************************************************************
**
**  @(#) visa.h 96/02/20 1.3
**
******************************************************************************/


/*
 * Header in the VISA ROM.
 * The VISA ROM is not directly addressible,
 * but is downloaded via the CDE_DEVx_VISA_CONF register.
 */
typedef struct VisaHeader
{
	uint32		visa_Config;		/* Configuration bits. */
	uint32		visa_Magic;		/* Constant to identify VISA. */
} VisaHeader;

/* Bits in the visa_Config word. */
#define	VISA_JUMPERS		0x03000000	/* Set by external pullups */
#define	VISA_EXTERNAL_ROM	0x04000000	/* An external ROM is present */
#define	VISA_VERSION		0x00FF0000	/* Mask for VISA version num */
#define	VISA_MFG		0x0000FF00	/* Mask for VISA manufacturer */
#define	VISA_ROM_SIZE		0x000000FF	/* Mask for VISA ROM size */

/* Value in the visa_Magic word. */
#define	VISA_MAGIC		0x4A554445	/* Same for all VISA chips */

#define	VISA_PCMCIA_ADDR	0x40000000	/* Magic address to tell
						   PCMCIA channel driver to
						   read VISA instead of card. */

#endif /* __HARDWARE_VISA_H */
