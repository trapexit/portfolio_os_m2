#ifndef __AUDIO_DEBUG_H
#define __AUDIO_DEBUG_H


/******************************************************************************
**
**  @(#) audio_debug.h 95/09/19 1.1
**
**  Audio Item structure dump inline functions.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950919 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#ifndef __AUDIO_STRUCTS_H
#include "audio_internal.h"
#endif

#ifndef __STDIO_H
#include "stdio.h"
#endif


#ifdef BUILD_STRINGS    /* { */

/* -------------------- DumpInsTemplate() */

static void DumpInsTemplate (const AudioInsTemplate *aitp, const char *banner)
{
    if (banner) printf ("%s: ", banner);
    printf ("AudioInsTemplate item 0x%x @ 0x%x\n", aitp->aitp_Item.n_Item, aitp);

    printf ("  n_Name                     '%s'\n", aitp->aitp_Item.n_Name);
    {
        Item const owner = aitp->aitp_Item.n_Owner;
        const ItemNode * const t = (ItemNode *)LookupItem (owner);
        char ownerStr[80];

        if (owner == 0)
        {
            strcpy(ownerStr,"'kernel'");
        }
        else if (t == 0)
        {
            strcpy(ownerStr,"<unknown>");
        }
        else
        {
            sprintf(ownerStr,"'%s'",t->n_Name);
        }

        printf ("  n_Owner                    0x%x (%s)\n", owner, ownerStr);
    }
    printf ("  n_OpenCount                %d\n", aitp->aitp_Item.n_OpenCount);
    printf ("  aitp_DeviceTemplate        0x%08x (function id = %d)\n", aitp->aitp_DeviceTemplate, ((DSPPTemplate *)aitp->aitp_DeviceTemplate)->dtmp_Header.dhdr_FunctionID);
    if (!IsListEmpty (&aitp->aitp_InstrumentList)) {
        ItemNode *n;

        printf ("  aitp_InstrumentList:\n   ");
        SCANLIST (&aitp->aitp_InstrumentList, n, ItemNode) {
            printf (" 0x%x", n->n_Item);
        }
        printf ("\n");
    }
    if (!IsListEmpty (&aitp->aitp_Attachments)) {
        ItemNode *n;

        printf ("  aitp_Attachments:\n   ");
        SCANLIST (&aitp->aitp_Attachments, n, ItemNode) {
            printf (" 0x%x", n->n_Item);
        }
        printf ("\n");
    }
    printf ("  aitp_Tuning                0x%x\n", aitp->aitp_Tuning);
    printf ("  aitp_DynamicLinkTemplates  0x%08x\n", aitp->aitp_DynamicLinkTemplates);
    if (aitp->aitp_DynamicLinkTemplates) {
        const Item *dlnktmpl;

        printf ("   ");
        for (dlnktmpl = aitp->aitp_DynamicLinkTemplates; *dlnktmpl > 0; dlnktmpl++) {
            printf (" (0x%x)", *dlnktmpl);
        }
        printf ("\n");
    }
}

#endif  /* } BUILD_STRINGS */


/*****************************************************************************/

#endif /* __AUDIO_DEBUG_H */
