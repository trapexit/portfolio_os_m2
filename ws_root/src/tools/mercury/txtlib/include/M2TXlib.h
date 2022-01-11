/*
	File:		M2TXlib.h

	Contains:	Prototypes for M2 Texture mapping library 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<4+>	  8/4/95	TMA		Added include for M2TXDither functions.
		 <4>	 7/11/95	TMA		Added M2TXiff.h header file to be included with the general
									headers.
		 <2>	 5/30/95	TMA		Added M2TXQuant.h.
		 <3>	 1/20/95	TMA		Massive cleanup of all headers
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/
/* All the necessary data types */
#include "M2TXTypes.h"

/* Handles all the file i/o */
#include  "M2TXio.h"

/* Handles all the IFF file i/o */
#include  "M2TXiff.h"

/* Accessors to the Header */
#include "M2TXHeader.h"

/* Accessors to DCI */
#include "M2TXDCI.h"

/* Compression calls */
#include "M2TXcompress.h"

/* acessor to the M2TX structure, Get functions return a pointer to the existing structure
 Set functions COPY the information (including pointers) from the chunk to the field in M2TX
*/
/* Accessors to PIP */
/* Color functions */
/* Raw Calls */
/* Index Calls  */
/* Convenience Calls */

#include "M2TXLibrary.h"

/* Functions for dealing with texFormat data structure */
#include "M2TXFormat.h"

/* Functions for dealing with Color Quantization */
#include "M2TXQuant.h"

/* Functions for dealing with Dithering */
#include "M2TXDither.h"

