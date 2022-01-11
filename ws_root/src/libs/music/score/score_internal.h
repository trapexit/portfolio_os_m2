#ifndef __SCORE_INTERNAL_H
#define __SCORE_INTERNAL_H


#ifdef EXTERNAL_RELEASE
  #error This is an internal include file.
#endif


/******************************************************************************
**
**  @(#) score_internal.h 96/03/26 1.5
**  $Id: score_internal.h,v 1.2 1995/03/14 23:58:54 peabody Exp $
**
**  Score Player - internal include file.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950314 WJB  Created.
**  950314 WJB  Made DisableScoreMessages() apply to all score player messages.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio/score.h>    /* self (convenience) */
#include <kernel/types.h>
#include <stdio.h>          /* printf() */


/* -------------------- Globals */

/* !!! score_messagesDisabled should be a constant TRUE, or completely removed,
       when BUILD_STRINGS is off. */

extern bool score_messagesDisabled;


/* -------------------- Macros */

/* !!! if the compiler optimizes correctly, these would just become NOPs if
       score_messagesDisabled is a constant. Otherwise, there needs to be
       BUILD_STRINGS and non-BUILD_STRINGS variants of them. */

#define PRT(x)  { if (!score_messagesDisabled) printf x; }
#define ERR(x)  PRT(x)

#define CHECKRESULT(val,name) \
    if (val < 0) \
    { \
        Result = val; \
        TOUCH(Result); \
        if (!score_messagesDisabled) PrintError (NULL, "\\failure in", name, Result); \
        goto cleanup; \
    }


/*****************************************************************************/


#endif /* __SCORE_INTERNAL_H */
