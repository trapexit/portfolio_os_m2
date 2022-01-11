
/******************************************************************************
**
**  @(#) patchdemo.c 96/08/28 1.18
**  $Id: patchdemo.c,v 1.23 1994/11/22 23:16:59 peabody Exp $
**
**  patchdemo main module.
**
**  Related files:
**      . patchdemo.h   central .h file for patchdemo
**      . pd_patch.c    patch structure management and file parser
**      . pd_faders.c   gui stuff
**      . pd_scope.c    oscilloscope
**
******************************************************************************/

/* !!! autodoc disabled */
/**
|||	NOAUTODOC -public -class shell_commands -name patchdemo
|||	Tool to experiment with DSP patch construction.
|||
|||	  Format
|||
|||	    patchdemo [-verbose] [-noscope] <patch file name>
|||
|||	  Description
|||
|||	    This program interprets a text file containing a description of a patch
|||	    made out of DSP instruments, samples, delay lines, etc. It then provides you
|||	    with a set of faders for adjusting knobs in the patch's instruments. The patch
|||	    language is a very simple command oriented language that closely resembles
|||	    audio folio function names. Therefore, patchdemo can be used to quickly construct
|||	    DSP patches that you later plan to implement in C. Also, with only a few
|||	    restrictions, patchdemo patches can be imported directly into ARIA.
|||
|||	    patchdemo also has a built-in oscilloscope for capturing and viewing output
|||	    audio signal.
|||
|||	  Arguments
|||
|||	    -verbose
|||	        When present causes a ton of potentially useful information
|||	        about the loaded patch to be displayed at the debugger console.
|||
|||	    -noscope
|||	        When present disables the oscilloscope. !!! This is only
|||	        useful with M2 DSPP simulator, which takes a rediculously long
|||	        time to fill in the delay line used by the scope.
|||
|||	    <patch file name>
|||	        Name of patch file to load.
|||
|||	  Controls -enumerated
|||
|||	    Faders Mode:
|||
|||	    A
|||	        Toggle between Start and Release of instrument.
|||
|||	    LShift + A
|||	        Stop instrument.
|||
|||	    B
|||	        Start instrument when pressed, Release instrument when released (like a
|||	        key on a MIDI keyboard).
|||
|||	    C
|||	        Display faders for next instrument in patch.
|||
|||	    LShift + C
|||	        Display faders for previous instrument in patch.
|||
|||	    X
|||	        Quit
|||
|||	    Up or Down
|||	        Select a fader by moving up or down.
|||
|||	    Left or Right
|||	        Adjust current fader (coarse).
|||
|||	    LShift + Left or Right
|||	        Adjust current fader (fine).
|||
|||	    RShift
|||	        Switch to scope mode.
|||
|||	    Scope Mode:
|||
|||	    A
|||	        Acquire signal as long as A button is held down. Hold last acquired
|||	        signal when released.
|||
|||	    C
|||	        Switch to faders mode.
|||
|||	    X
|||	        Quit - !!! doesn't work properly
|||
|||	    Up or Down
|||	        Increase or decrease vertical scaling.
|||
|||	    Left or Right
|||	        Pan left or right.
|||
|||	    LShift or RShift
|||	        Decrease or increase horizontal scaling.
|||
|||	  Patch Language -enumerated
|||
|||	    !!! still needs more detail
|||
|||	    ; <comment>
|||
|||	    # <comment>
|||
|||	    AttachSample <attachment symbol> <instrument> <sample> [<fifo name> [<start frame>]]
|||
|||	    Connect <source instrument> <source output> <dest instrument> <dest input>
|||
|||	    DelayLine <sample symbol> <nbytes> [<nchannels> [<loop (1/0)>]]
|||
|||	    LoadInstrument <instrument symbol> <instrument file name> [<triggerable (1/0)>]
|||
|||	    LoadSample <sample symbol> <sample file name>
|||
|||	    Tweak <instrument> <knob> <value>
|||
|||	  Implementation
|||
|||	    Command implemented in V24.
|||
|||	  Location
|||
|||	    $c/patchdemo
|||
|||	  See Also
|||
|||	    3DO cookbook for Sound Designers - more complete documentation of patchdemo
|||	    features, operation, and language.
|||
|||	    Examples/Audio/PatchDemo - examples patches.
|||
|||	    ARIA - tool to build custom DSP instruments.
|||
|||	    dspfaders - try out a single DSP instrument.
|||
**/

    /* local */
