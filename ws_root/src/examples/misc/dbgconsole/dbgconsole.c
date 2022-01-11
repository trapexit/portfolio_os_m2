
/******************************************************************************
**
**  @(#) dbgconsole.c 96/03/12 1.1
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -name DebugConsole
|||	Demonstrates use of the debugging console.
|||
|||	  Synopsis
|||
|||	    debugconsole
|||
|||	  Description
|||
|||	    Simple program demonstrating how to use the debugging console,
|||	    which provides a simple and efficient way to perform somewhat
|||	    real-time debugging output.
|||
|||	  Associated Files
|||
|||	    debugconsole.c
|||
|||	  Location
|||
|||	    examples/Miscellaneous/DebugConsole
|||
**/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/time.h>
#include <misc/debugconsole.h>
#include <stdio.h>


/*****************************************************************************/


int main(void)
{
Err    result;
Item   timerIO;
uint32 i;

    timerIO = result = CreateTimerIOReq();
    if (timerIO >= 0)
    {
        result = CreateDebugConsoleVA(DEBUGCONSOLE_TAG_HEIGHT, 480,
                                      DEBUGCONSOLE_TAG_TOP,    0,
                                      TAG_END);
        if (result >= 0)
        {
            DebugConsolePrintf("This is a test!\n");
            DebugConsolePrintf("Let's see if this thing %s\n","works");
            DebugConsolePrintf("\n\nHello\nWorld\n");
            WaitTime(timerIO, 5, 0);

            DebugConsoleClear();
            for (i = 0; i < 30; i++)
                DebugConsolePrintf("Upto %u\n",i);
            WaitTime(timerIO, 5, 0);

            DeleteDebugConsole();
        }
        else
        {
            printf("Unable to create debugging console: ");
            PrintfSysErr(result);
        }
    }
    else
    {
        printf("Unable to create timer IOReq: ");
        PrintfSysErr(result);
    }

    return result;
}
