typedef struct TimerBlock {
    TimerTicks start, end;	
    uint32 timescalled;
    uint32 totaltriangles;
    float accumtime;
    float totaltime;
} TimerBlock;
 
typedef struct Report {
    uint32 podcount0;
    float fps;
    float tps_screen,tps_scene;
} Report;

extern TimerBlock tb[2];
void GameCodeInit(MpmContext *);
void GameCode(MpmContext *mpmc);
bool AnalyzeRealTime(Report*,uint32 podcount, uint32 numtriangles);
void AnalyzeTime(TimerBlock *ptb,uint32 numtri,char*);
void MoveCameraInit(CloseData*);
void MoveCamera(CloseData *pclosedata);

