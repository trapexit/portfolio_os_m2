/* @(#) main.c 96/09/10 1.20 */

#include <kernel/types.h>
#include <kernel/time.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/cache.h>
#include <hardware/PPCasm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern void Perm(void);
extern void Towers(void);
extern void Queens(void);
extern void Intmm(void);
extern void Realmm(void);
extern void Puzzle(void);
extern void Quick(void);
extern void Bubble(void);
extern void Trees(void);
extern void Oscar(void);
extern Item CreateNULLDriver(void);
extern void NullSysCall(void);

extern void DoTaskTest(void);

#define INT_ON 0
#define INT_OFF 1
#define FULL_SUITE 0

enum {
  PERM=0,
  TOWERS,
  QUEENS,
  INTMM,
  REALMM,
#if FULL_SUITE
  PUZZLE,
  QUICK,
  BUBBLE,
  TREES,
#endif
  OSCAR,
  NUM_TESTS
};

/* the following are the speeds as of 7/11/95 build */
float speeds[NUM_TESTS] = {
  0.269043, /* PERM */
  0.320068, /* TOWERS */
  0.182129, /* QUEENS */
  0.192383, /* INTMM */
  0.296143, /* REALMM */
#if FULL_SUITE
  1.418457, /* PUZZLE */
  0.193848, /* QUICK */
  0.187256, /* BUBBLE */
  1.065353, /* TREE */
#endif
  0.338623, /* FFT */
};

float totalTimeOn;
float totalTimeOff;

typedef struct Benchmark {
  void (*func)(void);
  char name[15];
  float int_on_time;
  float int_off_time;
  float rel_speed;
} Benchmark;

Benchmark tests[NUM_TESTS] = { Perm,"Permutation",0.0,0.0,0.0,
				    Towers,"Towers",0.0,0.0,0.0,
				    Queens,"Queens",0.0,0.0,0.0,
				    Intmm,"IntMatMul",0.0,0.0,0.0,
				    Realmm,"FloatMatMul",0.0,0.0,0.0,
#if FULL_SUITE
				    Puzzle,"Puzzle",0.0,0.0,0.0,
				    Quick,"QuickSort",0.0,0.0,0.0,
				    Bubble,"BubbleSort",0.0,0.0,0.0,
				    Trees,"TreeSort",0.0,0.0,0.0,
#endif
				    Oscar,"FFT",0.0,0.0,0.0 };


static void startclock(TimerTicks *tt)
{
    SampleSystemTimeTT(tt);
}

static float stopclock(TimerTicks *start)
{
TimerTicks end;
TimerTicks diff;
TimeVal    tv;

    SampleSystemTimeTT(&end);
    SubTimerTicks(start, &end, &diff);
    ConvertTimerTicksToTimeVal(&diff, &tv);

    return tv.tv_Seconds + (tv.tv_Microseconds / 1000000.0);
}


void PrintHeader(int32 n,char *str)
{
  printf("%d %-30s",n,str);
}

void PrintAvg(float time,int32 n)
{
  printf(" avg time: %f usecs\n",n,(time/(float)n)*1000.0*1000.0);
}

typedef Err (* CallBackFunc)(uint32,uint32,uint32);

static void DoBenchmarkNoInts(int n)
{
  float time;
  TimerTicks start;
  int i;
  uint32 oldints;

  oldints = Disable();

  startclock(&start);
  for(i = 0 ; i< 10 ; i++) {
    (*tests[n].func)();
  }
  time = stopclock(&start);

  Enable(oldints);

  tests[n].int_off_time = time;
  totalTimeOff = totalTimeOff + time;
}

static void DoBenchmarkWithInts(int n)
{
  float time;
  TimerTicks start;
  int i;

  startclock(&start);
  for(i = 0 ; i< 10 ; i++) {
    (*tests[n].func)();
  }
  time = stopclock(&start);
  tests[n].int_on_time = time;
  totalTimeOn= totalTimeOn + time;
  tests[n].rel_speed = speeds[n]/time;
}