#include "patchdemo.h"

#ifndef PATCHDEMO_NO_GUI
    /* audiodemo */
#include <audiodemo/faders.h>           /* DriveFaders() */
#include <audiodemo/graphic_tools.h>    /* InitGraphics(), SwitchScreens() */
#endif

    /* portfolio */
#include <audio/audio.h>    /* Open/CloseAudioFolio() */
#include <kernel/operror.h> /* PrintError() */
#include <misc/event.h>     /* Init/KillEventUtility(), GetControlPadEvent() */
#include <stdio.h>          /* printf() */
#include <string.h>


/* -------------------- Debugging */

#define DEBUG_Mem           0   /* availmem test */


/* -------------------- local functions */

static Err DemoPatch (Patch *, bool enabledscope);
static void DumpPatch (const Patch *);


/* -------------------- main() */

int main (int argc, char *argv[])
{
    const char *patchname = NULL;
    Patch *patch = NULL;
    Err errcode = 0;
    bool verbose = FALSE;
    bool enablescope = TRUE;

  #if DEBUG_Mem
    printf ("main() entry: "); printavail(); printf ("\n");
  #endif

  #ifdef PATCHDEMO_NO_GUI
    printf ("patchdemo w/o GUI\n");
  #endif

        /* parse args */
    if (argc < 2) {
        printf ("usage: %s [-verbose] [-noscope] <patch file>\n", argv[0]);
        goto fail;
    }

    {
        int i;

        for (i=1; i<argc; i++) {
            if (argv[i][0] == '-') {
                if (!strcasecmp (argv[i], "-verbose")) verbose = TRUE;
                else if (!strcasecmp (argv[i], "-noscope")) enablescope = FALSE;
                else {
                    printf ("%s: unknown switch: %s\n", argv[0], argv[i]);
                    goto fail;
                }
            }
            else {
                if (!patchname) {
                    patchname = argv[i];
                }
                else {
                    printf ("%s: too many arguments\n", argv[0]);
                    goto fail;
                }
            }
        }
    }

    if (!patchname) {
        printf ("%s: no patch specified\n", argv[0]);
        goto fail;
    }

        /* open folios */
    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open", "AudioFolio", errcode);
        goto fail;
    }
#if 1
    if ((errcode = EnableAudioInput (TRUE, NULL)) < 0) {
        PrintError (NULL, "enable", "audio input", errcode);
        goto fail;
    }
#else
    printf("EnableAudioInput() disabled.\n");
#endif
  #ifndef PATCHDEMO_NO_GUI
    if ((errcode = InitGraphics(2)) < 0) {
        PrintError (NULL, NULL, "InitGraphics()", errcode);
        goto fail;
    }
  #endif
    if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) {
        PrintError (NULL, NULL, "InitEventUtility()", errcode);
        goto fail;
    }

        /* load patch */
    printf ("Loading patch...");
    if ((patch = LoadPatch (patchname, &errcode)) == NULL) {
        printf ("\n");
        PrintError (NULL, "load patch", patchname, errcode);
        goto clean;
    }

    if (verbose) {
        printf ("\n");
        DumpPatch (patch);
    }

    printf ("ready.\n");

        /* demo patch */
    if ((errcode = DemoPatch (patch, enablescope)) < 0) {
        PrintError (NULL, "demo patch", patchname, errcode);
        goto clean;
    }

clean:
    UnloadPatch (patch);

fail:
    KillEventUtility();
  #ifndef PATCHDEMO_NO_GUI
    TermGraphics();         /* !!! this function doesn't seem to clean up everything done by InitGraphics() */
  #endif
    CloseAudioFolio();

  #if DEBUG_Mem
    printf ("main() exit: "); printavail(); printf ("\n");
  #endif

    printf ("%s finished.\n", argv[0]);
    return 0;
}


/* -------------------- DemoPatch() */

#ifndef PATCHDEMO_NO_GUI
static Err procevents (PatchPageList *, ScopeProbe *);
#else
static Err procevents (const Patch *);
#endif
#define triggerpatch(patch,noteon) ( ( (noteon) != 0 ? StartPatch : ReleasePatch ) (patch, NULL) )

    /* !!! publish? */
