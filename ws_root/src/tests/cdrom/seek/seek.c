/* @(#) seek.c 96/05/01 1.4 */

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
 *  seek.c - test program for driving the CD-ROM.
 * 
 *
 * This is the main software for the seek tests. I always just use
 * seek with no arguements.
 * 
 * This prints seven columns of tab separated spaces which can then be pasted 
 * into an Excel spreadsheet for graphing.
 * 
 * The first is the number of tracks of the seek.
 * The second is the time for a forward seek from 0 to the number of tracks.
 * The third is the time for a reverse seek from the number of tracks to zero.
 * The fouth is a forward seek centered around the center of the disc.
 * The fifth is a reverse seek centered around the center of the disc.
 * The sixth is a forward seek of ending at the end of the disc.
 * The seventh is reverse seek starting at the end of the disc.
 * 
 * The number of tracks increases in a logarithmic fashion from 1 to the
 * total number of tracks on the disc.
 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <device/cdrom.h>
#include <misc/event.h>

#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "su.h"
#include "drive.h"
#include "mytime.h"

#define READMINBLOCK 150
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ABS(a)   (((a)< 0)?(-(a)):(a))
#define streq(a,b) (!strcmp(a, b))
#define DBG if (1 == 1) printf


static int verbose = FALSE ;

/* Attempt to measure the linear velocity of the disc */
void linear_rate_test(void)
{
	uint32	time;
	uint32	rate; 
	uint32	total = 0;
	int	i;
	
	for(i = 0; i < 10; i++)
	{
		time_seek(0, 12);
		time = time_seek(20, 20);
		rate = (50 * 3141592) / time;
		printf("Time = %8d, rate = %8d 0.5 rate = %8d\n",time,rate, rate/2);
		total += time;
	}
	time = total/10;
	rate = (50 * 3141592) / time;
	printf("AVG  = %8d, rate = %8d 0.5 rate = %8d\n",time,rate, rate/2);
}

void playfunction(int from, int to)
{
	int i;
	static unsigned char *buf;

	buf = AllocOrDie(2532 + 98 + 10);
	
	printf("Buffer is located at %08X\n",buf);
	
	printf("Reading ...\n");
	for(i = from; i <= to; i++)
		MyCDRead( buf,i,1 );
	printf("Complete.\n");
}


void rand_seeks(int blocks, int npasses)
{
	int		i;
	uint32	b;
	uint32 time0, time1, time;
	uint32 *array;
	static unsigned char *buf = NULL;
	static uint32 timer_overhead = 0;

	b = (uint32)blocks - 150 - 150;
	
	if (npasses < 0)
		npasses = 100;

	npasses++ ;
	
	printf("creating random table of %d seeks\n",npasses ); 

	if (verbose)
		printf("This test does not report propertimes when verbose is true\n");

	buf = AllocOrDie(2532 + 98 + 10);
	timer_overhead = TimerOverhead();

	array = (uint32 *)AllocOrDie(npasses*sizeof(uint32));
	for(i = 0; i < npasses; i++)
		array[i] = 150 + rand()%b; 

	printf("Seeking\n");
	
	MyCDRead(buf, array[0], 1);
	time0 = MyTime();
	for(i = 1; i < npasses; i++)
	{
		if (verbose)
			printf("%d\n",array[i] );
		MyCDRead(buf, array[i], 1);
	}
	time1 = MyTime();
				
	time = time1 - time0 - timer_overhead;
	time /= 1000;
	
	printf("%d random seeks took %d for %d avg seek\n",npasses-1,time,time/(npasses-1));
}

/* measure the repeatability of the testing */
void repeat_test(int start, int end, int npasses)
{
	int		n;
	int		s;
	uint32	time;
	int		maxs;

	if (start < 0)
		start = 17*60*75;
	if (end < 0)
		end = 42*60*75;
	if (npasses < 0)
		npasses = 10;
	
	maxs = 5+sectors_per_track(sector_to_track(end));
	
	printf("from %d to %d through %d to %d\n",start,end,start+maxs-1,end);
	printf("from t%d to t%d through t%d to t%d\n",int2time(start),int2time(end),int2time(start+maxs-1),int2time(end));

	for(n = 0; n < npasses; n++)
	{
		for(s = 0; s < maxs; s++)
		{
			time = time_seek(start-s, end) / 1000;
			printf("\t%d", time);
		}
		printf("\n");
	}		
}


/* Find worst seeks for tracks based on sectors */
#define MAX_TRACK_ARRAY (50) 