void DoCPUTests(void)
{
  int i;
  float refTotal = 0.0;

  printf("\n\nRunning Stanford benchmark suite (takes about 5 seconds)...\n\n");
  for(i=0;i<NUM_TESTS;i++) {
    CallBackSuper((CallBackProcPtr)DoBenchmarkNoInts, i, 0, 0);
    DoBenchmarkWithInts(i);
    refTotal = refTotal + speeds[i];
  }

  printf("\nAll times in seconds\n");
  printf("Score > 1 means test faster than latest reference.  < 1 is slower\n\n");
  printf("%-15s \t%s \t%s \t%s \t    %s\n","Benchmark","Ints on  ","Ints off","Diff","Score");
  printf("%-15s \t%s \t%s \t%s \t    %s\n","---------","-------  ","--------","----","-----");
  for(i=0;i<NUM_TESTS;i++) {
    printf("%-15s \t%f \t%f \t%f%c\t%f\n",
	   tests[i].name,
	   tests[i].int_on_time,
	   tests[i].int_off_time,
	   100*(tests[i].int_on_time-tests[i].int_off_time)/tests[i].int_on_time,'%',
	   tests[i].rel_speed);
  }
  printf("\nOverall score: %4.2f\n\n",refTotal/totalTimeOn);
}

uint32 buf[2];

void DoDriverTest(void)
{
  Item drvr;
  Item req;
  IOInfo info;
  Err err;
  float time;
  TimerTicks start;
  int n = 1000;
  int i;

    drvr = OpenNamedDeviceStack("null,1");
    if(drvr>=0) {
      req = CreateIOReq(0,0,drvr,0);
      if(req < 0) {
	CloseDeviceStack(drvr);
	return;
      }

      bzero( (void *)&info, sizeof(IOInfo));
      info.ioi_Command = CMD_STATUS;
      info.ioi_Send.iob_Buffer	= (uchar *) &buf;
      info.ioi_Send.iob_Len	= sizeof(buf);

      PrintHeader(n,"DoIO calls to null driver...");
      startclock(&start);
      for(i=0;i<n;i++) {
	err = DoIO(req,&info);
	if(err!=0)
	  printf("Error (%x) in iteration %d of call to NullDriver\n",err,i);
      }
      time = stopclock(&start);
      PrintAvg(time,n);
      DeleteIOReq(req);
      CloseDeviceStack(drvr);
    }
}

uint32 cacheme[1024];

static void CacheTest(float *result, int n)
{
uint32 oldints;
float time,total;
TimerTicks start;
int i,j;

  oldints = Disable();

  total = 0.0;
  for(i=0;i<n;i++) {
    startclock(&start);
    FlushDCacheAll(0);
    time = stopclock(&start);
    total+=time;
    for(j=0;j<1024;j++)
      cacheme[j]=i;
  }

  Enable(oldints);

  *result = total;
}

void DoCacheTest(void)
{
float total;

  PrintHeader(1000,"Data cache flushes...");

  CallBackSuper((CallBackProcPtr)CacheTest, (uint32)&total, 1000, 0);

  PrintAvg(total,1000);
}

void DoSysCallTest(void)
{
  float time;
  TimerTicks start;
  int i,n=1000;

  PrintHeader(n,"Null system calls...");
  startclock(&start);
  for(i=0;i<n;i++) {
    NullSysCall();
  }
  time = stopclock(&start);
  PrintAvg(time,n);
}

void DoOSTests(void)
{
  printf("Doing OS tests...\n");
  DoDriverTest();
  DoCacheTest();
  DoSysCallTest();
  DoTaskTest();
}

int main(void)
{

  printf("\n\n Welcome to M2Bench\n");

  DoCPUTests();
  DoOSTests();

  printf("\nm2bench complete\n");

  return 0;
}
