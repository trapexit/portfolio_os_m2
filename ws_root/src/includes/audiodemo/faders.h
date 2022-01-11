#ifndef __AUDIODEMO_FADERS_H
#define __AUDIODEMO_FADERS_H


/******************************************************************************
**
**  @(#) faders.h 96/01/18 1.10
**  $Id: faders.h,v 1.9 1994/10/06 18:43:49 peabody Exp $
**
**  audiodemo.lib faders
**
******************************************************************************/


#include <kernel/types.h>
#include <audiodemo/portable_graphics.h>


/* -------------------- Fader structures */

typedef struct Fader
{
	float32 fdr_XMin;
	float32 fdr_XMax;
	float32 fdr_YMin;
	float32 fdr_YMax;
	float32 fdr_VMin;         /* Value min and max. */
	float32 fdr_VMax;
	float32 fdr_Value;
	float32 fdr_Increment;
	int32   fdr_Highlight;
	const char   *fdr_Name;
/* user data - for use by client's event function */
	Item    fdr_UserItem;
	void   *fdr_UserData;
} Fader;


    /* forward references for FaderBlock */
struct FaderBlock;
typedef struct FaderBlock FaderBlock;

typedef int32 (*FaderEventFunctionP) ( int32 FaderIndex, float32 FaderValue, FaderBlock *fdbl );

struct FaderBlock
{
	int32   fdbl_Current;               /* Index of current fader. */
	int32   fdbl_Previous;
	int32   fdbl_NumFaders;             /* Number of Faders pointed to by fdbl_Faders (set by client). */
	int32   fdbl_OneShot;               /* Allow single click to advance faders. */
	Fader  *fdbl_Faders;                /* Pointer to Fader table (set by client). */
	FaderEventFunctionP fdbl_Function;  /* Called on any fader value change event, even trivial ones. (set by client). */
};


/* -------------------- Misc defines */

#define FADER_HEIGHT    (0.025)
#define FADER_SPACING   (0.04)
#define FADER_XMIN      (0.30)
#define FADER_XMAX      (0.75)
#define FADER_YMIN      (0.175)
#define FADER_YMAX      (0.9)
#define MAX_FADER_VALUE (1.0)

    /* @@@ this is assumed to truncate (!!! is there a better way to ensure this?) */
#define MAX_FADERS      (int)(((FADER_YMAX-FADER_YMIN)+(FADER_SPACING-FADER_HEIGHT)) / FADER_SPACING)

#define LEFT_VISIBLE_EDGE  (0.1)


/* -------------------- Globals */

extern PortableGraphicsContext *gPGCon;


/* -------------------- Functions */

int32 InitFader ( Fader *fdr , int32 Index);
Err DrawFader ( const Fader * );
int32 JoyToFader( Fader *fdr, uint32 joy );
int32 UpdateFader ( Fader *fdr , int32 val );

int32 InitFaderBlock ( FaderBlock *fdbl, Fader *Faders, int32 NumFaders, FaderEventFunctionP eventfn );
Err DrawFaderBlock ( const FaderBlock * );
int32 DriveFaders ( FaderBlock *fdbl, uint32 joy );

Err InitFaderGraphics ( int32 NumScr );
Err TermFaderGraphics ( void );


/*****************************************************************************/


#endif  /* __AUDIODEMO_FADERS_H */
