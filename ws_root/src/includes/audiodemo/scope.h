#ifndef __AUDIODEMO_SCOPE_H
#define __AUDIODEMO_SCOPE_H


/******************************************************************************
**
**  @(#) scope.h 96/07/29 1.2
**
**  scope include file.
**
******************************************************************************/

/* ------------------------------ rnm Lifted from patchdemo.h */

    /* audiodemo */
#include <audiodemo/portable_graphics.h>

    /* portfolio */
#include <kernel/types.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* -------------------- Scope Stuff */

/***************************************************************
** support for capture of digital signals from DSPP
** Collect signals in a delay line for analysis or graphics display.
**************************************************************/
typedef struct ScopeProbe
{
    Item   scpr_DelayLine;
    Item   scpr_Attachment;
    Item   scpr_Cue;
    Item   scpr_Signal;
    Item   scpr_Probe;
    PortableGraphicsContext  *scpr_gc;
    int32  scpr_Size;
    volatile int16 *scpr_Data;
} ScopeProbe;

Err DeleteScopeProbe( ScopeProbe *scpr );
Err CreateScopeProbe( ScopeProbe **resultScopeProbe, int32 NumBytes, PortableGraphicsContext *scopecon);
Err CaptureScopeBuffer( ScopeProbe *scpr );
Err DisplayScopeBuffer( ScopeProbe *scpr, int32 XOffset, float32 XScalar, float32 YScalar );
Err ConnectScope( ScopeProbe *scpr, Item inst, char *portName, int32 part );
void DisconnectScope( ScopeProbe *scpr);
Err DoScope( ScopeProbe *scpr, Item inst, char* portName, int32 part );

#define LEFT_VISIBLE_EDGE (0.1)
#define TEXT_OFFSET (0.1)
#define TEXT_HEIGHT (0.1)

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif  /* __AUDIODEMO_SCOPE_H */