#define NextLoopNode(l,n) (IsNode((l),NextNode(n)) ? NextNode(n) : FirstNode(l))
#define PrevLoopNode(l,n) (IsNodeB((l),PrevNode(n)) ? PrevNode(n) : LastNode(l))


/*
    Create and display PatchPageList for Patch.
*/
#ifndef PATCHDEMO_NO_GUI    /* { */

static Err DemoPatch (Patch *patch, bool enablescope)
{
    PatchPageList *pagelist = NULL;
    ScopeProbe *scpr = NULL;
    Err errcode = 0;

        /* create PatchPageList for Patch */
    if ((pagelist = NewPatchPageList (patch, &errcode)) == NULL) goto clean;

        /* trap no pages */
    if (IsEmptyList (&pagelist->ppage_PageList)) {
        PrintError (NULL, "\\no instruments to edit", NULL, 0);        /* !!! more graceful error message here */
        errcode = -1;       /* !!! real code */
        goto clean;
    }

        /* create scope probe */
    if (enablescope) {
        if ((errcode = CreateScopeProbe (&scpr, SCOPE_MAX_SAMPLES*2)) < 0) goto clean;
    }

        /* process events */
    errcode = procevents (pagelist, scpr);

clean:
    DeleteScopeProbe (scpr);
    DeletePatchPageList (pagelist);
    return errcode;
}

#else

static Err DemoPatch (Patch *patch, bool enablescope)
{
    return procevents (patch);
}

#endif  /* } */

/*
    process events for PatchPageList

    controls:
        X                   quit
        A                   toggle instrument start/release
        B                   start on down, release on up
        C                   select next PatchPage
        LShift C            select previous PatchPage
        Up/Down             select fader
        Left/Right          adjust fader value quickly
        LShift Left/Right   adjust fader value slowly
        RShift              patches/scope/audiomon
*/
#ifndef PATCHDEMO_NO_GUI
static Err procevents (PatchPageList *pagelist, ScopeProbe *scpr)
#else
static Err procevents (const Patch *patch)
#endif
{
    ControlPadEventData cur_cped, last_cped;
    Boolean noteon = FALSE;
  #ifndef PATCHDEMO_NO_GUI
    PatchPage *page = (PatchPage *)FirstNode (&pagelist->ppage_PageList);       /* empty patch is trapped in DemoPatch() */
    const Patch * const patch = pagelist->ppage_Patch;
  #endif
    Err errcode = 0;

  #ifndef PATCHDEMO_NO_GUI
        /* show first page */
    if ((errcode = ShowPatchPage (page)) < 0) goto clean;
  #endif

        /* get initial state of controller */
    if ((errcode = GetControlPad (1, FALSE, &cur_cped)) < 0) goto clean;
    last_cped = cur_cped;

        /* start instrument */
    if ((errcode = triggerpatch (patch, noteon = TRUE)) < 0) goto clean;

        /* process controller events */
    for (;;) {
            /* get a button event, build changed buttons mask */
        if ((errcode = GetControlPad (1, FALSE, &cur_cped)) < 0) goto clean;

            /* demultiplex button state changes */
        {
            const uint32 changedbuttons = last_cped.cped_ButtonBits ^ cur_cped.cped_ButtonBits;
            uint32 buttonmask;

          #ifndef PATCHDEMO_NO_GUI
                /* process fader events */
            DriveFaders (&page->ppage_FaderBlock, cur_cped.cped_ButtonBits);
          #endif

                /* !!! this is about as silly a way to do this as you can imagine! use edge detection stuff instead! */
            if (changedbuttons) for (buttonmask = 0x80000000; buttonmask; buttonmask >>= 1) if (changedbuttons & buttonmask) {
                const Boolean buttondown = (cur_cped.cped_ButtonBits & buttonmask) != 0;

                switch (buttonmask) {
                    case ControlA:      /* A: toggle start/release instrument; LShift+A: stop instrument */
                        if (buttondown) {
                            if (cur_cped.cped_ButtonBits & ControlLeftShift) {
                                noteon = FALSE;
                                if ((errcode = StopPatch (patch, NULL)) < 0) goto clean;
                            }
                            else {
                                if ((errcode = triggerpatch (patch, noteon = !noteon)) < 0) goto clean;
                            }
                        }
                        break;

                    case ControlB:      /* B: start/release instrument based on B button state. */
                        if ((errcode = triggerpatch (patch, noteon = buttondown)) < 0) goto clean;
                        break;

                  #ifndef PATCHDEMO_NO_GUI
                    case ControlC:      /* C: next instrument page; LShift+C: prev instrument page */
                        if (buttondown) {
                            page = (PatchPage *)( (cur_cped.cped_ButtonBits & ControlLeftShift)
                                                  ? PrevLoopNode (&pagelist->ppage_PageList, page)
                                                  : NextLoopNode (&pagelist->ppage_PageList, page) );

                            if ((errcode = ShowPatchPage (page)) < 0) goto clean;
                        }
                        break;
                  #endif

                    case ControlX:      /* Stop (X): quit */
                        if (buttondown) goto clean;

                  #ifndef PATCHDEMO_NO_GUI
                    case ControlRightShift:   /* Switch to scope display. 940715 */
                        if (scpr && buttondown) {
                            if ((errcode = DoScope( scpr, page )) < 0) goto clean;
                            if ((errcode = ShowPatchPage (page)) < 0) goto clean;
                        }
                        break;
                  #endif
                }
            }
        }

            /* store last event */
        last_cped = cur_cped;

      #ifndef PATCHDEMO_NO_GUI
            /* switch screen buffers (also waits a vblank) */
        SwitchScreens();
      #endif
    }

clean:
    StopPatch (patch, NULL);
    return errcode;
}


