/***************************************************************
**
** @(#) faders.c 96/01/18 1.11
** $Id: faders.c,v 1.16 1994/03/01 01:15:35 peabody Exp $
**
** Horizontal faders.
**
** By: Phil Burk
** Copyright 1993 - 3DO Company, Inc.
**
****************************************************************
**  940225 WJB  Added DrawFaderBlock().  Added const to DrawFader().
**  940228 WJB  Fixed functions to tolerate fdbl_NumFaders == 0.  Cleaned up includes.
**  940228 WJB  Changed fader fine adjustment to use smaller increments near 0.
**  940228 WJB  Now only calls fdbl_Function() when fader value actually changes.
**  940228 WJB  Added string.h.
**  951214 PLB  Added TermFaderGraphics
****************************************************************/

#include <audio/handy_macros.h> /* MIN() */
#include <audiodemo/faders.h>   /* self */
#include <audiodemo/portable_graphics.h>
#include <misc/event.h>         /* Control Pad stuff */
#include <stdio.h>              /* sprintf() */
#include <string.h>             /* memset() */


/* -------------------- Debugging */

#define DEBUG_JoyToFader    0

PortableGraphicsContext *gPGCon;
#define PGCONTEXT   gPGCon

static Err DrawNumberFP( float32 num );


/* -------------------- Fader functions */

/********************************************************************/
int32 InitFader ( Fader *fdr , int32 Index )
{
	memset (fdr, 0, sizeof *fdr);

	fdr->fdr_XMin = FADER_XMIN;
	fdr->fdr_XMax = FADER_XMAX;
	fdr->fdr_YMin = FADER_YMIN + ( Index * FADER_SPACING );
	fdr->fdr_YMax = fdr->fdr_YMin + FADER_HEIGHT;
	fdr->fdr_VMin = 0;   /* Value min and max. */
	fdr->fdr_VMax = 1.0;
	fdr->fdr_Value = 0.5;
	fdr->fdr_Increment = 0.02;

/* Printing this seems to fix a compiler bug in the setting of fdr->fdr_YMin !!! remove eventually */
#if 0
	printf("DIAB BUG!!! Print or die! - Index = %d, YMin = %g, YMax = %g\n", Index,
		fdr->fdr_YMin, fdr->fdr_YMax );
#endif

	return 0;
}

/********************************************************************/
Err DrawFader ( const Fader *fdr )
{
	float32 xpos, scalar;

	pgSetColor( PGCONTEXT, 1.0, 1.0, 1.0 );
	if (fdr->fdr_Name )
	{
		pgMoveTo( PGCONTEXT, LEFT_VISIBLE_EDGE, fdr->fdr_YMin );
		pgDrawText( PGCONTEXT, fdr->fdr_Name );
	}

/* Transform Value to display coordinates. */
	scalar = (fdr->fdr_Value - fdr->fdr_VMin) / ( fdr->fdr_VMax - fdr->fdr_VMin );
	xpos = fdr->fdr_XMin + (scalar * ( fdr->fdr_XMax - fdr->fdr_XMin ));

/* Draw first part of fader. */
	if (fdr->fdr_Highlight)
	{
		pgSetColor( PGCONTEXT, 1.0, 1.0, 0.0 );
	}
	else
	{
		pgSetColor( PGCONTEXT, 0.5, 0.5, 0.0 );
	}
	pgDrawRect( PGCONTEXT, fdr->fdr_XMin, fdr->fdr_YMin, xpos, fdr->fdr_YMax);

/* Draw second part of fader. */
	if (fdr->fdr_Highlight)
	{
		pgSetColor( PGCONTEXT, 0.0, 0.0, 1.0 );
	}
	else
	{
		pgSetColor( PGCONTEXT, 0.0, 0.0, 0.5 );
	}
	pgDrawRect( PGCONTEXT, xpos, fdr->fdr_YMin, fdr->fdr_XMax, fdr->fdr_YMax);

	pgSetColor( PGCONTEXT, 1.0, 1.0, 1.0 );
	pgMoveTo ( PGCONTEXT, fdr->fdr_XMax + 0.03, fdr->fdr_YMin );
	DrawNumberFP ( fdr->fdr_Value );

	return 0;
}

