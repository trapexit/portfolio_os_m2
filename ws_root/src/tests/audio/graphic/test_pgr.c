
/******************************************************************************
**
**  @(#) test_pgr.c 96/08/27 1.9
**
******************************************************************************/


#include <audio/audio.h>
#include <audio/music.h>
#include <graphics/graphics.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <audiodemo/portable_graphics.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}

void SetRandomColor( PortableGraphicsContext *pgcon )
{
	uint32 rmask;
	float32 r,g,b;
	rmask = rand();
	r = (rmask & 0xFF) / 256.0;
	g = ((rmask>>8) & 0xFF) / 256.0;
	b = ((rmask>>16) & 0xFF) / 256.0;
	pgSetColor( pgcon, r,g,b );
}

/************************************************************************/
void WaitButton( uint32 button )
{
	ControlPadEventData cped;

	do
	{
		GetControlPad (1, TRUE, &cped);   /* Wait for button press. */
	}
	while( (cped.cped_ButtonBits & button) == 0 );
}

/************************************************************************/
Err TestRandomLines( PortableGraphicsContext *pgcon )
{
	int32 i;
	uint32 rmask;
	float32 x,y;

/* Draw lots of random lines. */
	for( i=0; i<1000; i++ )
	{
		SetRandomColor( pgcon );

		rmask = rand();
		x = (rmask & 0xFFFF) / 65536.0;
		y = ((rmask>>8) & 0xFFFF) / 65536.0;
		pgDrawTo( pgcon, x, y );
	}
	pgSwitchScreens( pgcon );

	PRT(("Wait for button A to be pressed.\n"));
	WaitButton( ControlA );

	return 0;
}

/************************************************************************/
Err TestZeroLines( PortableGraphicsContext *pgcon )
{
	int32 i;
	uint32 rmask;
	float32 x,y;

/* Draw lots of random lines. */
	for( i=0; i<1000; i++ )
	{

		SetRandomColor( pgcon );

		rmask = rand();
		x = (rmask & 0xFFFF) / 65536.0;
		if( i & 1 )
		{
			y = 0.0;
			pgDrawTo( pgcon, x, y );
		}
		else
		{
			y = ((rmask>>8) & 0xFFFF) / 65536.0;
			pgMoveTo( pgcon, x, y );
		}
	}
	return pgSwitchScreens( pgcon );
}

/************************************************************************/
Err TestLinePattern( PortableGraphicsContext *pgcon )
{
	pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );

	pgSetColor( pgcon, 1.0, 1.0, 1.0 );
/* Draw some lines. */
	pgMoveTo( pgcon, 0.0, 0.0 );
	pgDrawTo( pgcon, 0.0, 1.0 );
	pgDrawTo( pgcon, 1.0, 1.0 );
	pgDrawTo( pgcon, 1.0, 0.0 );
	pgDrawTo( pgcon, 0.0, 0.0 );
	pgDrawTo( pgcon, 1.0, 1.0 );

	pgMoveTo( pgcon, 0.0, 1.0 );
	pgDrawTo( pgcon, 1.0, 0.0 );

/* Draw inner square. */
	pgSetColor( pgcon, 0.7, 0.2, 0.4 );
	pgMoveTo( pgcon, 0.25, 0.25 );
	pgDrawTo( pgcon, 0.25, 0.75 );
	pgDrawTo( pgcon, 0.75, 0.75 );
	pgDrawTo( pgcon, 0.75, 0.25 );
	pgDrawTo( pgcon, 0.25, 0.25 );
	pgDrawTo( pgcon, 0.75, 0.75 );

/* Draw outer diamond. */
	pgSetColor( pgcon, 0.1, 0.9, 0.6 );
	pgMoveTo( pgcon, 0.5, -0.25 );
	pgDrawTo( pgcon, 1.25, 0.5 );
	pgDrawTo( pgcon, 0.5, 1.25 );
	pgDrawTo( pgcon, -0.25, 0.5 );
	pgDrawTo( pgcon, 0.5, -0.25 );