/* -------------------- DumpPatch() */

static void DumpPatch (const Patch *patch)
{
    printf ("Patch: 0x%08lx\n", patch);

    if (patch) {
            /* display instruments */
        {
            PatchInstrument *inst;

            for (inst = (PatchInstrument *)FirstNode (&patch->patch_InstrumentList); IsNode(&patch->patch_InstrumentList,inst); inst = (PatchInstrument *)NextNode (inst)) {
                printf ("  Instrument \"%s\": template=\"%s\" item=0x%05lx attr=0x%02x knobtbl=0x%08lx nknobs=%ld\n", inst->pinst_Symbol.psym_Node.n_Name, inst->pinst_TemplateName, inst->pinst_Instrument, inst->pinst_AttrFlags, inst->pinst_KnobTable, inst->pinst_NKnobs);

                {
                    int32 nknobs = inst->pinst_NKnobs;
                    PatchKnob *knob = inst->pinst_KnobTable;

                    for (; nknobs--; knob++) {
                        printf ("    Knob \"%s\": ", knob->pknob_Name);

                        if (IsPatchKnobGrabbed (knob)) {
                            TagArg getknobtags[] = {    /* @@@ order is assumed below */
                                { AF_TAG_CURRENT },
                                { TAG_END }
                            };

                            GetAudioItemInfo (knob->pknob_Knob, getknobtags);

                            printf ("grabbed val=%ld\n", (int32)getknobtags[0].ta_Arg);
                        }
                        else {
                            printf ("connected\n");
                        }
                    }
                }
            }
        }

            /* display samples */
        {
            PatchSample *samp;

            for (samp = (PatchSample *)FirstNode (&patch->patch_SampleList); IsNode(&patch->patch_SampleList,samp); samp = (PatchSample *)NextNode (samp)) {
                TagArg tags[] = {           /* @@@ order assumed below */
                    { AF_TAG_FRAMES },
                    { AF_TAG_CHANNELS },
                    { AF_TAG_WIDTH },
                    TAG_END
                };

                printf ("  Sample \"%s\":", samp->psamp_Symbol.psym_Node.n_Name);
                if (samp->psamp_FileName)
                    printf (" file=\"%s\"", samp->psamp_FileName);
                printf (" item=0x%05lx", samp->psamp_Sample);
                if (!GetAudioItemInfo (samp->psamp_Sample, tags))
                    printf (" nframes=%lu nchannels=%lu width=%lu", (uint32)tags[0].ta_Arg, (uint32)tags[1].ta_Arg, (uint32)tags[2].ta_Arg);
                printf ("\n");
            }
        }

            /* display attachments */
        {
            PatchSampleAttachment *att;

            for (att = (PatchSampleAttachment *)FirstNode (&patch->patch_SampleAttachmentList); IsNode(&patch->patch_SampleAttachmentList,att); att = (PatchSampleAttachment *)NextNode (att)) {
                printf ("  Attachment \"%s\": item=0x%05lx\n", att->psatt_Symbol.psym_Node.n_Name, att->psatt_Attachment);
            }
        }
    }
}
