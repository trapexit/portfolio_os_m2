/*
 *
 *
 */
#ifndef _lights_h_
#define _lights_h_

#ifdef MACINTOSH
#include <kernel:types.h>
#else
#include <kernel/types.h>
#endif

#include "mercury.h"
#include "matrix.h"
#include "lighting.h"

/*
 *
 */
typedef struct Lights
{
	uint32	*lightProcess;
	uint32	*lightData;

} Lights, *pLights;

/*
 * Prototypes
 */
Lights* Lights_Construct(void);
void Lights_Print(Lights *self);

#endif
/* End of File */
