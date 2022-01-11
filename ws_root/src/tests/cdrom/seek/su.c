/* @(#) su.c 96/05/01 1.2 */

/*
 su.c

This file contains lots of little routines that are Utilities for the Seek
tests.

unsigned char AllocOrDie( int size ) 
   returns a pointer to size bytes of allocated memory.

int int2time( int value )
   converts value from an int to another int that when printed appears to be 
   the bcd formatted mmssff time.

int time2int( int value )
   inverse of int2time. 

int myatoi( char *string )
   if string starts with "t" or "T", then converts from mmssff text to int. 
   else returns atoi( string ).

int string2time( char *string )
   converts a string of form "tmmssff" to an integer where mm,ss,ff are bcd, 
   and the "t" is "t" or "T".

char *time2string( int value )
   returns a static string of the form "t001122" where "001122" is a msf time...
	
int sector_to_track( int sector )
   returns a rought estimate of the number of tracks from time zero, to a give
   sector.
	
int track_to_sector( int track )
   given a track number, returns a rough estimate of the number sectors between
   time zero, and that track.
	
int sectors_per_track( int track )
   given a track, how many sectors are there in one revolution of the disc.  
   (Useful for estimates of rotation latency.)
			
uint32 TimerOverhead()
   Alot of this code uses a call to MyTime(). How much time does it take to
   call MyTime?  This routine returns that number. See MyTime.c.
			
uint32 time_seek( int start, int end )
   given a starting sector number, and an ending sector number, return the 
   number of microseconds it takes to seek from the start to the end.
			
void get_minmax( int start, int end, int *pmin, int *pmax )
void get_minmax_long( int start, int end, int *pmin, int *pmax )
void get_minmax_short( int start, int end, int *pmin, int *pmax )
   given a start sector, and an end sector, measure the longest and shortest 
   times to perform approximately that seek. Accounts for rotational latency. 
   Actually performs many seeks.  Note that you must pass pointers to the
   locations for the min and max times.
			
   get_minmax() actually calls either get_minmax_long or get_minmax_short based 
   on whether the seek is a long seek or a short seek. 
			
int get_min_time( int start, int end )
   returns the shortest time found by get_minmax(). Easier to use.
			
void get_minmin( start, end , pfor, prev ) int start, end , *pfor, *prev ;
   This routine, given two sector numbers, times seeks in both directions.  It 
   trys several seeks at "about" the locations you asked for to try to remove
   the effects of rotational latency from the measurements.  Returns two times,
   one each for seeks from start to end, and for from end to start.
*/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>

#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "drive.h"
#include "mytime.h"
#include "su.h"

#define READMINBLOCK 150
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ABS(a)   (((a)< 0)?(-(a)):(a))

#define PI (3.1415926)	
#define sector_length ( 1.3 / 75. )
#define track_pitch	( 1.6/1000000. )


#define DBG(x) /* if( 1 == 0 ) printf x */

unsigned char *AllocOrDie(int size)
{
	unsigned char *temp;
	int i;
	
	temp = (unsigned char *)AllocMem(size, MEMTYPE_DMA);
	if(!temp)
	{
		printf("AllocOrDie failed to allocate. Death.\n");
		exit(0);
	}
	for(i = 0; i < size; i++)
		temp[i] = 0 ;

	return (temp);
}


int myatoi(char *string)
{
	switch(string[0])
	{
		case 't':
		case 'T':
			break;
		default:
			return (int)atoi(string);
	}

	return (time2int(atoi(string+1)));
}

int	int2time(int value)
{
	int	mm,ss,ff,result;

	mm = value/(75*60);
	ss = (value/75)%60;
	ff = value%75 ;

	result = mm*10000 + ss*100 + ff ;

	return (result);
}

int	time2int(int value)
{
	int mm,ss,ff,result;

	mm = value / 10000 ;
	ss = (value /100)%100 ;
	ff = value %100 ;

	result = 60*75*mm+75*ss+ff ;

	return (result);
}

#define S2T_BASE 8192

