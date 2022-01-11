
/******************************************************************************
**
**  @(#) luckie.c 96/02/21 1.8
**
******************************************************************************/

/**
|||	AUTODOC -class examples -group EventBroker -name luckie
|||	Uses the event broker to read events from the first control pad.
|||
|||	  Format
|||
|||	    luckie [anything]
|||
|||	  Description
|||
|||	    Uses the event broker to monitor and report activity for the first control
|||	    pad plugged in to the control port.
|||
|||	  Arguments
|||
|||	    [anything]
|||	        If you supply no arguments to this program,
|||	        it asks GetControlPad() to put the task to
|||	        sleep when waiting for an event. If you
|||	        supply an argument, GetControlPad() will not
|||	        put the task to sleep, and the program will
|||	        poll the control pad.
|||
|||	  Associated Files
|||
|||	    luckie.c
|||
|||	  Location
|||
|||	    Examples/EventBroker
|||
**/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <misc/event.h>
#include <stdio.h>


/*****************************************************************************/


int main(int32 argc)
{
Err                 err;
ControlPadEventData cp;

    printf("Initializing event utility\n");

    err = InitEventUtility(1, 0, LC_ISFOCUSED);
    if (err < 0)
    {
        printf("Unable to initialize the event utility: ");
        PrintfSysErr(err);
        return 0;
    }

    do
    {
        err = GetControlPad (1, argc == 1, &cp);
        if (err < 0)
        {
            printf("GetControlPad() failed: ");
            PrintfSysErr(err);
            break;
        }
        printf("Control pad 1: update %d, bits 0x%x", err, cp.cped_ButtonBits);

	if (cp.cped_AnalogValid) {
	  printf("    stick (%d,%d) shift (%d, %d) shuttle %d\n",
	     cp.cped_StickX,
	     cp.cped_StickY,
	     cp.cped_LeftShift,
	     cp.cped_RightShift,
	     cp.cped_Shuttle);
	} else {
	  printf("\n");
	}
    }
    while ((cp.cped_ButtonBits & ControlStart) == 0);

    printf("Shutting down luckie\n");

    KillEventUtility();

    return 0;
}
