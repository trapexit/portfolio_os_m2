
/******************************************************************************
**
**  @(#) maus.c 96/02/21 1.8
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group EventBroker -name maus
|||	Uses the event broker to read events from the first mouse.
|||
|||	  Format
|||
|||	    maus [anything]
|||
|||	  Description
|||
|||	    Uses the event broker to monitor and report activity for the first mouse
|||	    plugged in to the control port.
|||
|||	  Arguments
|||
|||	    [anything]
|||	        If you supply no arguments to this program,
|||	        it asks GetMouse() to put the task to sleep
|||	        when waiting for an event. If you supply an
|||	        argument, GetMouse() will not put the task
|||	        to sleep, and the program will poll the
|||	        mouse.
|||
|||	  Associated Files
|||
|||	    maus.c
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
Err            err;
MouseEventData mouse;

    printf("Initializing event utility\n");

    err = InitEventUtility(1, 1, LC_ISFOCUSED);
    if (err < 0)
    {
        printf("Unable to initialize the event utility: ");
        PrintfSysErr(err);
        return 0;
    }

    do
    {
        err = GetMouse(1, argc == 1, &mouse);
        if (err < 0)
        {
            printf("GetMouse() failed: ");
            PrintfSysErr(err);
            break;
        }

        printf("Mouse 0x%x at (%d,%d)\n", mouse.med_ButtonBits,
               mouse.med_HorizPosition, mouse.med_VertPosition);
    }
    while (mouse.med_ButtonBits != MouseLeft+MouseMiddle+MouseRight);

    printf("Shutting down maus\n");

    KillEventUtility();

    return 0;
}