static int s2t_table[]= { 
		   0,  879, 1714, 2510, 3273,   4006, 4713, 5396, 6058, 6699,
		7323, 7931, 8523, 9101, 9666,  10218,10759,11289,11809,12319,
		12820,13312,13796,14273,14741, 15203,15658,16106,16548,16984,
		17415,17839,18259,18673,19082, 19487,19887,20283,20674,21061,
		21444,21823,22198,22570,22938, 23302,23664,24021
};


int sector_to_track(int sector)
{
	int entry ;
	int lowtrack, deltrack, track ;
	
	entry = sector/S2T_BASE ;
	
	lowtrack = s2t_table[entry];
	deltrack = s2t_table[entry+1] - lowtrack;
	
	track = lowtrack+((sector%S2T_BASE)*deltrack)/S2T_BASE;
	
	return (track);
}

#define T2S_BASE 512

static int t2s_table[] = {
	0,4716,9584,14604,19776,25100,30576,36204,41984,47917,54001,60237,66626,
	73166,79858,86703,93699,100848,108149,115601,123206,130963,138871,146932,
	155145,163510,172027,180696,189517,198490,207615,216892,226321,235902,
	245635,255521,265558,275747,286089,296582,307228,318025,328975,340076
};

int track_to_sector(int track)
{
	int sector ;
	int entry ;
	int low, del ;
	
	entry = track/T2S_BASE ;
	
	low = t2s_table[entry];
	del = t2s_table[entry+1] - low;
	
	sector = low + ((track%T2S_BASE)*del)/T2S_BASE;
	
	return (sector);
}


int sectors_per_track(int track)
{
	return (track_to_sector( track+1 ) - track_to_sector( track ) + 1);
}

uint32 TimerOverhead(void)
{
		uint32	time0, time1 = 0;
		int		i;	

		time0 = MyTime();
		for(i = 0; i < 10; i++)
			time1 = MyTime();
	
		time0 = (time1 - time0) / 10;

#if 0
		if (1 == 0)
			printf("Timer overhead = %ld us\n",time0 );	
#endif
		
		return (time0);
}

uint32 time_seek(int start, int end)
{
	uint32 time0,time1,time;
	static unsigned char *buf = NULL;
	static uint32 timer_overhead = 0;

	if (!buf)
	{
		buf = AllocOrDie( 2532+98+10  ) ;
	
		timer_overhead = TimerOverhead() ;

#if 0
		if (1 == 0)
			printf("Timer overhead = %ld us\n",timer_overhead );	
#endif
	}
	
	MyCDRead(buf, READMINBLOCK+start, 1);
	time0 = MyTime();
	MyCDRead(buf, READMINBLOCK+end, 1);
	time1 = MyTime();
				
	time = time1 - time0 - timer_overhead;
	
	DBG(("Seek time from %6d to %6d is %6ld\n",start,end,time ));
	
	return (time);
}


#define DISTANCE( a,b ) (( a > b ) ? ( a - b ): ( b - a ))

void get_minmax_long(int start, int end, int *pmin, int *pmax)
{
	int	min, max;
	int spr ;
	
	int e1, e2;
	int i;
	int time;
	
	spr = sectors_per_track(sector_to_track(end));
	
	DBG(("sectors per track = %d\n", spr));
	
/* pick e1 and e2 such that they are two end points, both inside of 
 * the specified seek, spaced about a revolution apart, and such that seeks
 * from start to e1(e2) should linearly increase in time as the end moves from 
 * e1 to e2 ( with the exception of the rotational sawtooth effect ).
 */

	if (end > start)
	{
		e1 = end - spr;
		e2 = end;
	}
	else
	{
		e1 = end;
		e2 = end + spr;
	}

	/* punt, do eight seeks at various offsets to setup min/max */
	
	for(i = e1, min = -1, max = -1;
		i < e2-2;
		i += ((e2-e1)>>3))
	{
		time = (int)time_seek(start,i );
		if (min < 0)
		{
			min = time;
			max = time;
		}
		min = MIN( min,time );
		max = MAX( max,time );
	}
	*pmin = min; 
	*pmax = max;	
}

