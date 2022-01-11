
/******************************************************************************
**
**  @(#) audiomon.c 96/04/25 1.9
**
******************************************************************************/

/**
|||	AUTODOC -public -class Shell_Commands -group Audio -name audiomon
|||	Graphically displays audio resource usage.
|||
|||	  Format
|||
|||	    audiomon
|||
|||	  Description
|||
|||	    Graphically displays how audio folio (DSP) resources are currently being
|||	    used by the executing programs. This allows programmers to see the impact of
|||	    instruments, score playing, streaming, etc, on the audio system. Execute
|||	    this program while your audio software is playing to see its impact on the
|||	    system.
|||
|||	    The following resources are displayed:
|||
|||	    Ticks (batch)
|||	        DSP ticks per batch (as opposed to ticks per frame).
|||
|||	    Code Mem
|||	        Words of DSP code memory.
|||
|||	    Data Mem
|||	        Words of DSP data memory.
|||
|||	    FIFOs
|||	        DSP FIFOs (both input and output).
|||
|||	    Triggers
|||	        DSP Triggers.
|||
|||	    Each resource is displayed as a bar graph with a numerical readout to the
|||	    right of the bar graph. Ticks, FIFOs, and Triggers are each displayed as a
|||	    two-segment bar graph:
|||
|||	  -preformatted
|||
|||	    Type  [--yellow/red--|-----------blue----------]  In-Use : Free / Total
|||
|||	          |              |                         |
|||	          |<-- In-Use -->|<--------- Free -------->|
|||	          |                                        |
|||	          |<---------------- Total --------------->|
|||
|||	  -normal_format
|||
|||	    The In-Use region is displayed in yellow when it is less than 90% of the
|||	    total resource. It is displayed in red otherwise.
|||
|||	    Code and Data Memory are each displayed as a three-segment bar graph:
|||
|||	  -preformatted
|||
|||	    Type  [--yellow/red--|--cyan--|--blue/magenta--]  In-Use : Free (Largest) / Total
|||
|||	          |              |        |                |
|||	          |              |        |<-- Largest --->|
|||	          |              |                         |
|||	          |<-- In-Use -->|<-------- Free --------->|
|||	          |                                        |
|||	          |<--------------- Total ---------------->|
|||
|||	  -normal_format
|||
|||	    The In-Use region is displayed in yellow when it is less than 90% of the
|||	    total resource. It is displayed in red otherwise. The Largest region is
|||	    normally displayed in blue. If the largest free span is less than 33% of
|||	    the total free memory, this region is displayed in magenta instead.
|||
|||	  Implementation
|||
|||	    Command implemented in V31.
|||
|||	  Location
|||
|||	    System.m2/Programs/audiomon
|||
|||	  Caveats
|||
|||	    Note that there's no built-in way to quit this program. It must be killed
|||	    from the shell.
|||
|||	  See Also
|||
|||	    audioavail(@), GetAudioResourceInfo()
**/

#include <audio/audio.h>
#include <audiodemo/portable_graphics.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <stdio.h>
#include <string.h>


#define DEBUG_RefreshCount  0       /* display number of screen refreshes */
#define DEBUG_SafeArea      0       /* make safe area visible */


/* -------------------- Misc Defines */

#define REFRESH_DELAY   0.25        /* seconds between refreshes */

    /* Local error code builder */
#define MakeUErr(svr,class,err) MakeErr(ER_USER,0,svr,ER_E_USER,class,err)


/* -------------------- Layout */

    /* visible display area */
#define VISIBLE_XMIN    0.07
#define VISIBLE_YMIN    0.10
#define VISIBLE_XMAX    0.93
#define VISIBLE_YMAX    0.90

    /* bar area */
#define BAR_XMIN        (VISIBLE_XMIN + 0.160)
#define BAR_XMAX        (VISIBLE_XMAX - 0.270)
#define BAR_YMIN        (VISIBLE_YMIN + 0.075)
#define BAR_YMAX        VISIBLE_YMAX

    /* bar dimensions */
#define BAR_HEIGHT      0.025
#define BAR_WIDTH       (BAR_XMAX-BAR_XMIN)
#define BAR_SPACING     0.04

    /* labels */
#define BAR_LEFT_LABEL_XMIN  VISIBLE_XMIN
#define BAR_RIGHT_LABEL_XMIN (BAR_XMAX+0.015)
#define BAR_RIGHT_LABEL_XMAX VISIBLE_XMAX