/* Draw arrow so we know where 1,1 is. */
	pgMoveTo( pgcon, 0.2, 0.0 );
	pgDrawTo( pgcon, 1.0, 1.0 );
	pgDrawTo( pgcon, 0.0, 0.2 );

	pgMoveTo( pgcon, 0.5, 0.5 );

/* Make the screen we have created visible. */
	pgSwitchScreens( pgcon );

	WaitButton( ControlA );

	return 0;
}

/************************************************************************/
void TestRectangles( PortableGraphicsContext *pgcon )
{
/* Set background. */
	pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );

/* Orange horizontal strip. */
	pgSetColor( pgcon, 1.0, 0.7, 0.0 );
	pgDrawRect( pgcon, 0.2, 0.2, 0.8, 0.3 );

/* Green vertical strip. */
	pgSetColor( pgcon, 0.1, 0.9, 0.1 );
	pgDrawRect( pgcon, 0.3, 0.9, 0.45, 0.18 );

/* Make the screen we have created visible. */
	pgSwitchScreens( pgcon );

	PRT(("Wait for button A to be pressed.\n"));
	WaitButton( ControlA );
}

/************************************************************************/
void TestColorAtAngles( PortableGraphicsContext *pgcon )
{
	int32 i;
	float32 x,y;

/* Draw Background so we can see pixel colors. */
	pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );

/* Draw colored rectangle. */
	pgSetColor( pgcon, 0.7, 0.2, 0.4 );
	pgDrawRect( pgcon, 0.25, 0.25, 0.75, 0.75 );

/* Draw black inside. */
	pgSetColor( pgcon, 0.0, 0.0, 0.0 );
	pgDrawRect( pgcon, 0.35, 0.35, 0.65, 0.65 );

/* Draw radial fan of lines. */
#define NUM_LINES   (20)
	pgSetColor( pgcon, 0.7, 0.2, 0.4 );
	for( i=0; i<NUM_LINES; i++ )
	{
		x = 0.5 + (0.5 * cosf( (i * 2 * PI)/NUM_LINES ));
		y = 0.5 + (0.5 * sinf( (i * 2 * PI)/NUM_LINES ));
DBUG(("x = %g, y = %g\n", x, y ));
		pgMoveTo( pgcon, 0.5, 0.5);
		pgDrawTo( pgcon, x, y );
	}
/* Make the screen we have created visible. */
	pgSwitchScreens( pgcon );

	PRT(("Wait for button A to be pressed.\n"));
	WaitButton( ControlA );
}

/************************************************************************/
void TestText( PortableGraphicsContext *pgcon )
{
	pgClearDisplay( pgcon, 0.0, 0.0, 3.0 );

	pgMoveTo( pgcon, 0.1, 0.4 );
	pgDrawText( pgcon, "ABCD abcd XYZ xyz" );
	pgMoveTo( pgcon, 0.1, 0.6 );
	pgDrawText( pgcon, "0123456789" );
	pgMoveTo( pgcon, 0.1, 0.8 );
	pgDrawText( pgcon, "!@#$%^&*()_+" );

	PRT(("Wait for button A to be pressed.\n"));
	WaitButton( ControlA );
}

/************************************************************************/

int main( int argc, char *argv[] )
{
	int32 Result;
	PortableGraphicsContext *pgcon;

/* Eliminate compiler warnings. */
	TOUCH(argc);
	TOUCH(argv);

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		ERR(("main: error in InitEventUtility\n"));
		PrintfSysErr(Result);
		goto DONE;
	}

/* Set up double buffered display. */
	Result = pgCreateDisplay( &pgcon, 2 );
	if(Result < 0)
	{
		ERR(("pgOpenDisplay failed 0x%x\n"));
		return (int) Result;
	}

	TestText( pgcon );

	TestColorAtAngles( pgcon );

	TestLinePattern( pgcon );

	TestRectangles( pgcon );

	TestRandomLines( pgcon );


/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		ERR(("main: error in KillEventUtility\n"));
		PrintfSysErr(Result);
	}

	pgDeleteDisplay( &pgcon );

DONE:
	return( (int) Result );
}