#define NAVG 5 

void get_minmax_short(int start,int end, int *pmin, int *pmax)
{
	int min, max;
	int i;
	int time;

	time = (int)time_seek(start, end);		
	min = time; max = time ;
	for(i = 0; i < NAVG-1; i++)
	{
		time = (int)time_seek(start, end);
		min = MIN(min, time);
		max = MAX(max, time);	
	}
	*pmin = min; 
	*pmax = max;
}


#define SHORT_NUMBER_OF_SECTORS 20 

void get_minmax(int start, int end, int *pmin, int *pmax)
{
	int dist = start - end;
	if (dist < 0)
		 dist = -dist;
	if (dist < SHORT_NUMBER_OF_SECTORS)
		get_minmax_short(start, end, pmin, pmax);
	else
		get_minmax_long(start, end, pmin, pmax);
}


int get_min_time(int start, int end)
{
	int min, max;

	get_minmax(start, end, &min, &max );

	return(min);
}

static void	time_seek_maxmax(int start, int end, int *pfor, int *prev)
{
	int time0, time1, time2;
	static unsigned char *buf = NULL;
	static int timer_overhead = 0;
	static int last_location = 0;

	if (!buf)
	{
		buf = AllocOrDie(2532 + 98 + 10);
	
		timer_overhead = (int)TimerOverhead();

#if 0
		if (1 == 0)
			printf("Timer overhead = %d us\n",timer_overhead );	
#endif
	}
	
	if (ABS(start - last_location) < 32)
	{
		if (start > 1000)
			MyCDRead( buf, READMINBLOCK+start-1000 , 1 ) ;
		else
			MyCDRead( buf, READMINBLOCK+start+1000, 1 ) ;
	}
#if 0
	if (0)
		printf("(%d:%d)",start,end);
#endif 
	MyCDRead(buf, READMINBLOCK+start, 1);
	time0 = (int)MyTime();
	MyCDRead(buf, READMINBLOCK+end, 1);
	time1 = (int)MyTime();
	MyCDRead( buf, READMINBLOCK+start, 1);
	time2 = (int)MyTime();	
	last_location = start;
	*pfor = time1 - time0 - timer_overhead;
	*prev = time2-time1-timer_overhead;
}


void get_maxmax(int start, int end, int *pfor, int *prev)
{
	int i;
	int mfor = 0;
	int mrev = 0;
	int tfor, trev;
	int sectors;
	
	if (start > end)
	{
		i = start;
		start = end;
		end = i;
	}
	
	sectors = sectors_per_track(sector_to_track(start));
	i = sectors_per_track(sector_to_track(end));
		  
	if (i > sectors)
		sectors = i;
	
	while (sectors > 0)
	{
#if (1)	
		time_seek_maxmax(start, end, &tfor, &trev);
#else
		tfor = start;
		trev = end;
#endif
		if (tfor > mfor)
			mfor = tfor;
		if (trev > mrev)
			mrev = trev;
		sectors -= 2;
		start++ ;
		end-- ;
	}
	
	*pfor = mfor; 
	*prev = mrev;
}

void get_minmin(int start, int end, int *pfor, int *prev)
{
	int i;
	int mfor = 0x1e7; /* ten seconds is pretty big */
	int mrev = 0x1e7;
	int tfor, trev;
	int sectors;
	
	if (start > end)
	{
		i = start;
		start = end;
		end = i;
	}
	
	sectors = sectors_per_track(sector_to_track(start));
	i = sectors_per_track(sector_to_track(end));
		  
	if (i > sectors)
		sectors = i;
	
	while (sectors > 0)
	{
#if (1)	
		time_seek_maxmax( start,end ,&tfor,&trev );
#else
		tfor = start;
		trev = end;
#endif
		if (tfor < mfor)
			mfor = tfor;
		if (trev < mrev)
			mrev = trev;
		sectors -= 2;
		start++ ;
		end-- ;
	}
	
	*pfor = mfor; 
	*prev = mrev;
}