void log_seeks( int blocks)
{
	int tracks ;	
	int ftime, rtime;
	int max_tracks;
	int track_lengths[MAX_TRACK_ARRAY];
	int mode; /* zero = from zero to track, 1 = around middle, 2 = from outer to track */
	int mti;
	int start;
	int	end;

/* build list of max tracks */

	max_tracks = sector_to_track(blocks);
	tracks = max_tracks;
	mti = 0;
	while (tracks > 0)
	{
		track_lengths[mti] = tracks;
		tracks = (tracks*1000)/1300;
		
		mti++ ;
		if (mti == MAX_TRACK_ARRAY)
		{
			printf("Error: Array to small\n");
			exit(9);
		}
	}

	while (mti--)
	{
		tracks = track_lengths[mti];
		printf("%d",tracks);
		
		for (mode = 0; mode < 3; mode++)
		{
			switch (mode)
			{
				case 0:
					start = 0;
					end = tracks;
					break;
				case 1:
					start = max_tracks/2-tracks/2;
					end   = max_tracks/2+tracks/2;
					break;
				case 2:
					start = max_tracks-tracks;
					end = max_tracks;
					break;
				default:
					printf("Log_seeks internal puke\n");
					exit(9);
			}
#if (1)
			get_minmin(track_to_sector(start), track_to_sector(end), &ftime, &rtime); 
#else
			ftime = start *1000;
			rtime = end*1000;
#endif
			printf("\t%d\t%d", ftime/1000, rtime/1000);
		}
		printf("\n");
	}
}

 
void miss_seeks(int from, int to, int npass, int blocks)
{
	int i;
	int time;
	/* number of histogram slots */
#define HIST 1000
	/* worst case max seek time in ms */
#define MAXTIME 1000
	int results[HIST] ;
	
	for (i = 0; i < HIST; i++)
		results[i] = 0;
	
	while ((to > blocks) || (from > blocks))
	{
		{
			static int flag = 0;

			if (!flag)
			{
				flag++;
				printf("Adjusting for disc \n");
			}
		}
		from--;
		to--;
	}
	if ((to < 0) || (from < 0))
	{
		printf("disc too small\n");
		from = 0;
		to = blocks;
	}
	
	while (TRUE)
	{
		time = (int)time_seek(from, to);
		time = (time+500)/1000;			/* convert to ms */
		if (time > MAXTIME)
		{
			time = MAXTIME;
			printf("Clipping extra long time \n");
		}
		i = (time*HIST)/MAXTIME;		/* calculate slot */
		results[i] ++;
		printf("TIME:HITS:\t%d\t%d\n",(i*MAXTIME)/HIST,results[i]);

		if (results[i] >= npass)
			break;
	}

	printf("\n\n");
	
	for (i = 0; i < HIST; i++)
	{
		if (results[i])
			printf("TIME:HITS:\t%d\t%d\n",(i*MAXTIME)/HIST,results[i]);
	}
	
}

void long_seeks(int starts, int blocks)
{
	int s, d;
	int min, max;
	int dist;
	int start, end;
	
	for (d = -starts; d <= starts; d++)
	{
		dist = (blocks * d) / starts;
		
		printf("%08d",dist);
	
		for(s = 0; s <= starts; s++)
		{
			start = (blocks * s) / starts;  /* calc start location on the disc */
			end = start + dist;
			
			min = 0;
			max = 0;
			
			if ((end >= 0) && (end <= blocks))
			{
				get_minmax(start, end, &min, &max);
				printf("\t%d\t%d",min/1000,max/1000); 
			}
			else
				printf("\txxx\txxx");
		}
		printf("\n");
	}
}

