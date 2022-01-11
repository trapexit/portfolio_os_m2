/******************************************************************************
**
**  @(#) windpatch.c 96/08/27 1.11
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name windpatch
|||	Creates a "wind" sound effect using the patch compiler (audiopatch folio).
|||
|||	  Format
|||
|||	    windpatch
|||
|||	  Description
|||
|||	    Creates and plays a howling wind Patch Template. This demonstates use
|||	    of the audio folio's Patch Compiler.
|||
|||	    This program runs until any control pad event is received.
|||
|||	  Associated Files
|||
|||	    windpatch.c
|||
|||	  Location
|||
|||	    Examples/Audio/SoundEffects
|||
|||	  See Also
|||
|||	    PatchCmd(@)
**/

#include <audio/audio.h>
#include <audio/patch.h>
#include <kernel/operror.h>
#include <kernel/task.h>
#include <misc/event.h>
#include <stdio.h>


/* -------------------- Local functions */

static Err DoWindSound (void);
static Item CreateWindPatch (void);


/* -------------------- Code */

/***********************************************************************/
int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argc);

    printf ("%s: start\n", argv[0]);

    if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) goto clean;
    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    errcode = DoWindSound();

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);
    CloseAudioFolio();
    KillEventUtility();
    printf ("%s: done\n", argv[0]);
    return 0;
}


/***********************************************************************/
static Err DoWindSound (void)
{
    Item windPatch;
    Item leftWindIns, rightWindIns;
    Item outputIns = -1;
    Err errcode;

    if ((errcode = windPatch = CreateWindPatch()) < 0) goto clean;
    if ((errcode = leftWindIns = CreateInstrument (windPatch, NULL)) < 0)
        goto clean;
    if ((errcode = rightWindIns = CreateInstrument (windPatch, NULL)) < 0)
        goto clean;
    if ((errcode = outputIns = LoadInstrument ("line_out.dsp", 0, 100)) < 0)
        goto clean;

        /*
            Connect 2 different wind instruments from the same template on
            different channels, because we can and it sounds spookier than just
            a single instrument connected up monophonically to both outputs.
        */
    if ((errcode = ConnectInstrumentParts (leftWindIns, "Output",  0,
        outputIns, "Input", AF_PART_LEFT)) < 0) goto clean;
    if ((errcode = ConnectInstrumentParts (rightWindIns, "Output", 0,
        outputIns, "Input", AF_PART_RIGHT)) < 0) goto clean;

    if ((errcode = StartInstrument (outputIns, NULL)) < 0) goto clean;
    if ((errcode = StartInstrument (leftWindIns, NULL)) < 0) goto clean;
    if ((errcode = StartInstrument (rightWindIns, NULL)) < 0) goto clean;

        /* wait around until button pressed */
    {
        ControlPadEventData cped;
        if ((errcode = GetControlPad (1, TRUE, &cped)) < 0) goto clean;
    }

clean:
    StopInstrument (outputIns, NULL);
    UnloadInstrument (outputIns);
    DeletePatchTemplate (windPatch);
    return errcode;
}


/***********************************************************************/
/*
    This function creates a Patch Template out of constituent templates and
    returns the result.

    The patch basically consists of a white noise generator played through
    a low pass filter. The cutoff frequency and resonance of the filter
    are controlled by two independent random signals from red noise generators
    running at very low frequency.
*/
static Item CreateWindPatch (void)
{
    const Item windNoiseTmp     = LoadInsTemplate ("noise.dsp", NULL);
    const Item windFilterTmp    = LoadInsTemplate ("svfilter.dsp", NULL);
    const Item windRedNoiseTmp  = LoadInsTemplate ("rednoise_lfo.dsp", NULL);
    const Item windTimesPlusTmp = LoadInsTemplate ("timesplus.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

        /*
            Open audiopatch folio, which we only need open long enough to
            build the patch. Do this first, and return rather than go the
            normal cleanup if it failed.
        */
    if ((result = OpenAudioPatchFolio()) < 0) return result;

        /* create PatchCmdBuilder */
    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

        /* Define knobs and ports */
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT,
        AF_SIGNAL_TYPE_GENERIC_SIGNED);

        /* Add constituent templates to patch */
    AddTemplateToPatch (pb, "noise", windNoiseTmp);
    AddTemplateToPatch (pb, "modsignal1", windRedNoiseTmp);
    AddTemplateToPatch (pb, "modsignal2", windRedNoiseTmp);
    AddTemplateToPatch (pb, "modroute1", windTimesPlusTmp);
    AddTemplateToPatch (pb, "modroute2", windTimesPlusTmp);
    AddTemplateToPatch (pb, "filter", windFilterTmp);

        /* Connect signal path: noise -> filter -> output */
    SetPatchConstant (pb, "noise", "Amplitude", 0, 0.3);
    ConnectPatchPorts (pb, "noise",  "Output", 0, "filter", "Input", 0);
    ConnectPatchPorts (pb, "filter", "Output", 0, NULL,     "Output", 0);

        /* Connect one rednoise modulation signal to filter cutoff frequency */
    ConnectPatchPorts (pb, "modsignal1", "Output", 0,
                           "modroute1", "InputA", 0);
    ConnectPatchPorts (pb, "modroute1",  "Output", 0,
                           "filter", "Frequency", 0);
    SetPatchConstant (pb, "modsignal1", "Frequency", 0, 0.5);
    SetPatchConstant (pb, "modroute1", "InputB", 0, 0.0427);
    SetPatchConstant (pb, "modroute1", "InputC", 0, 0.0854);

        /* Connect the other rednoise modulation signal to filter resonance */
    ConnectPatchPorts (pb, "modsignal2", "Output", 0,
                           "modroute2", "InputA", 0);
    ConnectPatchPorts (pb, "modroute2", "Output", 0,
                           "filter", "Resonance", 0);
    SetPatchConstant (pb, "modsignal2", "Frequency", 0, 0.3);
    SetPatchConstant (pb, "modroute2", "InputB", 0, 0.0854);
    SetPatchConstant (pb, "modroute2", "InputC", 0, 0.0916);

        /* Did all of the PatchCmd constructors work? */
    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

        /*
            Create Patch Template from PatchCmd list returned from
            PatchCmdBuilder.
        */
    result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "wind.patch",
        TAG_END);

clean:
        /* we don't need this stuff after we've built the patch */
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (windNoiseTmp);
    UnloadInsTemplate (windFilterTmp);
    UnloadInsTemplate (windRedNoiseTmp);
    UnloadInsTemplate (windTimesPlusTmp);
    CloseAudioPatchFolio();
    return result;
}