/********************************************************************/
int32 JoyToFader( Fader *fdr, uint32 joy )
{
	float32 newvalue ;
	float32 incr;

/* LShift - fine: use 5% of normal increment or range (whichever is smaller) */
	incr = (joy & ControlLeftShift)
                     ? MIN( (fdr->fdr_Increment * 0.05), ABS(fdr->fdr_Value * 0.05) )
                     : fdr->fdr_Increment;          /* normal - coarse: normal increment */

	newvalue = fdr->fdr_Value;
#if DEBUG_JoyToFader
	printf ("%s: incr=%%g val=%%g->", fdr->fdr_Name, incr, fdr->fdr_Value);
#endif

	if (joy & ControlLeft)  newvalue -= incr;
	if (joy & ControlRight) newvalue += incr;

	fdr->fdr_Value = CLIPRANGE (newvalue, fdr->fdr_VMin, fdr->fdr_VMax);

#if DEBUG_JoyToFader
	printf ("%%g\n", fdr->fdr_Value);
#endif

	return 0;
}


/* -------------------- FaderBlock */

/********************************************************************/
int32 DriveFaders ( FaderBlock *fdbl, uint32 joy )
{
	if (fdbl->fdbl_NumFaders)
	{
		int32 NewFader = 0;

		if (joy & (ControlRight|ControlLeft))
		{
			Fader * const fader = &fdbl->fdbl_Faders [ fdbl->fdbl_Current ];
			const int32 oldval = fader->fdr_Value;

			JoyToFader (fader, joy);
			if (fader->fdr_Value != oldval && fdbl->fdbl_Function)
			    (*fdbl->fdbl_Function) (fdbl->fdbl_Current, fader->fdr_Value, fdbl);
		}
		else if (joy & ControlDown)
		{
			if (fdbl->fdbl_OneShot == 0)
			{
				if (fdbl->fdbl_Current < fdbl->fdbl_NumFaders-1 )
				{
					fdbl->fdbl_Current += 1;
					NewFader = 1;
				}
				fdbl->fdbl_OneShot = 1;
			}
		}
		else if (joy & ControlUp)
		{
			if (fdbl->fdbl_OneShot == 0)
			{
				if (fdbl->fdbl_Current > 0 )
				{
					fdbl->fdbl_Current -= 1;
					NewFader = 1;
				}
				fdbl->fdbl_OneShot = 1;
			}
		}

		if (joy & ControlA)
		{
		}

		if (joy == 0)
		{
			fdbl->fdbl_OneShot = 0;
		}

		if (NewFader)
		{
			fdbl->fdbl_Faders[fdbl->fdbl_Previous].fdr_Highlight = 0;
			fdbl->fdbl_Faders[fdbl->fdbl_Current].fdr_Highlight = 1;
			fdbl->fdbl_Previous = fdbl->fdbl_Current;
		}

	}

	return DrawFaderBlock ( fdbl );
}

/********************************************************************/
/*
    Init FaderBlock and Faders.  Harmless if NumFaders == 0.
*/
int32 InitFaderBlock ( FaderBlock *fdbl, Fader *Faders, int32 NumFaders, FaderEventFunctionP eventfn )
{
	memset (fdbl, 0, sizeof *fdbl);
	fdbl->fdbl_NumFaders = NumFaders;
	fdbl->fdbl_Function  = eventfn;

/* only set this stuff if NumFaders != 0 */
	if (NumFaders)
	{
		int32 i;

/* link in faders array */
		fdbl->fdbl_Faders = Faders;

/* init faders array */
		for (i=0; i<NumFaders; i++) InitFader ( &Faders[i], i );
		fdbl->fdbl_Faders[fdbl->fdbl_Current].fdr_Highlight = 1;
	}

	return 0;
}

/********************************************************************/
/*
    Draw all Faders in a FaderBlock
*/
Err DrawFaderBlock (const FaderBlock *faderblock)
{
    const Fader *fader = faderblock->fdbl_Faders;
    int32 nfaders      = faderblock->fdbl_NumFaders;

    while (nfaders--) DrawFader (fader++);

    return 0;
}

/********************************************************************/
static Err DrawNumberFP( float32 num )
{
	char Pad[100];

	sprintf(Pad, "%g", num);
	pgDrawText( PGCONTEXT, Pad );
	/* pgDrawText( PGCONTEXT, "    " ); */
	return 0;
}

/********************************************************************/
Err InitFaderGraphics ( int32 NumScr )
{
	return pgCreateDisplay( &PGCONTEXT, NumScr );
}
/********************************************************************/
Err TermFaderGraphics ( void )
{
	return pgDeleteDisplay( &PGCONTEXT );
}