/* -------------------- Code */

static Err AudioMonitor (void);
static void InitDisplay (PortableGraphicsContext *);
static void DispRsrcBar (PortableGraphicsContext *, int32 rsrcType, const AudioResourceInfo *);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argc);

    printf ("%s V%d\n", argv[0], PORTFOLIO_OS_VERSION);

        /* open audio folio */
    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
        goto clean;
    }

    if ((errcode = AudioMonitor()) < 0) {
        PrintError (NULL, NULL, NULL, errcode);
        goto clean;
    }

clean:
    CloseAudioFolio();
    return (int)errcode;
}

static Err AudioMonitor (void)
{
    PortableGraphicsContext *pgCon = NULL;
    int32 timersig = 0;
    Item timerio = -1;
    Err errcode;

        /* set up display */
    if ((errcode = pgCreateDisplay (&pgCon, 2)) < 0) goto clean;
    pgClearDisplay (pgCon, 0, 0, 0);

        /* set up metronome stuff */
    {
        const int32 result = AllocSignal (0);

        if (result <= 0) {
            errcode = result ? result : MakeUErr (ER_SEVERE, ER_C_STND, ER_NoSignals);
            goto clean;
        }
        timersig = result;
    }
    if ((errcode = timerio = CreateTimerIOReq()) < 0) goto clean;
    if ((errcode = StartMetronome (timerio, (int32)REFRESH_DELAY, (int32)(REFRESH_DELAY*1000000) % 1000000, timersig)) < 0) goto clean;

        /* display loop - never ends */
    {
        AudioResourceInfo lastRsrcInfo [AF_RESOURCE_TYPE_MANY];
        bool refresh = TRUE;
        int32 rsrcType;

        memset (lastRsrcInfo, 0, sizeof lastRsrcInfo);

        for (;;) {

                /* get resource info for all resources - set refresh if anything has changed */
            for (rsrcType=0; rsrcType<AF_RESOURCE_TYPE_MANY; rsrcType++) {
                AudioResourceInfo * const lastrinfo = &lastRsrcInfo[rsrcType];
                AudioResourceInfo rinfo;

                if (GetAudioResourceInfo (&rinfo, sizeof rinfo, rsrcType) >= 0) {
                    if (memcmp (&rinfo, lastrinfo, sizeof rinfo)) {
                        *lastrinfo = rinfo;
                        refresh = TRUE;
                    }
                }
            }

                /* if refresh is set, redraw screen, then clear refresh flag */
            if (refresh) {
                InitDisplay (pgCon);

                for (rsrcType=0; rsrcType<AF_RESOURCE_TYPE_MANY; rsrcType++) {
                    DispRsrcBar (pgCon, rsrcType, &lastRsrcInfo[rsrcType]);
                }

                pgSwitchScreens (pgCon);
                refresh = FALSE;
            }

            WaitSignal (timersig);
        }
    }

clean:
    if (timerio >= 0) {
        StopMetronome (timerio);
        DeleteTimerIOReq (timerio);
    }
    FreeSignal (timersig);
    pgDeleteDisplay (&pgCon);
    return errcode;
}

static void InitDisplay (PortableGraphicsContext *pgCon)
{
    pgClearDisplay (pgCon, 0.0, 0.0, 0.0);

  #if DEBUG_SafeArea
    pgSetColor (pgCon, 0.1, 0.1, 0.1);
    pgDrawRect (pgCon, VISIBLE_XMIN, VISIBLE_YMIN, VISIBLE_XMAX, VISIBLE_YMAX);
  #endif

    {
        char b[64];

        sprintf (b, "Audio Monitor V%d\n", PORTFOLIO_OS_VERSION);
        pgMoveTo (pgCon, VISIBLE_XMIN, VISIBLE_YMIN);
        pgSetColor (pgCon, 1.0, 1.0, 1.0);
        pgDrawText (pgCon, b);
    }

  #if DEBUG_RefreshCount
    {
        static int32 hits;
        char b[64];

        hits++;
        sprintf (b, "%d update(s)\n", hits);
        pgMoveTo (pgCon, BAR_RIGHT_LABEL_XMIN, VISIBLE_YMIN);
        pgDrawText (pgCon, b);
    }
  #endif
}

