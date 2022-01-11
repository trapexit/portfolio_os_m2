/*
 *    @(#) MPM_NewFrame.c 96/07/18 1.1
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"

/*
 * Initializes context
 *
 */

/*
 * Switches to a new frame by pointing the whichframe
 * context field to the next value
 *
 */
void MPM_NewFrame(MpmContext* mpmc)
{
    mpmc->whichframe = 1-mpmc->whichframe;
}