void short_seeks(int blocks, int starts, int dels)
{
	int s;
	int min, max;
	int dist;
	int start, end;
	int vs, ve, vl;
	
	vs = dels;
	ve = blocks - dels;
	vl = ve-vs;

	printf("%8d",0);
	for(s = 0; s <= starts; s++)
	{
		start = (100*s)/starts;
		printf(" v%03d ^%03d",start,start);
	}	
	printf("\n");
	
	for(dist = -dels/4; dist <= dels; dist++)
	{
		printf("%08d", dist);
		for(s = 0; s <= starts; s++)
		{
			start = vs+(vl * s) / starts;  /* calc start location on the disc */
			end = start+dist;
			get_minmax(start, end, &min, &max);
			printf("\t%4d\t%4d",min/1000,max/1000); 
		}
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	int blocks;
	int npass = -1;
	int nstart = -1;
	int argi;
	int from = -1;
	int to = -1;
	int dels = -1;
	uint32 time;
	int log_seek = FALSE;
	int linear_rate = FALSE;
	int one_seek = FALSE;
	int repeat = FALSE;
	int long_seek = FALSE;
	int short_seek = FALSE;
	int miss_seek = FALSE;
	int rand_seek = FALSE;
	int play = FALSE;
	int qflag;
	char *DeviceName = "cdrom";
	
#define DOUBLEARG(string, variable, function)	\
	if (streq(argv[argi],string)) { variable = function(argv[++argi]); }
#define SINGLEARG(string, variable, value)		\
	if (streq(argv[argi],string)) { variable = value; qflag = FALSE; }

	qflag = TRUE;
	
	printf("\n");
	for (argi = 0; argi < argc; argi++)
		printf("%s ",argv[argi]);
	printf("\n");
	
	for (argi = 1; argi < argc; argi++)
	{
		SINGLEARG("-log", log_seek, TRUE)
		else SINGLEARG("-repeat", repeat, TRUE)
		else SINGLEARG("-lin", linear_rate, TRUE)
		else SINGLEARG("-long", long_seek, TRUE)
		else SINGLEARG("-short", short_seek, TRUE)
		else SINGLEARG("-miss", miss_seek, TRUE)
		else SINGLEARG("-one", one_seek, TRUE)
		else SINGLEARG("-rand", rand_seek, TRUE)
		else SINGLEARG("-verbose", verbose, TRUE)
		else SINGLEARG("-play", play, TRUE)
		else DOUBLEARG("-pass",npass,(int)myatoi) 
		else DOUBLEARG("-start",nstart,(int)myatoi)
		else DOUBLEARG("-from",from,(int)myatoi)
		else DOUBLEARG("-to",to,(int)myatoi)
		else DOUBLEARG("-dels",dels,(int)myatoi)
		else DOUBLEARG("-dev",DeviceName,(char*))
		else 
		{
			printf("Unknown arg %s\n",argv[argi]);
			qflag = TRUE;
			break;
		}
	}
	
#define lprintf(x) printf("%s\n",x);

	if (qflag)
	{
		lprintf("Seek tester")
		lprintf("Test Types: Choose one of these...")
		lprintf("	-log	=>  performs lots of seeks of various sizes log spaced in length.")
		lprintf("				prints MINIMUM VALUES for seeks across various lengths of tracks")
		lprintf("	")
		lprintf("	-repeat =>	performs a series of seeks between -from and -to, -from an -to+1, -from and")
		lprintf("				-to+2... etc... Used to determine how 'repeatable' the drive is.....")
		lprintf("	")
		lprintf("	-lin	=>	attempts to measure the linear velocity of the disc. Kinda flakey..")
		lprintf("	")
		lprintf("	-rand	=>	performs a series of random seeks to determine overall seek time")
		lprintf("	")
		lprintf("	-one	=>	measures a single seek ( -pass times ) between -from and -to.")
		lprintf("	")
		lprintf("	-long 	=>  invokes long seeks ( -nstart is only other arg ) ")
		lprintf("				performs series of linearly spaced seeks from nstart locations")
		lprintf("				on disc, to every other nstart locations. ")
		lprintf("	-short	=>  invokes short seeks")
		lprintf("				performs series of sector based seeks, -nstart and -ndels are")
		lprintf("	")
		lprintf("	-play	=>  plays from to allowing one to check buffer repeatability")
		lprintf("	")
		lprintf("Test Parameters ( these values will be used where applicable )")
		lprintf("	-from 	#	=> sets a starting location.")
		lprintf("	-to 	#	=> sets a ending location.")
		lprintf("	-pass 	#	=> sets number of iterations.")
		lprintf("	-dev 	string => sets cd-rom device.")
		lprintf("	")
		lprintf("	Parameters can be an integer, or a mmssff time ( if preceeded by a 't' ).")
		lprintf("	")
		
		exit(0);
	}
	
	
	/* open and init cdrom device */
	 
	blocks =  MyCDOpen(TRUE, MyCDROM, (unsigned char *)DeviceName);
	if (blocks < 0) 
		exit(99);
	blocks -= READMINBLOCK;
	blocks -= READMINBLOCK;
	
	printf("CDROM open.. blocks = %d (time = %d min)\n",blocks, blocks/( 75*60) );

	if (play)
	{
		if (from == -1)
			from = 150;
		if (to == -1) 	
			to = blocks - 150;
		if (to < from) 
			to = from + 1;
		
		playfunction( from, to );
		exit(99);
	}
	
	if (one_seek)
	{
		if (from == -1)
			from = 0;
		if (to == -1 ) 
			to = blocks;
		if (npass == -1)
			npass = 1;
		while (npass > 1)
		{
			time = time_seek(from, to);
			printf("time to seek from %ld to %ld is %ld \n",from,to,time);
		}
		exit(0);
	}

	if (rand_seek)
	{
		rand_seeks(blocks, npass);
		exit(0);
	}

	if (repeat)
	{
		repeat_test(from, to, npass);
		exit(0);
	}

	if (linear_rate)
	{
		linear_rate_test();
		exit(0);
	}

	if (log_seek)
	{
		log_seeks(blocks);
		exit(0);
	}

	if (long_seek)
	{
		if (nstart < 0)
			nstart = 5;
		long_seeks(nstart, blocks);
		exit(0);
	}
	
	if (short_seek)
	{
		if (nstart < 0)
			nstart = 5 ;
		if (dels < 0) 
			dels = 32 ;
		short_seeks(blocks, nstart, dels);
		exit(0);
	}
	
	if (miss_seek)
	{
		if (from < 0)
			from = blocks/2 - 55500 ;	/* 1/3 */
		if (to < 0)
			to = blocks/2 + 55500;		/* 2/3 */
		if (npass < 0)
			npass = 100;
		miss_seeks (from, to, npass, blocks);
		exit(0);
	}

	printf("SeekTest parser confused. Try seek -h\n");
	exit(99);
}