typedef struct {
    float32 r,g,b;
} AMColor;

#define SetAMColor(pgCon,amcolor) pgSetColor ((pgCon), (amcolor).r, (amcolor).g, (amcolor).b)
#define DrawBarPart(pgCon,top,amcolor,minFrac,maxFrac) \
    do { \
        SetAMColor((pgCon),(amcolor)); \
        pgDrawRect ((pgCon), \
            BAR_XMIN + (float32)(minFrac) * BAR_WIDTH, (top), \
            BAR_XMIN + (float32)(maxFrac) * BAR_WIDTH, (top)+BAR_HEIGHT); \
    } while(0)

static void DispRsrcBar (PortableGraphicsContext *pgCon, int32 rsrcType, const AudioResourceInfo *rinfo)
{
    static const AMColor
        bcolBackfill    = { 0.0, 0.0, 0.5 },    /* blue */
        bcolNormalUsed  = { 0.5, 0.5, 0.0 },    /* yellow */
        bcolWarnUsed    = { 0.5, 0.0, 0.0 },    /* red */
        bcolFrag        = { 0.0, 0.5, 0.5 },    /* cyan */
        bcolWarnLargest = { 0.5, 0.0, 0.5 },    /* magenta */
        bcolNormalText  = { 1.0, 1.0, 1.0 },    /* white */
        bcolWarnText    = { 0.9, 0.3, 0.2 };    /* pink */

    static const char * const rsrcDesc[AF_RESOURCE_TYPE_MANY] = {   /* @@@ depends on AF_RESOURCE_TYPE_ order */
        "Ticks (batch)",
        "Code Mem",
        "Data Mem",
        "FIFOs",
        "Triggers",
    };
    const float32 barTop = BAR_YMIN + (rsrcType * BAR_SPACING);
    bool warnAvail = FALSE;
    bool warnFrag = FALSE;

        /* print resource type */
    pgMoveTo (pgCon, BAR_LEFT_LABEL_XMIN, barTop);
    SetAMColor (pgCon, bcolNormalText);
    pgDrawText (pgCon, rsrcDesc[rsrcType]);

        /* draw blue backdrop for bar */
    DrawBarPart (pgCon, barTop, bcolBackfill, 0.0, 1.0);

    if (rinfo->rinfo_Total) {
        const float32 freeFrac    = (float32)rinfo->rinfo_Free            / rinfo->rinfo_Total;
        const float32 largestFrac = (float32)rinfo->rinfo_LargestFreeSpan / rinfo->rinfo_Total;

            /* draw used region - warn if <=10% free */
        {
            AMColor bcolUsed;

            if (freeFrac <= 0.10) {
                bcolUsed = bcolWarnUsed;
                warnAvail = TRUE;
            }
            else {
                bcolUsed = bcolNormalUsed;
            }
            DrawBarPart (pgCon, barTop, bcolUsed, 0.0, 1.0 - freeFrac);
        }

        if (rinfo->rinfo_LargestFreeSpan) {
                /* draw fragmentation region (difference between total free and largest free) in cyan */
            DrawBarPart (pgCon, barTop, bcolFrag, 1.0 - freeFrac, 1.0 - largestFrac);

                /* draw largest free region in magenta if <=33% of free */
            if (largestFrac <= 0.33 * freeFrac) {
                DrawBarPart (pgCon, barTop, bcolWarnLargest, 1.0 - largestFrac, 1.0);
                warnFrag = TRUE;
            }
        }
    }

        /* print amount used */
    {
        const AMColor textColor = warnAvail ? bcolWarnText : bcolNormalText;
        char b[32];

        pgMoveTo (pgCon, BAR_RIGHT_LABEL_XMIN, barTop);

        sprintf (b, "%d : %d", rinfo->rinfo_Total - rinfo->rinfo_Free, rinfo->rinfo_Free);
        SetAMColor (pgCon, textColor);
        pgDrawText (pgCon, b);

        if (rinfo->rinfo_LargestFreeSpan) {
            sprintf (b, " (%d)", rinfo->rinfo_LargestFreeSpan);
            SetAMColor (pgCon, warnFrag ? bcolWarnText : textColor);
            pgDrawText (pgCon, b);
        }

        sprintf (b, " / %d", rinfo->rinfo_Total);
        SetAMColor (pgCon, textColor);
        pgDrawText (pgCon, b);
    }
}
