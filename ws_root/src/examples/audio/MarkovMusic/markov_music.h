/******************************************************************************
**
**  @(#) markov_music.h 96/04/18 1.3
**
******************************************************************************/
/* ---- assorted useful macros */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define MIN(val,maximum) ( val <= maximum ? val : maximum )
#define MAX(val,minimum) ( val >= minimum ? val : minimum )

	/* disc-based sounds */
enum {
	STOPPED_SOUND,
	MOVING_SOUND,
	FAST_SOUND,
	TRANSITION_SOUND,
	NUMBER_OF_SOUNDS
};

/* ---- Functions */
Err markerNumbertoName( int32 block, int32 subblock, char* name );
Err markerNametoNumber( char* name, int32* block, int32* subblock );

