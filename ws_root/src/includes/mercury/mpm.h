typedef struct MpmPodManager {
    uint32 	podcount0;	/* number of Pods on CPU0 */
    uint32 	needsort;
    Pod 	*podbase;
    Matrix	*matrixbase;
    Pod 	*firstpod[2]; /* CPU0 CPU1 */
} MpmPodManager;

typedef struct MpmSlaveBlock {
    uint32	pad0[8];
    Pod 	*ppod;		/* Assure that data is insulated by 1 cache line */
    CloseData 	*pclosedata;
    uint32 	returned_count;
    uint32	latency;
    uint32 	pad1[8];	/* Insulate with a cache line of data */
} MpmSlaveBlock;

typedef struct MpmContext {
    Item 		mpioreq;
    uint32 		whichframe;
    uint32		podpiperet0;	/* returned from CPU 0 rendering */ 
    uint32 		podcount;	/* max number of pods in either frame */
    float		*xformbuffer[2];/* CPU 0, CPU 1 */
    MpmPodManager 	pm[2]; 		/* Frame 0 Frame 1 */
    CloseData 		*pclosedata[3];	/* CPU 0, CPU 1 Frame 0, CPU 1 Frame 1 */
    GState    		*gs[2];		/* CPU 0 CPU 1 */
    MpmSlaveBlock 	slavedata;	/* Data passed to slave */
    uint32		*stack;		/* Stack used in slave call */
    Err			status;		/* Returned from slave call */
} MpmContext;


#define  	MPM_GetCloseData(mpmc) ((mpmc)->pclosedata[0])
#define 	MPM_Getpodbase(mpmc) (mpmc)->pm[(mpmc)->whichframe].podbase
#define 	MPM_Getmatrixbase(mpmc) (mpmc)->pm[(mpmc)->whichframe].matrixbase
#define		MPM_Getpod(mpmc,i) (MPM_GetPodBase(mpmc)+i)
#define		MPM_Getmatrix(mpmc,i) (MPM_GetMatrixBase(mpmc)+i)
#define		MPM_Getpodcount0(mpmc) (mpmc)->pm[(mpmc)->whichframe].podcount0
#define		MPM_Getpodcount(mpmc) (mpmc)->podcount
#define		MPM_Getfirstpod(mpmc,cpu) (mpmc)->pm[(mpmc)->whichframe].firstpod[cpu]
#define 	MPM_Getneedsort(mpmc) (mpmc)->pm[(mpmc)->whichframe].needsort
#define		MPM_Getxformbuffer(mpmc,cpu) (mpmc)->xformbuffer[cpu]

#define  	MPM_SetCloseData(mpmc,cd) (mpmc)->pclosedata[0]=cd
#define 	MPM_Setpodbase(mpmc,pb) (mpmc)->pm[(mpmc)->whichframe].podbase=pb
#define 	MPM_Setmatrixbase(mpmc,mb) (mpmc)->pm[(mpmc)->whichframe].matrixbase=mb
#define		MPM_Setpod(mpmc,i,p) *(MPM_GetPodBase(mpmc)+i)=p
#define		MPM_Setmatrix(mpmc,i,m) *(MPM_GetMatrixBase(mpmc)+i)=m
#define		MPM_Setpodcount0(mpmc,c) \
				(mpmc)->pm[0].podcount0=c; \
				(mpmc)->pm[0].needsort=1; \
				(mpmc)->pm[1].podcount0=c; \
				(mpmc)->pm[1].needsort=1

#define		MPM_Setpodcount(mpmc,c)  \
				(mpmc)->pm[(mpmc)->whichframe].podcount=c; \
				(mpmc)->pm[(mpmc)->whichframe].needsort=1

#define		MPM_Setfirstpod(mpmc,cpu,fpod)  \
				(mpmc)->pm[(mpmc)->whichframe].firstpod[cpu]=fpod
#define 	MPM_Setneedsort(mpmc,ns) (mpmc)->pm[(mpmc)->whichframe].needsort=ns
#define		MPM_Setxformbuffer(mpmc,cpu, x) (mpmc)->xformbuffer[cpu]=x

void		MPM_NewFrame(MpmContext*);

/*		Initializes data in context */
void		MPM_Init(MpmContext*);

/* 		Copies pods and matrices from frame 0 into frame 1 */ 
void 		MPM_DuplicatePods(MpmContext *mpmc);

/* 		Splits the list according to the break */
void		MPM_Split(MpmContext*);

/* 		Utility that links, sorts and splits */
void		MPM_LinkSortSplit(MpmContext*);

/* 		returns triangle count from last invocation of MPM_Draw */
uint32 		MPM_DrawSendSwap(MpmContext*, Item newbitmap); 

/* 		uses low latency to draw */
uint32 		MPM_DrawSendSwapLL(MpmContext*, Item newbitmap); 

/* 		Makes CloseData for CPU1 match that for CPU0 */
void 	       	MPM_SyncCloseData(MpmContext *);

/* 		returns triangle count from last call to MPM_DrawSendSwap but
		does not start pipe for another frame */
uint32		MPM_LastFrame(MpmContext*,Item newbitmap);

/*		Sets the xformbuffers on all pods for the current frame */
void		MPM_SetPodBuffers(MpmContext*);

/* Calling sequence is:

loop:
   MPM_NewFrame
   MPM_GetPodBase
   clear frame buffer
   game code
   MPM_SyncCloseData
   MPM_LinkSortSplit
   MPM_DrawSendSwap
 endloop

   MPM_EndFrame /* for last frame */


/* To be removed */
void InitSendAndSwap(Item viewItem);
