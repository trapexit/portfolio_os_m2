
/******************************************************************************
**
**  @(#) loopbench.c 96/01/17 1.3
**
******************************************************************************/

/*
** loopbench - do continuous benchmarks, reporting occasionally.
*
** Author: Phil Burk
** Copyright 3DO 1995-
*/

#include <audio/audio.h>
#include <stdio.h>
#include <stdlib.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}
#define DEFAULT_NUMTESTS   (5)
#define DEFAULT_NUMREPS   (10)

/*******************************************************************/
/* Towers of Hanoi lifted from m2bench */

#define maxcells	18
#define stackrange	3

struct    element {
	    int discsize;
	    int next;
	};

int    		stack[stackrange+1];
struct element  cellspace[maxcells+1];
int    		freelist, movesdone;

    /*  Program to Solve the Towers of Hanoi */

Error (char *emsg)
{
	printf(" Error in Towers: %s\n",emsg);
}

Makenull (int s)
{
	stack[s]=0;
}

int Getelement (void)
{
	int temp;
	if ( freelist>0 ) {
	    temp = freelist;
	    freelist = cellspace[freelist].next;
	}
	else
	{
	    Error("out of space   ");
	    temp = -1;
	}
	return (temp);
}

void Push(int i,int s)
{
	int errorfound, localel;
	errorfound=false;
	if ( stack[s] > 0 )
    	if ( cellspace[stack[s]].discsize<=i ) {
		errorfound=true;
		Error("disc size error");
	}
	if ( ! errorfound ) {
	    localel=Getelement();
	    cellspace[localel].next=stack[s];
	    stack[s]=localel;
	    cellspace[localel].discsize=i;
	}
}

void Init (int s,int n)
{
	int discctr;
	Makenull(s);
	for ( discctr = n; discctr >= 1; discctr-- )
    		Push(discctr,s);
}

int Pop (int s)
{
 	int temp, temp1;
	if ( stack[s] > 0 )
    	{
	    temp1 = cellspace[stack[s]].discsize;
	    temp = cellspace[stack[s]].next;
	    cellspace[stack[s]].next=freelist;
	    freelist=stack[s];
	    stack[s]=temp;
	    return (temp1);
    	}

  	Error("nothing to pop ");
    	return -1;
}

void MoveIt (int s1,int s2)
{
	Push(Pop(s1),s2);
	movesdone=movesdone+1;
}

void tower(int i,int j,int k)
{
	int other;

	if ( k==1 )
    		MoveIt(i,j);
	else {
	    	other=6-i-j;
	    	tower(i,other,k-1);
	    	MoveIt(i,j);
	    	tower(other,j,k-1);
	}
}


void Towers (void)
{
    int i;
    for ( i=1; i <= maxcells; i++ )
	cellspace[i].next=i-1;
    freelist=maxcells;
    Init(1,14);
    Makenull(2);
    Makenull(3);
    movesdone=0;
    tower(1,2,14);
    if ( movesdone != 16383 )
	printf (" Error in Towers.\n");
}

/*******************************************************************/
#define FIC_NUM_VALS   (200)
static int32 ficData[FIC_NUM_VALS];

static void ficInit( void )
{
	int32 i;
	for( i=0; i<FIC_NUM_VALS; i++ ) ficData[i] = i*3;
}
static int32 ficAddEmUp( int32 numReps )
{
	register int32 i, j, sum;
	sum = 0;
	for( j=0; j<numReps; j++ )
	{
		for( i=0; i<FIC_NUM_VALS; i++ ) sum += ficData[i];
	}
	return sum;
}

/*******************************************************************/
void PrintHelp( void )
{
	printf("Usage: loopbench {options}\n");
	printf("   -rRep         = number of repititions / bench (default = %d)\n", DEFAULT_NUMREPS);
	printf("   -tTests       = run benchmark this many times(default = %d)\n", DEFAULT_NUMTESTS);
}

/*******************************************************************/
int main(int argc, char *argv[])
{
	int32 Result, sum;
	int32 numReps = DEFAULT_NUMREPS;
	int32 numTests = DEFAULT_NUMTESTS;
	int32 lastTime = 0;
	char *s, c;
	int i;
	AudioTime  startTime, endTime, elapsedTime;

	PRT(("Begin %s\n", argv[0] ));
	TOUCH(argc); /* Eliminate anal compiler warning. */

/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}


/* Get input parameters. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];

		if( *s++ == '-' )
		{
			c = *s++;
			switch(c)
			{
			case 'r':
				numReps = atoi(s);
				break;
			case 't':
				numTests = atoi(s);
				break;

			case '?':
			default:
				PrintHelp();
				exit(1);
				break;
			}
		}
		else
		{
			PrintHelp();
			exit(1);
		}
	}

	ficInit();

/* Run benchmarks and report results. */
	for( i=0; i<numTests; i++ )
	{
		startTime = GetAudioTime();
#if 0
		for( j=0; j<numReps; j++ )
		{
/*			Towers(); */
		}
#endif

		sum = ficAddEmUp( numReps );

		endTime = GetAudioTime();
		elapsedTime = endTime - startTime;
		PRT(("%d reps took %d ticks, sum = %d\n", numReps, elapsedTime, sum ));
		if( lastTime > 0 )
		{
			float32 fraction = (float32) elapsedTime / (float32)  lastTime;
			PRT(("    = %g * last time;\n", fraction ));
		}
		lastTime = elapsedTime;
	}

	PRT(("Done with %s\n", argv[0] ));
	CloseAudioFolio();
	return 0;
}
