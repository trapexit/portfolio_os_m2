/******************************************************************************
**
**  @(#) markov_tables.h 96/02/28 1.3
**
**	Markov Tables header
**
**	Data tables for Markov Music
**	Author: rnm
**
******************************************************************************/

#define MOD(x,y) ((x) - ((x)/(y))*(y))

void mtPrintTable ( int32 from, int32 to );
char* mtChoose ( int32 fromSound, int32 toSound, char* fromMarker,
  char* toMarker );
