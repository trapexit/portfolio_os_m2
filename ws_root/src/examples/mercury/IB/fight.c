#include "fight.h"
Ptcl		Player[2];
ObjDetail	Objects[36];
PlayerMove	PMoves[2][MOVEKEYS];
AnimDetail	AnimDetails[2][NUMOFMOVES]; /* header data for all moves */
short		PosOffset[NUMOFPOSFRAMES * 3]; /* position data for all moves */
ObjAnim		AnimData[NUMOFANIMFRAMES]; /* animation data for all moves */
PlatformStruct	Platform;
OptionsRec	Options;
SVECTOR		zerovec	= {0, 0, 0};
SVECTOR		CameraPosition	= {0};
ushort		nextPoof 			= 0;
ushort          nextDustPoof                    = 0;
int32		RuneOffset			= -0x000F0000;
int32		rune				= 0x00FF0000;
short		CombatTimer			= 60;
short		GameInProgress		= 1;
ulong		PlayerSep			= 0;
uchar		FlameArray[28]		= {0};

/* Prototypes for routines only used in this file */
/* Carries around all the program's data */
typedef struct AppData {
  CloseData   *close;
  Matrix      *matrixSkew;
  Matrix      *matrixCamera;
  
  uint32     curRenderScreen;
  float      rx, ry, rz;        /* rotation of object */
  float      tx, ty, tz;        /* location of object */
  
  ControlPadEventData	cped;

} AppData;
/************************************************************/


/*
 * Shadow Defines
 */

#ifdef DRAW_SHADOWS
#define RC_SHADOW_SIZE 1.0/SHADOW_SIZE
#define RC_SHADOW_SEG  1.0/SHADOW_SEG

#ifndef SINGLE_SHADOW
float  Shad_Player_Dist;
#define FIXED_CAMERA_HEIGHT 180
#endif

#ifdef RENDER_SHADOW

#define SHADOW_USE_MP
#undef SHADOW_USE_LOW_LATENCY

#ifndef SINGLE_SHADOW
GraphicsEnv  *shadowEnv1;
AppData      shadowApp1;
AppData      *shadowData1 = &shadowApp1;
#endif

#define kNumCmdListBufs     3
#define kCmdListBufSize     40*1024

GraphicsEnv  *shadowEnv0;
AppData      shadowApp0;
AppData      *shadowData0 = &shadowApp0;
extern float shadowVertex[];

#endif  /* RENDER_SHADOW */

#ifdef POD_SHADOW
Pod          *shadowPod0;
Pod          *shadowPod1;
#endif

#ifdef BLINN_SHADOW
BlinnShadow shadowTrans[2][20];
#endif

#endif /* DRAW_SHADOWS */

#define  USE_BLITTER_FLAME
#define  USE_BLITTER_BG
#define  USE_MP
#define  USE_STENCIL
#define   USE_LOWLATENCY

#ifdef USE_BLITTER_FLAME
#define USE_BLITTER
#else
#ifdef USE_BLITTER_BG
#define USE_BLITTER 
#endif
#endif

#define IB_SCREENWIDTH  640
#define IB_SCREENHEIGHT 480
#define IB_SCREENDEPTH  32
#define IB_BACKHEIGHT   300     /* Probably overkill, but just to be safe */

#ifdef RENDER_SHADOW
#ifdef FAKE_SHADOW
#define IB_SHADOW_WIDTH  128
#define IB_SHADOW_HEIGHT 128
#else
#define IB_SHADOW_WIDTH  176
#define IB_SHADOW_HEIGHT 176
#define IB_SHADOW_DEPTH  16
#endif
#endif

#define IB_SCREENWIDTH  640
#define IB_SCREENHEIGHT 480
#define IB_SCREENDEPTH  32

#define IB_BG_W         0.998
#define IB_BG_WIDTH     320   
#define IB_BG_HEIGHT    150   

void WaitForAnyButton(ControlPadEventData *cped)
{
    GetControlPad(1, TRUE, cped); /* Button down */
    GetControlPad(1, TRUE, cped); /* Button up */
}

BlitMatrix bltMat = 
{ 1, 0,
  0, 1,
  0, 0
};


void SetDBlendAttr(BlitterSnippetHeader *snippets[], uint32 type, uint32 value);
void SetTextureAttr(BlitterSnippetHeader *snippets[], uint32 type, uint32 value);

/************************************************************/
Matrix* Char_Obj[2][20];

static BlitObject*	FlamePot0;
static BlitObject*	FlamePot1;
static BlitObject*	Flame0;
static BlitObject*	Flame1;

static 	Point2 P2_Pot0		=	{12, 150};
static 	Point2 P2_Pot1		=	{560, 150};
static 	Point2 P2_Flame0	=	{15, 165};
static 	Point2 P2_Flame1	=	{557, 165};

/*Bitmap* Video_Buffer; */

/* Y value below which a dust cloud will be generated */
#define DustY 5

/* flame stuff */

#define kDecay       12    /* lower = taller flames */
#define kSmooth      1
#define kXleft       5     /* left edge of the bottom flame */
#define kXright      23    /* right edge of the bottom flame */
#define kDeadzone    5
#define kFlamewidth  18
#define kMaxY        49
#define kMaxX        27
#define RootRand     20
#define MinFire      160
#define FireIncrease 6

static uchar flame_buffer[kMaxY + 1][kMaxX + 1] = {0};

/* Mercury limits we have arbitrarily set -- lower if tight on mem */

/* #define kMaxCmdListWords    100000 */
#define kMaxCmdListWords    200000

#define kRadian             	(3.141592653589793/180.0)
#define Rand()	rand()

static ushort loadTMD(ushort PNum, Ptcl *player, ushort numtmds, ushort startpoly, ushort offset);
static short FireRand(void);
static void CalcFire(ushort Life0, ushort Max0, ushort Life1, ushort Max1);

gfloat Bg_Vertices[] = 
{
  0, 0, IB_BG_W, 0, 0,
  (IB_SCREENWIDTH-1), 0, IB_BG_W, (IB_BG_WIDTH-1), 0,
  (IB_SCREENWIDTH-1), (IB_BACKHEIGHT-1), IB_BG_W, (IB_BG_WIDTH-1), (IB_BG_HEIGHT-1),
  0, (IB_BACKHEIGHT-1), IB_BG_W, 0, (IB_BG_HEIGHT-1),
};

#ifdef USE_BLITTER_BG
static void DrawBG(BlitObject *blit, AppData* appData);
char *Bg_Address;
#endif

static void *LoadPCFile(char *fname, char *buf);
/*static void Load_BGImage(char *fname, char *buf); */
static void cbvsync(AppData* app);
static void PositionObjects(AppData* appData );
void print_pod(Pod *pod);

void Program_Init(	GraphicsEnv* genv, AppData** app);
void mainloop(	GraphicsEnv* genv, AppData* app);

static void Square0(Vector3 *first, Vector3 *second);				
static float DistanceSquared(Vector3 *first, Vector3 *second);
static void ProcessCamera(AppData* appData);

static void HitARune(Ptcl *p, ushort DrawExplosion, ushort playernum);
static void FaceOpponent(Ptcl *p1, Ptcl *p2, ushort playernum);
static ushort IsBehind(Ptcl *p1, Vector3 *v2);

static void UpdateCharPos(ushort playernum, Ptcl *p, Ptcl *otherp);
static void RecalcProbabilities(uchar *AttackProb, ushort movenum, short amount);
static void JumpToFrame(Ptcl *p, ushort currframe, ushort targetframe);
static void SetMove(Ptcl *p, ushort playernum, ushort Move, ushort StartAtLoop);
static void RestartLoop(Ptcl *p, ushort playernum, ushort Move);
static short ChooseOffensiveMove(Ptcl *p, Ptcl *otherp);
static short ChooseDefensiveMove(Ptcl *p, Ptcl *otherp);
static void CalcAImove(Ptcl *p, Ptcl *otherp, ushort LoopEnded);
static void ProcessInput(ushort id);
static void CheckForHits(Ptcl *p, Ptcl *otherp);
static void ProcessHits(void);

Vector3D CameraPosition;

uint32         modelPodCount;
uint32         modelPodCount2;
uint32         modelPodCount3;
uint32         modelPodCount4;
uint32         modelPodCount5;

static ushort paddbuffidx;
static ulong paddbuffer[8];
static Poof poofs[MAX_POOFS]; /* explosions */

static Poof dustPoofs[MAX_DUST];

/* static	uint32	char0 = 0;  /* Torgo */
/* static	uint32	char0 = 1;  /* Luthor */
/* static	uint32	char0 = 2;  /* Erland */
/* static	uint32	char0 = 3;  /* Darius */
static	uint32	char1 = 4; /* XENO */
/* static	uint32	char0 = 5;  /* Ignatius */
/* static	uint32	char0 = 6;  /* Shinestra */
/* static	uint32	char0 = 7;  /* RedCloud */
/* static	uint32	char0 = 8; /* URGO */
/* static	uint32	char0 = 9; /* ARDRUS */
/* static	uint32	char0 = 10;  /* Sasha DONT WORK*/
static	uint32	char0 = 11;  /* Nym */
/* static	uint32	char0 = 12; /* BALTH */
/* static	uint32	char0 = 13; /* Kaurik */
/* static	uint32	char0 = 14; /* Balok */
/* static	uint32	char1 = 15;  /* Stellerex */
/* static	uint32	char1 = 16;  /* Minion of Order */
/* static	uint32	char1 = 16;  /* Lord of Order */
/* static	uint32	char1 = 18;  /* Minion of Chaos */


uchar AttackTable[5][15] = {
  {40, 25, 15, 20, 5,   /* level 0 high */
   40, 25, 15, 20, 5,   /* level 0 medium */
   40, 25, 15, 20, 5},  /* level 0 low */
  {35, 23, 22, 20, 16,  /* level 1 high */
   35, 23, 22, 20, 16,  /* level 1 medium */
   35, 23, 22, 20, 16}, /* level 1 low */
  {28, 22, 30, 20, 27,  /* level 2 */
   28, 22, 30, 20, 27,  /* level 2 */
   28, 22, 30, 20, 27}, /* level 2 */
  {22, 21, 37, 20, 38,  /* level 3 */
   22, 21, 37, 20, 38,  /* level 3 */
   22, 21, 37, 20, 38}, /* level 3 */
  {15, 20, 45, 20, 50,  /* level 4 */
   15, 20, 45, 20, 50,  /* level 4 */
   15, 20, 45, 20, 50}  /* level 4 */
};

struct PlayerAnim {
  ulong overhead[5];
  /*	short data[480][512]; /* buffer that player anims get dumped into */
  uchar *data;		  /* 3bytes long of array[480][512] which is part of the  */
  /* texture in the SpriteObj  */
} PlayerAnim;

typedef struct {
  char   bg[50];
  char   pmodel[50];
  char   shadow[50];
  char   animdata[50];
  char   animdetails[50];
  char   posdata[50];
  char   keydata[50];
  ushort circular;
  ushort lawchar;
  char   pltex0[50];
  char   pltex1[50];
  char   textures[12][50];
  ushort numofobjs;
  ushort height;
  ushort damageR;
  ushort armor;
  ushort weapon1;
  ushort weapon2;
} PlayerData;

static PlayerData PData[20] = {
  {"Ironblood/BG/BG-TORG.utf",
   "Ironblood/Character/TORGO/TORG-180",
   "TORG-180_world",
   "Ironblood/Character/TORGO/XFORMS.DAT",
   "Ironblood/Character/TORGO/ANIMDETL.DAT",
   "Ironblood/Character/TORGO/POSITION.DAT",
   "Ironblood/Character/TORGO/KEYS.DAT",
   0, 1,
   "Ironblood/platform/TORGO/BASE",
   "Ironblood/platform/TORGO/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   14, 182, 4, 4, 9, -1
  },
  {"Ironblood/BG/BG-LUTH.utf",
   "Ironblood/Character/Luthor/LUTHC180",
   "XXXXXXXXXXX",
   "Ironblood/Character/Luthor/dluthor.dat",
   "Ironblood/Character/Luthor/aluthor.dat",
   "Ironblood/Character/Luthor/pluthor.dat",
   "Ironblood/Character/Luthor/KEYS.DAT",
   1, 1,
   "Ironblood/platform/Luthor/BASE",
   "Ironblood/platform/RING/BLUE_CR/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 247, 4, 5, 14, -1
  },
  {"Ironblood/BG/BG-ERLA.utf",
   "Ironblood/Character/ERLAND/ERLA-180",
   "ERLA-180_world",
   "Ironblood/Character/ERLAND/XFORMS.DAT",
   "Ironblood/Character/ERLAND/ANIMDETL.DAT",
   "Ironblood/Character/ERLAND/POSITION.DAT",
   "Ironblood/Character/ERLAND/KEYS.DAT",
   1, 1,
   "Ironblood/platform/ERLAND/BASE",
   "Ironblood/platform/ERLAND/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 230, 3, 2, 12, 14
  },
  {"Ironblood/BG/BG-DARI.utf",
   "Ironblood/Character/DARIUS/DARI-180",
   "DARI-180_world",
   "Ironblood/Character/DARIUS/XFORMS.DAT",
   "Ironblood/Character/DARIUS/ANIMDETL.DAT",
   "Ironblood/Character/DARIUS/POSITION.DAT",
   "Ironblood/Character/DARIUS/KEYS.DAT",
   1, 1,
   "Ironblood/platform/DARIUS/BASE",
   "Ironblood/platform/DARIUS/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 256, 4, 5, 14, -1
  },
  {"Ironblood/BG/BG-XENO.utf",
   "Ironblood/Character/Xenobia/XENO-180",
   "XENO-180_world",
   "Ironblood/Character/Xenobia/XFORMS.DAT",	/* satoru */
   "Ironblood/Character/Xenobia/ANIMDETL.DAT",
   "Ironblood/Character/Xenobia/POSITION.DAT",
   "Ironblood/Character/Xenobia/KEYS.DAT",
   0, 1,
   "Ironblood/platform/Xenobia/BASE",
   "Ironblood/platform/Xenobia/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 241, 3, 3, 11, 13
  },
  {"Ironblood/BG/BG-IGNA.utf",
   "Ironblood/Character/IGNATIUS/IGNA-180",
   "IGNA-180_world",
   "Ironblood/Character/IGNATIUS/XFORMS.DAT",
   "Ironblood/Character/IGNATIUS/ANIMDETL.DAT",
   "Ironblood/Character/IGNATIUS/POSITION.DAT",
   "Ironblood/Character/IGNATIUS/KEYS.DAT",
   1, 1,
   "Ironblood/platform/IGNATIUS/BASE",
   "Ironblood/platform/IGNATIUS/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 171, 2, 2, 11, -1
  },
  {"Ironblood/BG/BG-SHIN.utf",
   "Ironblood/Character/SHINESTA/SHIN-180",
   "SHIN-180_world",
   "Ironblood/Character/SHINESTA/XFORMS.DAT",
   "Ironblood/Character/SHINESTA/ANIMDETL.DAT",
   "Ironblood/Character/SHINESTA/POSITION.DAT",
   "Ironblood/Character/SHINESTA/KEYS.DAT",
   1, 1,
   "Ironblood/platform/SHINESTA/BASE",
   "Ironblood/platform/SHINESTA/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 224, 2, 3, 11, -1
  },
  {"Ironblood/BG/BG-REDC.utf",
   "Ironblood/Character/REDCLOUD/REDC-180",
   "REDC-180_world",
   "Ironblood/Character/REDCLOUD/XFORMS.DAT",
   "Ironblood/Character/REDCLOUD/ANIMDETL.DAT",
   "Ironblood/Character/REDCLOUD/POSITION.DAT",
   "Ironblood/Character/REDCLOUD/KEYS.DAT",
   0, 1,
   "Ironblood/platform/REDCLOUD/BASE",
   "Ironblood/platform/REDCLOUD/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 230, 5, 1, 14, 15
  },
  {"Ironblood/BG/BG-URGO.utf",
   "Ironblood/Character/URGO/URGO-180",
   "URGO-180_world",
   "Ironblood/Character/URGO/XFORMS.DAT",
   "Ironblood/Character/URGO/ANIMDETL.DAT",
   "Ironblood/Character/URGO/POSITION.DAT",
   "Ironblood/Character/URGO/KEYS.DAT",
   1, 0,
   "Ironblood/platform/URGO/BASE",
   "Ironblood/platform/URGO/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 164, 3, 5, -1, -1
  },
  {"Ironblood/BG/BG-ARDR.utf",
   "Ironblood/Character/ARDRUS/ARDR-180",
   "ARDR-180_world",
   "Ironblood/Character/ARDRUS/XFORMS.DAT",
   "Ironblood/Character/ARDRUS/ANIMDETL.DAT",
   "Ironblood/Character/ARDRUS/POSITION.DAT",
   "Ironblood/Character/ARDRUS/KEYS.DAT",
   1, 0,
   "Ironblood/platform/ARDRUS/BASE",
   "Ironblood/platform/ARDRUS/RING",
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 248, 3, 3, 12, -1
  },
  {"Ironblood/BG/BG-SASH.utf",
   "Ironblood/Character/SASHA/SASH-180.csf",
   "SASH-180_world",
   "Ironblood/Character/SASHA/XFORMS.DAT",
   "Ironblood/Character/SASHA/ANIMDETL.DAT",
   "Ironblood/Character/SASHA/POSITION.DAT",
   "Ironblood/Character/SASHA/KEYS.DAT",
   1, 0,
   "Ironblood/platform/SASHA/BASE.csf",
   "Ironblood/platform/SASHA/RING.csf",                         {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 188, 3, 2, -1, -1
  },
  {"Ironblood/BG/BG-NYM.utf",
   "Ironblood/Character/Nym/NYMC180",
   "NYMC180_world",
   "Ironblood/Character/Nym/XFORMS.DAT",	/* satoru */
   "Ironblood/Character/Nym/ANIMDETL.DAT",
   "Ironblood/Character/Nym/POSITION.DAT",
   "Ironblood/Character/Nym/KEYS.DAT",
   1, 0,
   "Ironblood/platform/Nym/BASE",
   "Ironblood/platform/Nym/RING",     
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 188, 2, 2, 10, 13
  },
  {"Ironblood/BG/BG-BALT.utf",
   "Ironblood/Character/Balthazzar/BALTC180",			 
   "BALTC180_world",
   "Ironblood/Character/Balthazzar/ANIM.DAT",
   "Ironblood/Character/Balthazzar/ANIMDETL.DAT",
   "Ironblood/Character/Balthazzar/POSITION.DAT",
   "Ironblood/Character/Balthazzar/KEYS.DAT",
   0, 0,
   "Ironblood/Platform/Balthazzar/BASE",
   "Ironblood/Platform/Ring/RED_SQ/RING",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 302, 5, 2, 16, -1
  },
  {"Ironblood/BG/BG-KAUR.utf",
   "Ironblood/Character/KAURIK/KAUR-180",
   "KAUR-180_world",
   "Ironblood/Character/KAURIK/XFORMS.DAT",
   "Ironblood/Character/KAURIK/ANIMDETL.DAT",
   "Ironblood/Character/KAURIK/POSITION.DAT",
   "Ironblood/Character/KAURIK/KEYS.DAT",
   1, 0,
   "Ironblood/platform/KAURIK/BASE",
   "Ironblood/platform/KAURIK/RING",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   18, 244, 3, 1, 10, 11
  },
  {"Ironblood/BG/BG-BALO.utf",
   "Ironblood/Character/BALOK/BALO-180",
   "BALO-180_world",
   "Ironblood/Character/BALOK/XFORMS.DAT",
   "Ironblood/Character/BALOK/ANIMDETL.DAT",
   "Ironblood/Character/BALOK/POSITION.DAT",
   "Ironblood/Character/BALOK/KEYS.DAT",
   1, 0,
   "Ironblood/platform/BALOK/BASE",
   "Ironblood/platform/BALOK/RING",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   16, 258, 4, 4, 7, 11
  },
  {"Ironblood/BG/BG-STEL.utf",
   "Ironblood/Character/STEL/STEL-180",
   "STEL-180_world",
   "Ironblood/Character/STEL/XFORMS.DAT",
   "Ironblood/Character/STEL/ANIMDETL.DAT",
   "Ironblood/Character/STEL/POSITION.DAT",
   "Ironblood/Character/STEL/KEYS.DAT",
   1, 0,
   "Ironblood/platform/STEL/BASE",
   "Ironblood/platform/STEL/RING",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 196, 4, 1, 14, -1
  },
  {"Ironblood/BG/BG-LAW.utf",
   "Ironblood/Character/MOFO/MOFO-180",
   "MOFO-180_world",
   "Ironblood/Character/MOFO/XFORMS.DAT",
   "Ironblood/Character/MOFO/ANIMDETL.DAT",
   "Ironblood/Character/MOFO/POSITION.DAT",
   "Ironblood/Character/MOFO/KEYS.DAT",
   1, 1,
   "Ironblood/platform/STEL/BASE.csf",
   "Ironblood/platform/STEL/RING.csf",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   16, 268, 4, 0, -1, -1 /* *** need armor */
  },
  {"Ironblood/BG/BG-CHAOS.utf",
   "Ironblood/Character/LOFO/LOFO-180.csf",
   "LOFO-180_world",
   "Ironblood/Character/LOFO/XFORMS.DAT",
   "Ironblood/Character/LOFO/ANIMDETL.DAT",
   "Ironblood/Character/LOFO/POSITION.DAT",
   "Ironblood/Character/LOFO/KEYS.DAT",
   1, 1,
   "Ironblood/platform/STEL/BASE",
   "Ironblood/platform/STEL/RING",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 306, 5, 0, 12, -1 /* *** need armor */
  },
  {"Ironblood/BG/BG-CHAOS.utf",
   "Ironblood/Character/MOFC/MOFC-180",
   "MOFC-180_world",
   "Ironblood/Character/MOFC/XFORMS.DAT",
   "Ironblood/Character/MOFC/ANIMDETL.DAT",
   "Ironblood/Character/MOFC/POSITION.DAT",
   "Ironblood/Character/MOFC/KEYS.DAT",
   1, 0,
   "Ironblood/platform/STEL/BASE",
   "Ironblood/platform/STEL/RING", 
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   17, 216, 4, 0, 12, -1 /* *** may be too small; need armor */
  },
  {"Ironblood/BG/BG-CHAOS.utf",
   "Ironblood/Character/LOFC/LOFC-180.csf",
   "LOFC-180_world",
   "Ironblood/Character/LOFC/XFORMS.DAT",
   "Ironblood/Character/LOFC/ANIMDETL.DAT",
   "Ironblood/Character/LOFC/POSITION.DAT",
   "Ironblood/Character/LOFC/KEYS.DAT",
   1, 0,
   "Ironblood/platform/STEL/BASE.csf",
   "Ironblood/platform/STEL/RING.csf",  
   {
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
     "XXXXXXXXXXX",
   },
   16, 331, 5, 0, -1, -1 /* *** need armor */
  }
};

static ulong ThreeDOtoSONY_Controler(uint32 padd1, uint32 padd2)
{
  register ulong temp;
  
  temp = 0x00000000;
  
  if((padd1 & ControlUp) == ControlUp)			temp |= (ulong) padLup;
  if((padd1 & ControlDown)	== ControlDown)		temp |= (ulong) padLdown;
  if((padd1 & ControlLeft)	== ControlLeft)		temp |= (ulong) padLleft;
  if((padd1 & ControlRight) ==	ControlRight)	temp |= (ulong) padLright;
  if((padd1 & (ControlUp | ControlLeft)) == (ControlUp | ControlLeft))	temp |= (ulong) padLdul;
  if((padd1 & (ControlLeft | ControlDown))	== (ControlLeft | ControlDown)) temp |= (ulong) padLddl;
  if((padd1 & (ControlRight | ControlDown)) ==	(ControlRight | ControlDown)) temp |= (ulong) padLddr;
  if((padd1 & (ControlUp | ControlRight)) == (ControlUp | ControlRight))	temp |= (ulong) padLdur;
  if((padd1 & ControlF)==	ControlF)			temp |= (ulong) padLtop;
  if((padd1 & ControlC)==	ControlC)			temp |= (ulong) padLbottom;
  if((padd1 & ControlE)==	ControlE)			temp |= (ulong) padRup;
  if((padd1 & ControlA)==	ControlA)			temp |= (ulong) padRdown;
  if((padd1 & ControlD)==	ControlD)			temp |= (ulong) padRleft;
  if((padd1 & ControlB)==	ControlB)			temp |= (ulong) padRright;
  if((padd1 & ControlLeftShift)==	ControlLeftShift)	temp |= (ulong) padRtop;
  if((padd1 & ControlRightShift)==	ControlRightShift)	temp |= (ulong) padRbottom;	
  if((padd1 & ControlE)==	ControlE)			temp |= (ulong) padTriangle;
  if((padd1 & ControlA)==	ControlA)			temp |= (ulong) padX;
  if((padd1 & ControlD)==	ControlD)			temp |= (ulong) padSquare;
  if((padd1 & ControlB)==	ControlB)			temp |= (ulong) padCircle;
  if((padd1 & ControlX)==	ControlX)			temp |= (ulong) padSelect;
  if((padd1 & ControlStart)==	 ControlStart)	temp |= (ulong) padStart;
  
  if((padd2 & ControlUp) == ControlUp)			temp |= (ulong) padLup << 16;
  if((padd2 & ControlDown)	== ControlDown)		temp |= (ulong) padLdown << 16;
  if((padd2 & ControlLeft)	== ControlLeft)		temp |= (ulong) padLleft << 16;
  if((padd2 & ControlRight) ==	ControlRight)	temp |= (ulong) padLright << 16;
  if((padd2 & (ControlUp | ControlLeft)) == (ControlUp | ControlLeft))	temp |= (ulong) (padLdul) << 16;
  if((padd2 & (ControlLeft | ControlDown))	== (ControlLeft | ControlDown)) temp |= (ulong) (padLddl) << 16;
  if((padd2 & (ControlRight | ControlDown)) ==	(ControlRight | ControlDown)) temp |= (ulong) (padLddr) << 16;
  if((padd2 & (ControlUp | ControlRight)) == (ControlUp | ControlRight))	temp |= (ulong) (padLdur) << 16;
  if((padd2 & ControlF)==	ControlF)			temp |= (ulong) padLtop << 16;
  if((padd2 & ControlC)==	ControlC)			temp |= (ulong) padLbottom << 16;
  if((padd2 & ControlE)==	ControlE)			temp |= (ulong) padRup << 16;
  if((padd2 & ControlA)==	ControlA)			temp |= (ulong) padRdown << 16;
  if((padd2 & ControlD)==	ControlD)			temp |= (ulong) padRleft << 16;
  if((padd2 & ControlB)==	ControlB)			temp |= (ulong) padRright << 16;
  if((padd2 & ControlLeftShift)==	ControlLeftShift)	temp |= (ulong) padRtop << 16;
  if((padd2 & ControlRightShift)==	ControlRightShift)	temp |= (ulong) padRbottom << 16;	
  if((padd2 & ControlE)==	ControlE)			temp |= (ulong) padTriangle << 16;
  if((padd2 & ControlA)==	ControlA)			temp |= (ulong) padX << 16;
  if((padd2 & ControlD)==	ControlD)			temp |= (ulong) padSquare << 16;
  if((padd2 & ControlB)==	ControlB)			temp |= (ulong) padCircle << 16;
  if((padd2 & ControlX)==	ControlX)			temp |= (ulong) padSelect << 16;
  if((padd2 & ControlStart)==	 ControlStart)	temp |= (ulong) padStart << 16;
  return temp;
}
/************************************************/


static ushort loadTMD(ushort PNum, Ptcl *player, ushort numtmds,
                      ushort startpoly, ushort offset)
{
  ObjDetail *Obj;
  float      minx, miny, minz, maxx, maxy, maxz;
  ushort     index;
  
  short center_x, center_y, center_z;
  
  Pod *curPod;
  
  if(PNum == char0)
    curPod = gBSDF->pods;
  else
    curPod = gBSDF2->pods;
  
  for (index = 0; index < 4; index++) player->AttackObj[index] = -1;
  
  for (index = 0, Obj = player->Obj; index < numtmds; index++) {
    
    curPod = curPod->pnext;
    if (curPod == NULL)
      printf("Whooops! CurPod is NULL! Player=%d index=%d numtmds=%d\n",
	     PNum, index, numtmds);

    minx = miny = minz = 32767;
    maxx = maxy = maxz = -32767;
    
    /*Satoru	Char_GetBound(Char_Obj[offset][index+1], &Box3_temp, Char_Obj[offset][index+1]); */
    
    minx = curPod->pgeometry->fxmin;
    maxx = curPod->pgeometry->fxextent + curPod->pgeometry->fxmin;
    
    miny = curPod->pgeometry->fymin;
    maxy = curPod->pgeometry->fyextent + curPod->pgeometry->fymin;
    
    minz = curPod->pgeometry->fzmin;
    maxz = curPod->pgeometry->fzextent + curPod->pgeometry->fzmin;
    
    
    /*		printf("max %hd %hd %hd\n", maxx, maxy, maxz); */
    /*		printf("min %hd %hd %hd\n", minx, miny, minz); */
    
    
    /*Satoru	Char_GetCenter(Char_Obj[offset][index+1], &P3_temp, Char_Obj[offset][index+1]); */
    
    center_x = (short) 0;
    center_y = (short) 0;
    center_z = (short) 0;
    
    /*		printf("center %hd %hd %hd\n", center_x, center_y, center_z); */
    
    /* calculate end points of a line parallel to each side of the object's */
    /* bounding box, up the center -- keep the 2 longest axes only */
    /* add 20 to length to get through the target object down to the */
    /* detection axis inside */
    Obj->Base[0].x = (Obj->Base[0].x < (minx - 5.0))?(Obj->Base[0].x):(minx - 5.0);
    Obj->Tip[0].x  = (Obj->Tip[0].x  > (maxx + 5.0))?(Obj->Tip[0].x):(maxx + 5.0);
    Obj->Base[0].y = Obj->Tip[0].y = center_y;
    Obj->Base[0].z = Obj->Tip[0].z = center_z;
    Obj->Base[1].x = Obj->Tip[1].x = center_x;
    Obj->Base[1].y = (Obj->Base[1].y < (miny - 5.0))?(Obj->Base[1].y):(miny - 5.0);
    Obj->Tip[1].y =  (Obj->Tip[1].y  > (maxy + 5.0))?(Obj->Tip[1].y):(maxy + 5.0);                                
    Obj->Base[1].z = Obj->Tip[1].z = center_z;
    Obj->Base[2].x = Obj->Tip[2].x = center_x;
    Obj->Base[2].y = Obj->Tip[2].y = center_y;
    Obj->Base[2].z = (Obj->Base[2].z < (minz - 5.0))?(Obj->Base[2].z):(minz - 5.0);
    Obj->Tip[2].z =  (Obj->Tip[2].z  > (maxz + 5.0))?(Obj->Tip[2].z):(maxz + 5.0);
    
    /* calculate attack's start axis (only check 2 of three axes) */
    if (maxx - minx > maxy - miny) { /* x > y, so keep x */
      if (maxz - minz > maxy - miny) { /* z > y, so keep z & x */
	Obj->StartAxis = 2; /* 2 + 1 = 0 (x) */
      }
      else Obj->StartAxis = 1; /* keep z & y */
    }
    else { /* y > x, so keep y */
      if (maxz - minz > maxx - minx) { /* z > x, so keep y & z */
	Obj->StartAxis = 1; /* 1 + 1 = 2 (z) */
      }
      else Obj->StartAxis = 0; /* keep x & y */
    }
    /*		printf("StartAxis %hd\n", Obj->StartAxis); */
    
    if (curPod->pnext == NULL)
      {
	Obj++;
      }
    else if(curPod->pmatrix != curPod->pnext->pmatrix){
      /*			printf("Obj%2d {\n", index);		 */
      /*			printf("max %f %f %f\n", (short) Obj->Tip[0].x - 5.0,  (short) Obj->Tip[1].y - 5.0,  (short) Obj->Tip[2].z - 5.0); */
      /*			printf("min %f %f %f\n", (short) Obj->Base[0].x + 5.0, (short) Obj->Base[1].y + 5.0, (short) Obj->Base[2].z + 5.0); */
      /*			printf("}\n"); */
      Obj++;
    }
    else
      index--;
    
  }
  curPod = NULL;
  
  return 0;
}

static short FireRand(void)
{
  return (Rand() % (RootRand * 2 + 1) - RootRand);
}

void ADdivby4( void )
{
  int32 i;
  
  /*	for(i=0; i<35280; i++){ */
  /*		AnimData[i].rotx	=	-AnimData[i].rotx * 360.0 / 4096.0 * kRadian; */
  /*		AnimData[i].roty	=	-AnimData[i].roty * 360.0 / 4096.0 * kRadian; */
  /*		AnimData[i].rotz	=	 AnimData[i].rotz * 360.0 / 4096.0 * kRadian; */
  
  /*		vx =   ( -vx * 360.0 / 4096.0 * kRadian);	*/
  /*		vy =   ( -vy * 360.0 / 4096.0 * kRadian);	*/
  /*		vz =   ( vz * 360.0 / 4096.0 * kRadian);	*/
  
  /*		AnimData[i].tranx	=	-AnimData[i].tranx / 4.0; */
  /*		AnimData[i].trany	=	-AnimData[i].trany / 4.0; */
  /*		AnimData[i].tranz	=	AnimData[i].tranz / 4.0; */
  /*	} */
  for(i=0; i<NUMOFPOSFRAMES * 3; i+=3){
    PosOffset[i]   = -PosOffset[i]/4.0;
    PosOffset[i+1] =  PosOffset[i+1]/4.0;
    PosOffset[i+2] =  PosOffset[i+2]/4.0;
  }
}

ulong swap_long(ulong input)
{
  ulong temp;
  
  temp = input>>16;
  temp |= input<<16;
  return temp;
}

static void CalcFire(ushort Life0, ushort Max0, ushort Life1, ushort Max1)
{
  register ushort x;
  register ushort y;
  register short  i;
  register short  j;
  register uchar *thisrow;
  register uchar *nextrow;
  register short  MoreFire = -2;
  register ulong *slptr;
  register ulong *dlptr;
  
  
  static ushort Life0_prev = 200;
  static ushort Life1_prev = 200;
  
  /* this loop controls the root of the flames */
  for (i = kXleft; i < kXright + 1; i++) {
    x = (short)FlameArray[i];
    if (x < MinFire) {
      if (x > 10) x = x + Rand() % FireIncrease;
    }
    else x = x + FireRand() + MoreFire;
    if (x > 255) x = 255;
    FlameArray[i] = (uchar)x;
  }
  
  /* set the bottom line of the flames */
  slptr = (ulong *)FlameArray;
  dlptr = (ulong *)flame_buffer[kMaxY];
  for (i = kMaxX >> 2; i > 0; i--, slptr++, dlptr++) *dlptr = *slptr; /* copy 32 bytes */
  
  /* this loop makes the actual flames */
  thisrow = flame_buffer[0];
  nextrow = &flame_buffer[1][1];
  for (y = 0; y < kMaxY; y++) {
    *thisrow = thisrow[kMaxX] = 0;
    thisrow++;
    for (i = 1; i < kMaxX; i++, thisrow++, nextrow++) {
      x = *nextrow;
      if (x < kDecay) *thisrow = 0;
      else {
	j = (Rand() % 3) - 1;
	thisrow[j] = (uchar)(x - (Rand() % kDecay));
	if (j == 1 && i < kMaxX - 1) i++, thisrow++, nextrow++;
      }
    }
    thisrow++;
    nextrow += 2;
  }
    /* make small puffs of flame randomly */
  /*
    if ((Rand() % 150) == 0) {
    x = kXleft + Rand() % (kFlamewidth - 2 * kDeadzone - 5);
    memset(&FlameArray0[x], 255, 5);
    }
    if ((Rand() % 150) == 0) {
    x = kXleft + Rand() % (kFlamewidth - 2 * kDeadzone - 5);
    memset(&FlameArray1[x], 255, 5);
    }
    */
  
  
  /* size the flame sprites to % of life left */
  /* original height = kMaxY+21 = 70 */
  /* original width  = kMaxX+5  = 32 */
  /*    i = kMaxX - 5; /* difference of 10 */
  /*    j = kMaxY + 20; /* one less, since adding 1 if Life > 0 */
  if (Life0 > 0) {
    /*        x = (Life0 * i / Max0) + 10; /* new width */
    /*        y = (Life0 * j / Max0) + 1; /* new height */
    
    if((Life0 == Life0_prev)||(Life0_prev==0.0)){}
    else{
      bltMat[0][0] = 1.0;
      bltMat[1][1] = (gfloat)Life0/(gfloat)Life0_prev;
      Blt_MoveVertices(Flame0->bo_vertices, -P2_Flame0.x, -P2_Flame0.y);
      Blt_TransformVertices(Flame0->bo_vertices, bltMat);
      Blt_MoveVertices(Flame0->bo_vertices, P2_Flame0.x, P2_Flame0.y);
    }
  }
  if (Life1 > 0)
    {
      
      if((Life1 == Life1_prev)||(Life1_prev==0.0)){}
      else
	{
	  bltMat[0][0] = 1.0;
	  bltMat[1][1] = (gfloat)Life1/(gfloat)Life1_prev;
	  Blt_MoveVertices(Flame1->bo_vertices, -P2_Flame1.x, -P2_Flame1.y);
	  Blt_TransformVertices(Flame1->bo_vertices, bltMat);
	  Blt_MoveVertices(Flame1->bo_vertices, P2_Flame1.x, P2_Flame1.y);
	}
    }
  Life0_prev = Life0;
  Life1_prev = Life1;
  return;
}

#ifdef USE_BLITTER_BG
static void DrawBG(BlitObject *blit, AppData* appData)
{
  float rot;
  Vector3D   lvec = {1, 0, 1};
  float      x;
  uint32     offset;

  
  Vector3D_OrientateByMatrix(&lvec, appData->matrixCamera);
  rot = (float) atan2f(lvec.z, lvec.x) / 3.14159265358979;
  if(rot<0) rot += 1.0;
  
  /*  UV Approach */
  
  x =  640.0 * rot;
  /*
    Bg_Vertices[18] = Bg_Vertices[3] = x;
    Bg_Vertices[8] = Bg_Vertices[13] = x+IB_BG_WIDTH-1; 
    Blt_SetVertices(blit->bo_vertices, Bg_Vertices);  
    */
  offset = (uint32)x + 0.5;
  offset *= (blit->bo_txdata->btd_txData.bitsPerPixel>>3);
  if (offset != 0)
    Blt_SetTexture(blit, (void *)(Bg_Address+offset));
}
#endif

/* --------------------------------------------------------------------- */
/************************************************/
static void *LoadPCFile(char *fname, char *buf)
{
  FileInfo ImageFileInfo;
  RawFile *file;
  Err err;
  unsigned char temp_char;
  uint32 i;
  
  err = OpenRawFile(&file, fname, FILEOPEN_READ);
  if (err < 0) printf(" ******* error opening %s\n", fname);
  
  if(GetRawFileInfo(file, &ImageFileInfo, sizeof(FileInfo))<0)
    printf("couldn't get raw file info\n");
  
  if(ReadRawFile(file, buf, ImageFileInfo.fi_ByteCount) < 1){
    printf("Fail reading file ...\n");
    return 0;
  }			
  
  for(i=0; i<ImageFileInfo.fi_ByteCount; i+=2){
    temp_char	= buf[i];
    buf[i]		= buf[i+1];
    buf[i+1]	= temp_char;
  }
  printf("Loaded %-50s (%6d bytes:)\n", fname, ImageFileInfo.fi_ByteCount);
  CloseRawFile(file);
  
  return (buf + ImageFileInfo.fi_ByteCount);
}

static void Load_BGImage(char *fname, char *buf)
{
  FileInfo ImageFileInfo;
  RawFile *file;
  Err err;
  /*  FileInfo *info; */
  /*  unsigned char temp_char; */
  /*  uint32 i; */
  
  err = OpenRawFile(&file, fname, FILEOPEN_READ);
  if (err < 0) printf(" ******* error opening %s\n", fname);
  
  if(GetRawFileInfo(file, &ImageFileInfo, sizeof(FileInfo))<0)
    printf("couldn't get raw file info\n");
  
  if(ReadRawFile(file, buf, ImageFileInfo.fi_ByteCount) < 1){
    printf("Fail reading file ...\n");
  }			
  printf("Loaded %-50s (%6d bytes:)\n", fname, ImageFileInfo.fi_ByteCount);
  CloseRawFile(file);
}

static void LoadPlatform(ushort Player0)
{
  /*	ushort    i , n_prim = 0, count = 0, runecount = 0, circle; */
  
  /*	RuneB_Texture = Txb_Create(); */
  
  /*	theSDF = SDF_Open (PData[char0].pltex0);	if (theSDF == NULL) printf("Error opening the file RING.csf\n"); */
  /*	theSDFObj = SDF_FindObj (theSDF, "character", "BASE_world"); if (theSDFObj == NULL) printf("Error opening the file RING.csf\n");	 */
  /*	Obj_Assign (&platform1, theSDFObj); */
  /*	printf ("SDF finished loading. RNDPLTFM.csf\n"); */
  /*	if (theSDF) SDF_Close (theSDF); */
  
  if(PData[Player0].lawchar) {
    rune = 0x0000FFFF;
    RuneOffset = -0x00000F0F;
    /* Load the blue color rune. */
    /*		if(Txb_Load(RuneB_Texture, "RuneB.utf")<0) printf("Error can't load the RuneB.utf"); printf("Finished loading RuneB.utf\n"); */
    /*check wether it is circular or square*/
    if (PData[Player0].circular) {
      /*       circle = 1; */
      /*		Platform.MaxDistance = 2175625; /* 1475 squared */
      Platform.MaxDistance = 135977; /* 368.75 squared */
      Platform.SquarePlat = 0;
      
      /*			theSDF = SDF_Open ("Ironblood/platform/RING/BLUE_CR/RING.csf");	if (theSDF == NULL) printf("Error opening the file RING.csf\n"); */
      /*			theSDFObj = SDF_FindObj (theSDF, "character", "RING_3"); if (theSDFObj == NULL) printf("Error opening the file RING.csf\n"); */
      /*			Obj_Assign (&platform2, theSDFObj); */
      /*			printf ("SDF finished loading. SQRPLTFM.csf\n"); */
      /*			if (theSDF) SDF_Close (theSDF); */
    }
    else {
      /*   	circle = 0; */
      Platform.SquarePlat = 1;
      /*		Platform.MaxDistance = 975; /* maximum value of abs(x) or abs(z) */
      Platform.MaxDistance = 244; /* maximum value of abs(x) or abs(z) */
      
      /*			theSDF = SDF_Open ("Ironblood/platform/RING/BLUE_SQ/RING.csf");	if (theSDF == NULL) printf("Error opening the file RING.csf\n"); */
      /*			theSDFObj = SDF_FindObj (theSDF, "character", "RING_3"); if (theSDFObj == NULL) printf("Error opening the file RING.csf\n"); */
      /*			Obj_Assign (&platform2, theSDFObj); */
      /*			printf ("SDF finished loading. SQRPLTFM.csf\n"); */
      /*			if (theSDF) SDF_Close (theSDF); */
    }
    
  }
  else{
    /*		rune = 0x00FF0000; */
    /*		RuneOffset = -0x000F0000; */
    /* Load the red color rune. */
    /*		if(Txb_Load(RuneB_Texture, "RuneR.utf")<0) printf("Error can't load the RuneR.utf"); printf("Finished loading RuneR.utf\n"); */
    
    if (PData[Player0].circular) {
      /*	        circle = 1; */
      /*			Platform.MaxDistance = 2175625; /* 1475 squared */
      Platform.MaxDistance = 135977; /* 368.75 squared */
      Platform.SquarePlat = 0;
      /*			theSDF = SDF_Open ("Ironblood/platform/RING/RED_CR/RING.csf");	if (theSDF == NULL) printf("Error opening the file RING.csf\n"); */
      /*			theSDFObj = SDF_FindObj (theSDF, "character", "RING_3"); if (theSDFObj == NULL) printf("Error opening the file RING.csf\n"); */
      /*			Obj_Assign (&platform2, theSDFObj); */
      /*			printf ("SDF finished loading. SQRPLTFM.csf\n"); */
      /*			if (theSDF) SDF_Close (theSDF); */
    }
    else {
      /*    	    circle = 0; */
      Platform.SquarePlat = 1;
      /*			Platform.MaxDistance = 975; /* maximum value of abs(x) or abs(z) */
      Platform.MaxDistance = 244; /* maximum value of abs(x) or abs(z) */
      /*			theSDF = SDF_Open ("Ironblood/platform/RING/RED_SQ/RING.csf");	if (theSDF == NULL) printf("Error opening the file RING.csf\n"); */
      /*			theSDFObj = SDF_FindObj (theSDF, "character", "RING_3"); if (theSDFObj == NULL) printf("Error opening the file RING.csf\n"); */
      /*			Obj_Assign (&platform2, theSDFObj); */
      /*			printf ("SDF finished loading. SQRPLTFM.csf\n"); */
      /*			if (theSDF) SDF_Close (theSDF); */
    }
    
  }
  /*	txb =  Txb_Create(); */
  /*	tex_array = Array_Create(1, 0); */
  /*	tex_array = Mod_GetTextures(platform2); */
  /*	txb = Array_GetObj(tex_array, 0); */
  
  return;
}


/************************************************/
static void InitPlayers(void)
{
  Ptcl       *p0;
  Ptcl       *p1;
  AnimDetail *destad;
  void       *ad;
  void       *po;
  ushort      i, j;
  uchar temp_swap = 0;
  int8  temp_swap_s = 0;
  
  p0 = Player;
  p1 = &Player[1];
  
  p0->NumOfTMDs = PData[char0].numofobjs;	
  p1->NumOfTMDs = PData[char1].numofobjs;
  
  p0->Obj = Objects;
  p1->Obj = &Objects[18];
  
  loadTMD(char0, p0, p0->NumOfTMDs, 0, 0);
  loadTMD(char1, p1, p1->NumOfTMDs, 0, 1);
  
  p0->orientation = Char_Obj[0][0];
  p1->orientation = Char_Obj[1][0];
  
  copyVector(&p0->position, &zerovec);
  copyVector(&p1->position, &zerovec);	
  
  p0->vbcount = p1->vbcount = p0->lastpadd = p1->lastpadd = p0->HitRune = p1->HitRune = 0;
  
  Matrix_SetTranslation(Char_Obj[0][0], -200, 0, 0);
  Player[0].position.x = -200;
  Player[0].position.y = 0;
  Player[0].position.z = 0;
  p0->FacingRight = 1;
  
  Matrix_SetTranslation(Char_Obj[1][0], 200, 0, 0);
  Player[1].position.x = 200;
  Player[1].position.y = 0;
  Player[1].position.z = 0;
  p1->FacingRight = 0;
  
  PlayerSep = 160000; /* distance between them, squared */
  
  p0->HitPoints = p1->HitPoints = p0->HPleft = p1->HPleft = 200;
  p0->NextMove = p0->CalcedMove = p1->NextMove = p1->CalcedMove = STAND;
  p0->RepeatDamage = PData[char0].damageR;
  p0->LawChar = PData[char0].lawchar;
  p0->Weapon1 = PData[char0].weapon1;
  p0->Weapon2 = PData[char0].weapon2;
  
  p0->Human = 1;
  p1->RepeatDamage = PData[char1].damageR;
  p1->LawChar = PData[char1].lawchar;
  p1->Weapon1 = PData[char1].weapon1;
  p1->Weapon2 = PData[char1].weapon2;
  
  p1->Human = 0;
  for (i = 0; i < 15; i++) {
    p0->AttackProb[i] = AttackTable[Options.Level][i];
    p1->AttackProb[i] = AttackTable[Options.Level][i];
  }
  
  LoadPCFile(PData[char0].keydata, (void *)PMoves[0]);
  LoadPCFile(PData[char1].keydata, (void *)PMoves[1]);
  
  /* now read and parse the AnimDetails, AnimData and PosOffset structures */
  LoadPCFile(PData[char0].animdetails, (void *)AnimDetails);
  ad = LoadPCFile(PData[char0].animdata, (void *)AnimData);
  po = LoadPCFile(PData[char0].posdata, (void *)PosOffset);
  destad = AnimDetails[0];
  
  for (i = 0; i < NUMOFMOVES; i++, destad++) {
    destad->AnimDataPtr = (ObjAnim *)(swap_long((ulong)destad->AnimDataPtr)+ (ulong)AnimData);
    destad->PosOffsetPtr = (short *)(swap_long((ulong)destad->PosOffsetPtr)+ (ulong)PosOffset);
  }
  LoadPCFile(PData[char1].animdetails, (void *)&AnimDetails[1]);
  
  LoadPCFile(PData[char1].animdata, ad);
  LoadPCFile(PData[char1].posdata, po);
  destad = AnimDetails[1];
  for (i = 0; i < NUMOFMOVES; i++, destad++) {
    destad->AnimDataPtr = (ObjAnim *)(swap_long((ulong)destad->AnimDataPtr) + (ulong)ad);
    destad->PosOffsetPtr = (short *)(swap_long((ulong)destad->PosOffsetPtr) + (ulong)po);
  }
  for(i=0; i<2; i++){
    for(j=0; j<NUMOFMOVES; j++){
      temp_swap = AnimDetails[i][j].NumFrames;
#if 0
      fprintf(stderr,"Char %d: Move:%d \tNumFrames %d\n",i,j,temp_swap);
#endif
      AnimDetails[i][j].NumFrames = AnimDetails[i][j].Sound;
      AnimDetails[i][j].Sound = temp_swap;
      
      temp_swap_s = AnimDetails[i][j].AttackObjNum[0];
      AnimDetails[i][j].AttackObjNum[0] = AnimDetails[i][j].AttackObjNum[1];
      AnimDetails[i][j].AttackObjNum[1] = temp_swap_s;
      
      temp_swap_s = AnimDetails[i][j].AttackObjNum[2];
      AnimDetails[i][j].AttackObjNum[2] = AnimDetails[i][j].AttackObjNum[3];
      AnimDetails[i][j].AttackObjNum[3] = temp_swap_s;
      
      temp_swap = AnimDetails[i][j].Damage;
      AnimDetails[i][j].Damage = AnimDetails[i][j].Knockdown;
      AnimDetails[i][j].Knockdown = temp_swap;
      
      temp_swap = AnimDetails[i][j].KicksUpDust;
      AnimDetails[i][j].KicksUpDust = AnimDetails[i][j].Bleeds;
      AnimDetails[i][j].Bleeds = temp_swap;
    }
  }
  
  ADdivby4();
  
#if 0
  for(i=0; i<1; i++){
    for(j=0; j<NUMOFMOVES; j++){
      printf("AttackObjNum:   %3hd\n", AnimDetails[i][j].AttackObjNum[0]);	
      printf("AttackObjNum:   %3hd\n", AnimDetails[i][j].AttackObjNum[1]);
      printf("AttackObjNum:   %3hd\n", AnimDetails[i][j].AttackObjNum[2]);	
      printf("AttackObjNum:   %3hd\n", AnimDetails[i][j].AttackObjNum[3]);
    }
  }
  
  
  for(i=0; i<1; i++)
    for(j=0; j<NUMOFMOVES; j++){
      printf("\n");
      printf("AnimDetails[%2hu][%hu]: \n", i, j);	
      printf("ushort      NumFrames:  %hu\n", AnimDetails[i][j].NumFrames);
      /*printf("short       LoopCount:  %hd\n", AnimDetails[i][j].LoopCount); */
      /*printf("uchar		Sound1:     %3hhu\n", AnimDetails[i][j].Sound1); */
      /*printf("uchar		SFrame1:    %3d\n", AnimDetails[i][j].SFrame1); */
      /*printf("uchar		Sound2:     %3d\n", AnimDetails[i][j].Sound2); */
      /*printf("uchar		SFrame2:    %3d\n", AnimDetails[i][j].SFrame2); */
      /*printf("ushort		LoopStart:  %hu\n", AnimDetails[i][j].LoopStart); */
      /*printf("ushort		LoopEnd:    %hu\n", AnimDetails[i][j].LoopEnd); */
      /*printf("uchar		Damage:     %3d\n", AnimDetails[i][j].Damage); */
      /*printf("uchar		Knockdown:  %3d\n", AnimDetails[i][j].Knockdown); */
      printf("uchar		KicksUpDust:%3d\n", AnimDetails[i][j].KicksUpDust);
      /*printf("uchar		filler1:        %3d\n", AnimDetails[i][j].filler1); */
      /*printf("short		AttackObjNum:   %3hd\n", AnimDetails[i][j].AttackObjNum); */
      printf("short       *PosOffsetPtr:  %p\n", AnimDetails[i][j].PosOffsetPtr);
      printf("            *AnimDataPtr:   %p\n", AnimDetails[i][j].AnimDataPtr);	
    }
  printf("PosOffset Begining of 1:  %p\n", &PosOffset[0]);
  printf("PosOffset Begining of 2:  %p\n", &PosOffset[NUMOFPOSFRAMES * 3/2]);
#endif
  return;
}

static void cbvsync(AppData* app)
{
  register ulong pad1, pad2;
  
  GetControlPad (1, FALSE, &app->cped);
  pad1 = app->cped.cped_ButtonBits;
  
  app->cped.cped_ButtonBits = 0;
  
  GetControlPad (2, FALSE, &app->cped);
  pad2 = app->cped.cped_ButtonBits;
  
  paddbuffer[paddbuffidx] = ThreeDOtoSONY_Controler(pad1, pad2);
  paddbuffidx++;
  paddbuffidx &= 7;
  return;
}

static void PositionObjects(AppData* appData )
{
  Ptcl  *p;
  register ushort index;
  register short  temp;
  register ushort i;
  Poof *poof;
  POLY_FT4 *sp;
  ObjDetail *Obj;
  
  /*	Poof  *poof; */
  /*	POLY_FT4 *sp; */
  
  register short *t;
  register short sx, sy, sz;
  
  Matrix mat_temp;
  
  Vector3D Pt4_temp = {0, 0, 0};
  Vector3D Vx = {1, 0, 0};
  Vector3D Vz = {0, 0, 1};
  
  
  UpdateCharPos(0, Player, &Player[1]);
  UpdateCharPos(1, &Player[1], Player);
  
  /* process character movement first, then draw individual objects */ 
  
  for(i=0; i<2; i++){
    p = &Player[i];
    p->OpponentIsInFront = 1 - IsBehind(p, &Player[1-i].position);
    
    if((temp = p->SpinAmount) != 0 && CombatTimer>0){
      Matrix_TurnYLocal(Char_Obj[i][0], (float) temp * 0.00048828125); 
    }
    
    Vx.x = 1;
    Vx.y = 0;
    Vx.z = 0;
    
    Vz.x = 0;
    Vz.y = 0;
    Vz.z = 1;
    
    Vector3D_OrientateByMatrix( &Vz, p->orientation);
    Vector3D_OrientateByMatrix( &Vx, appData->matrixCamera);
    if(Vector3D_Dot(&Vx, &Vz) < 0) p->FacingRight = 1;
    else p->FacingRight = 0;
    
    /*		Trans_Copy(Char_GetTransform(Shadow_Obj[i][0]), Char_GetTransform(Char_Obj[i][0])); */
    
    for (index=0, Obj=p->Obj; index < p->NumOfTMDs; index++, Obj++) {
      
      /* get frame rotation & position info */ 
      t = (short *)p->animdata;
      
      sx = *t;	t++;	
      sy = *t;	t++;	
      sz = *t;	t++;
#if 0		
      vx = (gfloat) sx;
      vy = (gfloat) sy;
      vz = (gfloat) sz;
      vx =   ( -vx * 360.0 / 4096.0 * kRadian);
      vy =   ( -vy * 360.0 / 4096.0 * kRadian);
      vz =   ( vz * 360.0 / 4096.0 * kRadian);
#endif		
      Matrix_Identity(Char_Obj[i][index+1]);
      
      Matrix_TurnX(Char_Obj[i][index+1], (float)-sx * 0.001533980787886); 	
      Matrix_TurnY(Char_Obj[i][index+1], (float)-sy * 0.001533980787886);	
      Matrix_TurnZ(Char_Obj[i][index+1], (float) sz * 0.001533980787886); 	
      
      sx = *t;	t++;
      sy = *t;	t++;
      sz = *t;	t++;
      
      
      p->animdata = (ObjAnim *)t;
      
      Matrix_Translate(Char_Obj[i][index+1], 
		       -((float) sx / 4.0),
		       -((float) sy / 4.0),
		       ((float) sz / 4.0));
      
      /* Add the world position & orientaion of the character */
      Matrix_Mult(&mat_temp, Char_Obj[i][index+1], Char_Obj[i][0]);
      Matrix_Copy(Char_Obj[i][index+1], &mat_temp);
      
      /*			Trans_Copy(Char_GetTransform(Shadow_Obj[i][index+1]), Char_GetTransform(Char_Obj[i][index+1])); */
      
      /******************/	
      
      for (temp = 0; temp < 3; temp++) {
	/* remember old position */
	copyVector(&Obj->LastBase[temp], &Obj->CurrBase[temp]);
	copyVector(&Obj->LastTip[temp], &Obj->CurrTip[temp]);
	
	Pt4_temp.x = Obj->Base[temp].x;
	Pt4_temp.y = Obj->Base[temp].y;
	Pt4_temp.z = Obj->Base[temp].z;
	
	Vector3D_MultiplyByMatrix(&Pt4_temp, &mat_temp);
	
	Obj->CurrBase[temp].x = Pt4_temp.x ;
	Obj->CurrBase[temp].y = Pt4_temp.y ;
	Obj->CurrBase[temp].z = Pt4_temp.z ;
	
	Pt4_temp.x = Obj->Tip[temp].x;
	Pt4_temp.y = Obj->Tip[temp].y;
	Pt4_temp.z = Obj->Tip[temp].z;
	
	Vector3D_MultiplyByMatrix(&Pt4_temp, &mat_temp);
	
	Obj->CurrTip[temp].x = Pt4_temp.x ;
	Obj->CurrTip[temp].y = Pt4_temp.y ;
	Obj->CurrTip[temp].z = Pt4_temp.z ;		
      }/*End of "for (temp = 0; temp < 3; temp++)" */

#if 1
      if (p->KicksUpDust && p->FrameNum > 2 && Obj->CurrTip[temp].y < DustY+3)
	{
	  if (abs(Obj->CurrTip[temp].y - Obj->LastTip[temp].y) > 4)
	    {
	      /* make dust poof */
	      poof = &dustPoofs[nextDustPoof];
	      copyVector(&poof->pos, &Obj->CurrTip[temp]);
	      if (nextDustPoof > 1 && DistanceSquared(&poof->pos, &poof[-1].pos) < 289) 
		continue; /* 70 squared */
	      
	      poof->pooftype = 2; /* dust */
	      poof->framenum = 20; /* was 255 */
	      sp = poof->sprite;
	      poof->vec1.x = poof->vec1.y = poof->vec2.y = poof->vec3.x = -6 - (Rand() % 6);
	      poof->vec1.z = poof->vec2.z = poof->vec3.z = poof->vec4.z = 0;
	      poof->vec2.x = poof->vec3.y = poof->vec4.x = poof->vec4.y = 12 + (Rand() % 6);
	      poof->alpha = 0x0F + (Rand()%0x10);
	      sp->color = (poof->alpha <<24) + 0x00FFFFAA;

	      /* 
		 SetShadeTex(sp, 0);  SetShadeTex(&sp[1], 0); // shading is ON
		 sp->tpage = sp[1].tpage = GetTPage(0, 3, 704, 0);
		 sp->clut = sp[1].clut = GetClut(256, 489);
		 setRGB0(sp, 128, 128, 128);  setRGB0(&sp[1], 128, 128, 128);
		 */
	      nextDustPoof++;
	      if (nextDustPoof >= MAX_DUST) nextDustPoof = 0;
	    }
	}
      /* Dust */
#endif
    
    }/*End of "for (index=0, Obj=p->Obj; index < p->NumOfTMDs; index++, Obj++)" */
  }/*End of "for(i=0; i<2; i++)" */
  return;
}


/* =========================================================--- */

void Program_Init(GraphicsEnv* genv, AppData** app)
{
  int32 i, j;
  Pod *curPod;
  Matrix *oldMat;

  Vector3D camLocation = {-4000, 4000, -4000};
  ViewPyramid  vp;
  ViewPyramid  shadowVP;
  AppData*     appData;
  uint32       maxPodVerts;
  uint32       maxPodVerts2;
  uint32       maxPodVerts3;
  uint32       maxPodVerts4;
  uint32       maxPodVerts5;
  
  /************************************/	
  /* Create an instance of AppData	*/
  /************************************/
  *app = AllocMem(sizeof(AppData), MEMTYPE_NORMAL);
  if (*app == NULL) {
    printf("Couldn't create AppData structure.  Exiting\n");	exit(1);
  }
  appData = *app;
  
  /************************************/
  /* Load Char 0						*/
  /************************************/
  Model_LoadFromDisk(PData[char0].pmodel, &modelPodCount, &maxPodVerts);
  
  curPod = gBSDF->pods;
  Char_Obj[0][0] = Matrix_Construct();
  Matrix_Identity(Char_Obj[0][0]);
  
  j = 1;
  Char_Obj[0][j] = curPod->pmatrix;
  
  for(i=1; i<gBSDF->numPods; i++){	
    curPod = curPod->pnext;
    if (curPod == NULL)
      {
	printf("Whoops! CurPod is NULL at Char=%d NumPods=%d i=%d j=%d\n",0,
	       gBSDF->numPods,i,j);
      }
    else if(Char_Obj[0][j] != curPod->pmatrix){
      j++;
      Char_Obj[0][j] = curPod->pmatrix;
    }
  }
#ifdef POD_SHADOW
  printf("Shadow Pod Setup\n");

  curPod = shadow0BSDF->pods;
  j=1;
  oldMat = curPod->pmatrix;
  curPod->pmatrix = Char_Obj[0][j];
#ifdef BLINN_SHADOW
  shadowTrans[0][j].Original = curPod->pmatrix;  /* Keep original transform */
  curPod->pmatrix = &(shadowTrans[0][j].Squash);
  curPod->pmaterial->base.r = curPod->pmaterial->base.g = curPod->pmaterial->base.b = 0.0;
#endif


  for(i=1; i<shadow0BSDF->numPods; i++) {
    curPod = curPod->pnext;
    if (curPod == NULL) {
      printf("Whoops! CurPod is NULL at Char=%d NumPods=%d i=%d j=%d\n", 0, shadow0BSDF->numPods,i,j);
    } else {
      if( oldMat != curPod->pmatrix ) {
	j++;
	oldMat = curPod->pmatrix;
      }
      curPod->pmatrix = Char_Obj[0][j];
#ifdef BLINN_SHADOW
      shadowTrans[0][j].Original = curPod->pmatrix;  /* Keep original transform */
      curPod->pmatrix = &(shadowTrans[0][j].Squash);
      curPod->pmaterial->base.r = curPod->pmaterial->base.g = curPod->pmaterial->base.b = 0.0;
#endif
    }
  }

#ifdef BLINN_SHADOW
  shadowTrans[0][0].numNodes = shadow0BSDF->numPods;
#endif

#endif /* POD_SHADOW */

  /************************************/
  /* Load Char 1						*/
  /************************************/
  Model_LoadFromDisk2(PData[char1].pmodel, &modelPodCount2, &maxPodVerts2);
  
  curPod = gBSDF2->pods;
  Char_Obj[1][0] = Matrix_Construct();
  Matrix_Identity(Char_Obj[1][0]);
  
  j = 1;
  Char_Obj[1][j] = curPod->pmatrix;
  for(i=1; i<gBSDF2->numPods; i++){
    curPod = curPod->pnext;
    if(curPod==NULL){
      printf("Whoops! CurPod is NULL Char %d, NumPods=%d i=%d j=%d\n",1,
	     gBSDF2->numPods, i, j);
    }
    else if(Char_Obj[1][j] != curPod->pmatrix){
      j++;
      Char_Obj[1][j] = curPod->pmatrix;
    }
  }

#ifdef POD_SHADOW

  printf("Shadow Pod Setup\n");

  curPod = shadow1BSDF->pods;
  j=1;
  oldMat = curPod->pmatrix;
  curPod->pmatrix = Char_Obj[1][j];
#ifdef BLINN_SHADOW
  shadowTrans[1][j].Original = curPod->pmatrix;  /* Keep original transform */
  curPod->pmatrix = &(shadowTrans[1][j].Squash);
  curPod->pmaterial->base.r = curPod->pmaterial->base.g = curPod->pmaterial->base.b = 0.0;
#endif
  for(i=1; i<shadow1BSDF->numPods; i++) {

    curPod = curPod->pnext;
    if (curPod == NULL) {
      printf("Whoops! CurPod is NULL at Char=%d NumPods=%d i=%d j=%d\n",0,
	     shadow1BSDF->numPods,i,j);
    } else {
      if(oldMat != curPod->pmatrix) {
	j++;
	oldMat = curPod->pmatrix;
      }
    }
    curPod->pmatrix = Char_Obj[1][j];
#ifdef BLINN_SHADOW
  shadowTrans[1][j].Original = curPod->pmatrix;  /* Keep original transform */
  curPod->pmatrix = &(shadowTrans[1][j].Squash);
  curPod->pmaterial->base.r = curPod->pmaterial->base.g = curPod->pmaterial->base.b = 0.0;
#endif
  }
#ifdef BLINN_SHADOW
  shadowTrans[1][0].numNodes = shadow1BSDF->numPods;
#endif

#endif  /* POD_SHADOW */
  
  Model_LoadFromDisk5("Ironblood/Platform/RING/RED_SQ/RUNE", &modelPodCount5, &maxPodVerts5);

  Model_LoadFromDisk3(PData[char1].pltex1, &modelPodCount3, &maxPodVerts3);

  Matrix_Translate(gBSDF3->pods->pmatrix,
		   0.0, 0.5, 0.0);  /* Bring up the ring so that Z-buffer doesn't freak */

  Model_LoadFromDisk4(PData[char1].pltex0, &modelPodCount4, &maxPodVerts4);
  
  
  maxPodVerts4 = (maxPodVerts4 + 3) & ~3;
  

#ifdef USE_MP
  /* Add MP support */
  appData->close = M_Init(2000, 0, genv->gs);
  M_InitMP(appData->close, 400,64*1024 , 650);
#else
  /*  appData->close = M_Init(maxPodVerts4, kMaxCmdListWords, genv->gs); */
  appData->close = M_Init(2000, 0, genv->gs);
#endif

 
  if (appData->close == NULL)
    {
      printf("Couldn't init Mercury.  Exiting\n");	
      exit(1);
    }	
  
  appData->close->fwclose = 1.01;
  appData->close->fwfar = 100000.0;
  appData->close->fscreenwidth = genv->d->width;
  appData->close->fscreenheight = genv->d->height;
  appData->close->depth = genv->d->depth;
  
  /************************************/
  /* Setup camera						*/
  /************************************/
  appData->matrixSkew = AllocMem(sizeof(Matrix), MEMTYPE_NORMAL);
  if (appData->matrixSkew == NULL) {
    printf("Couldn't allocate skew matrix.  Exiting\n");	exit(1);
  }
  vp.left = -1.0;
  vp.right = 1.0;
  vp.top = 0.75;      /* Basic 4/3 aspect ratio, with positive Y UP */
  vp.bottom = -0.75;
  vp.hither = 1.2;
  Matrix_Perspective(appData->matrixSkew, &vp, 0.0, (float)(genv->d->width),
		     0.0, (float)(genv->d->height), 1.0/10.0);
  
  
  /************************************/
  /* Additional camera setup			*/
  /************************************/
  appData->matrixCamera = Matrix_Construct();
  if (appData->matrixCamera == NULL) {
    printf("Couldn't allocate camera matrix.  Exiting\n");	exit(1);
  }
  
  Matrix_SetTranslationByVector(appData->matrixCamera, &camLocation );
  M_SetCamera(appData->close, appData->matrixCamera, appData->matrixSkew);
  
#ifdef POD_SHADOW
#ifndef BLINN_SHADOW
  SetupShadows(shadowEnv0, shadowData0);
#ifndef SINGLE_SHADOW
  SetupShadows(shadowEnv1, shadowData1);
#endif
#endif
#endif

  /************************************/
  /* Setup the controller				*/
  /************************************/
  printf("Setup controller...\n");
  
  InitEventUtility( 1, 0, LC_ISFOCUSED ); 
}

/* --------------------------------------------------------------------- */

void print_pod(Pod *pod)
{
  printf("flags      = %d\n",	pod->flags);
  printf("CloaseData = ????\n");
  printf("ptexture   = %p\n",	pod->ptexture);
  printf("pmatrix    = %p\n", pod->pmatrix);
  Matrix_Print(pod->pmatrix);	
  printf("plights    = %d\n",	pod->plights);
  printf("puserdata  = %d\n",	pod->puserdata);
  printf("pmaterial  = %p\n",	pod->pmaterial);
}

static void Square0(Vector3 *first, Vector3 *second)
{
  second->x = first->x * first->x;
  second->y = first->y * first->y;
  second->z = first->z * first->z;
}

static float DistanceSquared(Vector3 *first, Vector3 *second)
{
  Vector3 vec1;
  
  vec1.x = first->x - second->x;
  vec1.y = first->y - second->y;
  vec1.z = first->z - second->z;
  
  /*	return Vector3D_Length(&vec1) * Vector3D_Length(&vec1); */
  return	(vec1.x * vec1.x) + 
    (vec1.y * vec1.y) + 
    (vec1.z * vec1.z);
}

static ushort IsBehind(Ptcl *p1, Vector3 *v2)
{
  register short retval;
  Vector3 result = {0, 0, 10};
  
  Vector3D_OrientateByMatrix(&result, p1->orientation);
  Vector3D_Add(&result, &result, &p1->position);
  
  if (DistanceSquared(&result, v2) < DistanceSquared(&p1->position, v2)) {
    retval = 1; /* result of == is in front */
  }
  else retval = 0;
  return(retval);
}

static void HitARune(Ptcl *p, ushort DrawExplosion, ushort playernum)
{
  Poof *poof;
  POLY_FT4 *sp;
  SVECTOR tempv;
  
  p->HitRune = 1;
  p->MoveNum = STAND; /* cancel current move */
  p->FrameNum = 0;
  if (IsBehind(p, &zerovec)) { /* he's facing away from center */
    Matrix_LookAt(Char_Obj[playernum][0], &p->position, &zerovec, 0);
    
    p->NextMove = 48; /* fall back from medium hit */
    switch(p->MoveNum) { /* if crouched, do low fall */
    case STAND+1 : /* crouch */
    case STAND+5 : /* crouch block */
    case      12 : /* low attack 1 */
    case      13 : /* low attack 2 */
    case      14 : /* low attack 3 */
    case      15 : /* low attack 4 */
    case REACT+6 : /* reactions to low hits */
    case REACT+7 : p->NextMove = 51; /* fall back while crouching */
      break;
    default      : p->NextMove = 48; /* fall back from medium hit */
      break;
    }
  }
  else { /* he's facing the center of the platform */
    Matrix_LookAt(Char_Obj[playernum][0], &zerovec, &p->position, 0);
    
    switch(p->MoveNum) { /* if crouched, do low fall */
    case STAND+1 : /* crouch */
    case STAND+5 : /* crouch block */
    case      12 : /* low attack 1 */
    case      13 : /* low attack 2 */
    case      14 : /* low attack 3 */
    case      15 : /* low attack 4 */
    case REACT+6 : /* reactions to low hits */
    case REACT+7 : p->NextMove = 50; /* fall forward while crouching */
      break;
    default      : p->NextMove = 45; /* fall forward from medium hit */
      break;
    }
  }
  
  if (DrawExplosion) { /* make big explosion on rune ring */
    p->HPleft -= 20;
    
    poof = &poofs[nextPoof];
    poof->pooftype = 10; /* doesn't face camera */
    poof->framenum = 255;
    copyVector(&poof->pos, &p->position);
    
    /*		Trans_Init(poof->mat, NULL); */
    Matrix_Identity(&poof->mat);
    
    if (Platform.SquarePlat == 0) Matrix_MultiplyOrientation(&poof->mat, p->orientation); /*poof->mat = p->orientation; /* circular platform */
    else { /* square platform -- calc matrix accordingly */
      if (abs(p->position.x - p->position.z) < 10) Matrix_MultiplyOrientation(&poof->mat, p->orientation); /*poof->mat = p->orientation; /* in corner */
      else {
	tempv.x = abs(p->position.x);
	tempv.z = abs(p->position.z);
	if (tempv.z > tempv.x) {
	  tempv.y = 180;
	}
	else {
	  tempv.y = 90;
	}
	tempv.x = tempv.z = 0;
	/*				Trans_Rotate(poof->mat, TRANS_YAxis, tempv.y);	/* RotMatrix(&tempv, &poof->mat); */
	/*				Matrix_RotateYLocal(&poof->mat, tempv.y * kRadian); */
	Matrix_RotateY(&poof->mat, tempv.y * kRadian);
      }
    }
    
    
    poof->pos.y = 55;
    /*		Trans_Translate(poof->mat, &poof->pos); */
    
    poof->vec1.x = 55;
    poof->vec1.y = 55;
    
    poof->vec2.x = -55;
    poof->vec2.y = 55;
    
    poof->vec3.x = 55;
    poof->vec3.y = -55;
    
    poof->vec4.x = -55;
    poof->vec4.y = -55;
    
    poof->vec1.z = poof->vec2.z = poof->vec3.z = poof->vec4.z = 0;

    sp = poof->sprite;
    
    /*       SetShadeTex(sp, 0);         SetShadeTex(&sp[1], 0); /* shading is ON */
    /*       setRGB0(sp, 224, 224, 224); setRGB0(&sp[1], 224, 224, 224); */
    /*       sp->tpage = sp[1].tpage = GetTPage(0, 3, 896, 256); */
    /*       sp->clut = sp[1].clut = GetClut(256, 488); */
    /*       setUVWH(sp, 0, 0, 128, 64); setUVWH(&sp[1], 0, 0, 128, 64); */
    
    sp[0].tc_data[0].u = 0;
    sp[0].tc_data[0].v = 0;
    
    sp[0].tc_data[1].u = 0.5;
    sp[0].tc_data[1].v = 0;
    
    sp[0].tc_data[2].u =  0;
    sp[0].tc_data[2].v =  0.5;
    
    sp[0].tc_data[3].u = 0.5;
    sp[0].tc_data[3].v = 0.5;
    
    sp[0].color = 0x7F000000;
    
    nextPoof++;
    if (nextPoof >= MAX_POOFS) nextPoof = 0;
    
  }
}

static void FaceOpponent(Ptcl *p1, Ptcl *p2, ushort playernum)
{
  
  SVECTOR pos1, pos2;
  Vector3 Vz = {-1, 0, 0};
  
  copyVector(&pos1, &p1->position);
  copyVector(&pos2, &p2->position);
  pos1.y = pos2.y = 0;
  
  Vector3D_Subtract(&pos2, &pos2, &pos1);
  Vector3D_Normalize(&pos2);
  
  Vector3D_OrientateByMatrix(&Vz, p1->orientation);
  
  Matrix_RotateYLocal(Char_Obj[playernum][0], (float) (Vector3D_Dot(&Vz, &pos2)*0.5)); 
  
  return;
}

static void UpdateCharPos(ushort playernum, Ptcl *p, Ptcl *otherp)
{
  register ulong newdist;
  register short *diff;
  Vector3D tempv, newpos, holdpos;
  Vector3D templv, temppos;
  
  if (p->SpinAmount == 0) {
    if (p->Human) switch(p->MoveNum) {
    case 2 : /* evades do */
    case 3 : FaceOpponent(p, otherp, playernum); break;
      
      break;
    case STAND+2 : /* walks don't */
    case STAND+3 : break;
    default : if (p->MoveNum >= STAND && p->MoveNum < REACT - 2) 
      FaceOpponent(p, otherp, playernum);
    break;
    }
    else if (p->MoveNum < REACT - 2) FaceOpponent(p, otherp, playernum);
  }
  /* move characters */
  copyVector(&holdpos, &p->position);
  /* update animation offset */
  
  diff = p->posoffset;
  
  tempv.x = *diff;	diff++;	
  tempv.y = *diff;	diff++;
  tempv.z = *diff;	diff++;
  
  p->posoffset = diff;
  
  if (tempv.x != 0 || tempv.y != 0 || tempv.z != 0) {
    
    Vector3D_OrientateByMatrix(&tempv, p->orientation);
    copyVector(&templv, &tempv);
    
    /* check that he's not too close to the other guy */
    newpos.x = p->position.x + templv.x;
    newpos.y = p->position.y + templv.y;
    newpos.z = p->position.z + templv.z;
    /* if bumping into other guy, push him out of the way */
    if ((newdist = DistanceSquared(&newpos, &otherp->position)) < (ulong)2025) { /* 180/4 squared */
      if (newdist < PlayerSep) {
	/* cut x & z distance to half, and push other player */
	/* unless other guy is lying on the ground */
	
	if (p->HitRune) {
	  if (!otherp->HitRune &&
	      (!(otherp->MoveNum == REACT-2 || otherp->MoveNum == REACT-1 ||
		 (otherp->MoveNum > 43 && otherp->MoveNum < 52)))) {
	    /* hit rune ring, now falling into other guy, and he's not */
	    /* falling or lying stunned on the ground */
	    HitARune(otherp, 0, (1 - playernum));
	    
	  }
	}				
	else if (p->MoveNum < REACT-2 && (otherp->MoveNum == REACT-2 || otherp->MoveNum == REACT-1)) {
	  templv.x = templv.z = newpos.y = 0; /* don't move */
	}
	otherp->position.x += templv.x; /* other guy gets full effect */
	otherp->position.z += templv.z;
	/*				Char_Translate(Char_Obj[(playernum+1)%2][0], templv.x, 0, templv.z); */
	Matrix_Translate(Char_Obj[(playernum+1)%2][0], templv.x, 0, templv.z);
	
	
	if (p->MoveNum >= REACT) {
	  templv.x /= 2; /* this guy only gets half effect */
	  templv.z /= 2;
	}
	p->position.x += templv.x;
	p->position.z += templv.z;
	
	/*				Char_Translate(Char_Obj[playernum][0], templv.x, (newpos.y - p->position.y), templv.z); */
	Matrix_Translate(Char_Obj[playernum][0], templv.x, (newpos.y - p->position.y), templv.z);
	
	
	
	p->position.y = newpos.y; /* the originally-calculated y	 */
      }
      else {
	Matrix_Translate(Char_Obj[playernum][0],	(newpos.x - p->position.x),
			 (newpos.y - p->position.y),
			 (newpos.z - p->position.z));
	
	copyVector(&p->position, &newpos);
      }
    }
    else {
      Matrix_Translate(Char_Obj[playernum][0],	(newpos.x - p->position.x),
		       (newpos.y - p->position.y),
		       (newpos.z - p->position.z));
      copyVector(&p->position, &newpos);
    }
    PlayerSep = DistanceSquared(&newpos, &otherp->position);
  }
  
  /* now check if he's outside the platform */
  if (!p->HitRune) {
    if (Platform.SquarePlat) {
      newdist = abs(p->position.x);
      if (newdist < Platform.MaxDistance - 94) newdist = abs(p->position.z);
      p->CloseToRune = (newdist > (Platform.MaxDistance - 94) ? newdist : 0);
      newdist = abs(p->position.x);
      if (newdist < Platform.MaxDistance) newdist = abs(p->position.z);
    }
    else {
      copyVector(&temppos, &p->position);
      Square0(&temppos, &templv); /* DistanceSquared(&p->position, &zerovec); /* circular platform */
      newdist = templv.x + templv.z;
      /* platform size = 1475, - 375 = 1100, squared = 1210000 */
      if (newdist > 75625) p->CloseToRune = newdist;
      else p->CloseToRune = 0;
    }
    if (newdist > Platform.MaxDistance) {
      p->position.y = 0; /* in case he's in the middle of a jump */
      HitARune(p, 1, playernum);
      copyVector(&p->position, &holdpos);
    }
  }
  
  /*FntPrint("player %d hit rune %d\n", playernum, p->HitRune); */
  
}

static ushort ProcessOneKey(ushort padd, PlayerMove *PMove, Ptcl *p)
{
  register ushort index;
  register short movenum;
  register ushort longestmatched;
  register ushort temp;
  register ushort tempkey;
  PlayerMove *Move;
  
  /*Satoru*/
  /*if ((padd & padSelect) && (Vcount > LastDump + 60)) LastDump = 0; /* set flag so dump occurs **** */
  
  movenum = STAND; /* default if not holding any keys, or no valid move found */
  p->SpinAmount = 0;
  /* check/process held-key moves */
  if (padd & (padLtop | padLbottom | padRtop | padRbottom | padLddr | padLleft)) {
    p->vbcount = 0; /* key is hit so don't bother delaying any more */
    if (padd & padRtop) { /* make character spin or evade */
      if (p->MoveNum < 2 || p->MoveNum == STAND+2 || p->MoveNum == STAND+3) p->SpinAmount = 100;
    }
    else if (padd & padRbottom) {
      if (p->MoveNum < 2 || p->MoveNum == STAND+2 || p->MoveNum == STAND+3) p->SpinAmount = -100;
    }
    if (padd & padLtop) { /* high block */
      /* clear the key from the pad so they don't register in normal */
      /* key processing below */
      padd &= ~(padLtop);
      if (padd & padLdown) movenum = STAND+5; /* crouch block */
      else movenum = STAND+4; /* standing block */
    }
    else if (padd & padLbottom) { /* blocking */
      padd &= ~(padLbottom);
      if (padd & padLup) movenum = STAND+4; /* standing block */
      else movenum = STAND+5; /* crouch block */
    }
    else if (padd & padLdown) { /* crouch */
      movenum = STAND + 1;
      /* don't clear key cuz might be part of longer sequence */
    }
    /* check walking/running */
    else if (padd & (padLright | padLleft)) { /* wants to walk or run */
      /* first determine direction */
      if (p->FacingRight) { /* keys normal */
	if (padd & padLright) padd |= padForward;
	else if (padd & padLleft) padd |= padBackward;
      }
      else { /* keys reversed */
	if (padd & padLright) padd |= padBackward;
	else if (padd & padLleft) padd |= padForward;
      }
      switch(p->MoveNum) {
      case 0 : /* runs */
      case 1 :
      case STAND+2 : /* walks */
      case STAND+3 : movenum = p->MoveNum;
	break;
      default : /* assume he wants to walk */
	if (padd & padForward) movenum = STAND+2;
	else if (padd & padBackward) movenum = STAND+3;
      }
    }
  }
  /* mask off keys that were held down last time */
  tempkey = padd;
  padd &= p->lastpadd;
  p->lastpadd = ~tempkey;
  
  if (padd == 0) {
    p->vbcount++;
    if (p->vbcount == KEYDELAY) { /* time's up -- choose a move (haven't got one yet) */
      /* search for completed moves, and choose longest one */
      longestmatched = 0;
      for (index = 0, Move = PMove; index < MOVEKEYS; index++, Move++) {
	temp = Move->movematch;
	Move->movematch = 0; /* reset all key counters -- kill all moves in progress */
	if (temp == Move->movesize) { /* move complete */
	  if (temp > longestmatched) {
	    longestmatched = temp;
	    movenum = index;
	  }
	}
      }
      /* if (movenum != -1) printf("     %d successful after delay\n", movenum); */
    }
  }
  else { /* only processes key presses / changes */
    p->vbcount = longestmatched = 0;
    /* first check 1-key moves */
    if ((padd & padRtop) && p->SpinAmount == 0) movenum = 2 + p->FacingRight; /* evade */
    else if ((padd & padRbottom) && p->SpinAmount == 0) movenum = 3 - p->FacingRight; /* evade */
    else {
      temp = 1; /* assume an attack is found */
      switch(padd & (padTriangle | padSquare | padCircle | padX)) {
      case padTriangle : movenum = 8; break;
      case padSquare   : movenum = 9; break;
      case padX        : movenum = 10; break;
      case padCircle   : movenum = 11; break;
      default          : temp = 0; break;
      }
      if (temp) { /* got one of the attack keys */
	/* tempkey still has the original padd value */
	if (tempkey & padLup) movenum -= 4;
	else if (tempkey & padLdown) movenum += 4;
      }
    }
    /* now check multi-key moves */
    for (Move = PMove, index = 0; index < MOVEKEYS; index++, Move++) {
      temp = Move->movetable[Move->movematch];
      /* if checking for forward/backward, ignore left/right */
      /* also, allow slop in directional (left) pad */
      if ((temp & padForward) || (temp & padBackward)) tempkey = padd & ~(padLleft | padLright | padLup | padLdown);
      else {
	tempkey = padd & ~(padForward | padBackward);
	switch(temp) {
	case padLup    :
	case padLdown  : tempkey &= ~(padLleft | padLright);
	  break;
	case padLleft  :
	case padLright : tempkey &= ~(padLup | padLdown);
	  break;
	default : break;
	}
      }
      /* does new key match this move? */
      if (temp != tempkey) {
	/* if (Move->movematch > 0) printf("key %d (%d of %d) didn't match %d - resetting\n", padd, Move->movematch, Move->movesize, index); */
	Move->movematch = 0; /* key didn't match - reset this move */
      }
      else { /* key matched */
	/* increment match counter for this move */
	Move->movematch++;
	temp = Move->movematch;
	/* printf("key %d (%d of %d) matched %d\n", padd, temp, Move->movesize, index); */
	/* is entire key sequence matched? */
	if (temp == Move->movesize && /* i.e. movematch == movesize */
	    temp > longestmatched) {
	  longestmatched = temp;
	  movenum = index;
	  if (movenum > 1) movenum += 14; /* skip 1-key attacks that were already checked above */
	}
      }
    }
    if (movenum < STAND) { /* got a new controller-initiated move */
      /* reset all key counters -- kill all moves in progress */
      for (index = 0, Move = PMove; index < MOVEKEYS; index++, Move++) Move->movematch = 0;
    }
  }
  return(movenum);
}

static void RecalcProbabilities(uchar *AttackProb, ushort movenum, short amount)
{
  register ushort i;
  register ushort flag;
  
  if (movenum < 16 && movenum > 3) {
    AttackProb = &AttackProb[((movenum >> 2) - 1) * 5];
    movenum %= 4;
    flag = 1;
    for (i = 0; i < 4; i++) {
      if (i == movenum) {
	if (AttackProb[i] + (amount << 1) - amount) flag = 0;
      }
      else if (AttackProb[i] < amount) flag = 0;
    }
    if (flag) for (i = 0; i < 4; i++) {
      if (i == movenum) AttackProb[i] += ((amount << 1) + amount);
      else AttackProb[i] -= amount;
    }
  }
  return;
}

static void JumpToFrame(Ptcl *p, ushort currframe, ushort targetframe)
{
  p->FrameNum = targetframe;
  targetframe -= currframe;
  p->posoffset = &((p->posoffset)[targetframe * 3]);
  p->animdata = &((p->animdata)[targetframe * p->NumOfTMDs]);
  return;
}

static void SetMove(Ptcl *p, ushort playernum, ushort Move, ushort StartAtLoop)
{
  AnimDetail *ad;
  
  if (!p->Human && (p->Damage > 0)) RecalcProbabilities(p->AttackProb, p->MoveNum, -1);
  p->RDamageCtr = p->RepeatDamage; /* reset damage counter */
  ad = &AnimDetails[playernum][Move];
  p->MoveNum = Move;
  p->LoopStart = ad->LoopStart; /* for moves 4-23, loop defines attack windows */
  p->LoopEnd = ad->LoopEnd;
  p->LoopCounter = ad->LoopCount;
  p->posoffset = ad->PosOffsetPtr;
  p->animdata = ad->AnimDataPtr;
  p->AttackObj[0] = ad->AttackObjNum[0];
  p->AttackObj[1] = ad->AttackObjNum[1];
  p->Damage = ad->Damage;
  p->Knockdown = ad->Knockdown;
  p->KicksUpDust = ad->KicksUpDust;
#if 0
  printf("P:%d M:%d\n", playernum, Move);
  if (ad->KicksUpDust)
    printf("Hmm! Dust!\n");
#endif
  p->FrameNum = 0;
  p->NextMove = STAND;
  if (StartAtLoop) JumpToFrame(p, 0, p->LoopStart);
}

static void RestartLoop(Ptcl *p, ushort playernum, ushort Move)
{
  AnimDetail *ad;
  register ushort targetframe;
  
  ad = &AnimDetails[playernum][Move];
  p->posoffset = ad->PosOffsetPtr;
  p->animdata = ad->AnimDataPtr;
  p->FrameNum = targetframe = p->LoopStart;
  p->posoffset = &((p->posoffset)[targetframe * 3]);
  p->animdata = &((p->animdata)[targetframe * p->NumOfTMDs]);
}

static short ChooseOffensiveMove(Ptcl *p, Ptcl *otherp)
{
  register ushort temp;
  register ushort hilow;
  register short retval;
  register ushort total;
  
  /* if was running back, don't suddenly run forward, it looks dumb */
  if (PlayerSep > 8100 && p->MoveNum != 1) { /* too far from opponent -- close in 360 squared  */
    if (PlayerSep > 10000 || p->MoveNum == 0) retval = 0; /* run forward */
    else retval = STAND+2; /* inch forward */
  }
  else {
    temp = 4 - Options.Level;
    if ((Rand() % 5) < temp) { /* do nothing, but stop running sometimes */
      if (p->MoveNum < 2 && (Rand() & 1)) retval = STAND;
      else if ((p->MoveNum == 1) && p->CloseToRune) retval = STAND;
      else retval = -1;
    }
    else {
      /* calc high/low based on where both fighters are */
      if ((otherp->MoveNum > 11 && otherp->MoveNum < 16) || /* low attacks */
	  otherp->MoveNum == STAND+1 || otherp->MoveNum == STAND+5) {
	/* other guy is low ... try a high attack, based on level */
	if (Rand() % 8 < Options.Level) hilow = 5 * (Rand() & 1); /* 50/50 */
	else hilow = 10;
      }
      else { /* opponent is standing */
	if (Rand() % 8 < Options.Level) hilow = 10; /* try a low attack */
	else hilow = 5 * (Rand() & 1); /* 50/50 split medium & high attacks */
      }
      temp = Rand() % 100;
      total = 0;
      /* satoru printf("looking for %d from %d -", temp, hilow);	*/
      for (retval = 0; retval < 4; retval++) {
	/* satoru printf(" %d", p->AttackProb[hilow + retval]);	*/
	total += p->AttackProb[hilow + retval];
	if (temp < total) break;
      }
      if (retval == 4 && Rand() % 100 < p->AttackProb[hilow + 4]) { /*printf(" combo!")*/; 
      } /* turn on combo flag!! */
      /*printf(" found %d", retval)*/;
    }
  }
  /*printf(" final %d", retval)*/;
  return(retval);
}

static short ChooseDefensiveMove(Ptcl *p, Ptcl *otherp)
{
  register short retval;
  SVECTOR temppos, templv;
  
  retval = Rand() % 10;
  switch (retval) {
  case 9  : /* run backwards */
    if (p->CloseToRune == 0 && PlayerSep < 4050 && /* half of normal distance */
	p->MoveNum != 0) { /* don't back up if running forwards - it looks dumb */
      retval = 1;
      break;
    }
    retval = Rand() % 9; /* pick another # */
  case 8  : /* evade - but be careful .. if the other guy's close to the */
  case 7  : /* rune, and I am too, evading may hit the rune */
  case 6  :
  case 5  : if (p->CloseToRune) {
    if (abs(abs(p->position.x) - abs(p->position.z)) < 375) {
      /* in corner - go berserk */
      retval = ChooseOffensiveMove(p, otherp);
    }
    else {
      copyVector(&temppos, &CameraPosition);
      Square0(&temppos, &templv); /* DistanceSquared(&CameraPosition, &zerovec); /* circular platform */
      if (templv.x + templv.z > p->CloseToRune) {
	/* evade away from camera */
	if (p->FacingRight) retval = 3; /* evade left */
	else retval = 2;
      }
      else {
	/* evade towards camera */
	if (p->FacingRight) retval = 2; /* evade right */
	else retval = 3;
      }
    }
  }
  else retval = 2 + (Rand() & 1); /* who cares */
  break;
  default : /* block */
    retval = STAND+4;
    if ((otherp->MoveNum > 11 && otherp->MoveNum < 16) || /* low attacks */
	otherp->MoveNum == STAND+1 || otherp->MoveNum == STAND+5) {
      retval++;
    }
    break;
  }
  return(retval);
}

static void CalcAImove(Ptcl *p, Ptcl *otherp, ushort LoopEnded)
{
  register short DesiredMove;
  register ushort temp;
  
  if (p->CalcedMove == -1) { /* don't already have a move */
    /* randomly do nothing / delay, based on level */
    temp = 4 - Options.Level;
    if ((Rand() % 5) < temp) temp = 0;
    else {
      /* check for end of loop on self and opponent */
      if ((p->FrameNum == 1 && p->MoveNum != 1) ||
	  LoopEnded || (otherp->FrameNum == 1) ||
	  (p->MoveNum == 0 && p->FrameNum > p->LoopStart)) temp = 1;
      else {
	if (otherp->MoveNum < 4 || otherp->MoveNum > STAND-1) {
	  if (otherp->FrameNum == otherp->LoopEnd) temp = 1;
	  else temp = 0;
	}
	else if (otherp->FrameNum == (otherp->LoopStart & 255) ||
		 otherp->FrameNum == (otherp->LoopEnd >> 8)) temp = 1;
	else temp = 0;
      }
    }
    if (temp) {
      /* either this player finished a move, or the other guy started or */
      /* finished one, so choose a new move to do */
      switch(p->MoveNum) { /* if down, stay down */
      case STAND+5 : /* crouch block */
      case      12 :
      case      13 :
      case      14 :
      case      15 : DesiredMove = STAND+1;
	break;
      default      : if (Rand() & 4) DesiredMove = STAND+1;
      else DesiredMove = -1;
      break;
      }
      switch(otherp->MoveNum) {
      case 32 : /* lying down - do nothing */
      case 33 :
      case 44 : /* falling down - ditto */
      case 45 :
      case 46 :
      case 47 :
      case 48 :
      case 49 :
      case 50 :
      case 51 : break;
      default :
	if ((otherp->MoveNum > 3 && otherp->MoveNum < STAND) && otherp->OpponentIsInFront) {
	  /* other guy is attacking - try to be defensive */
	  if ((Rand() % 24 - Options.Level) > (4 - Options.Level)) DesiredMove = ChooseDefensiveMove(p, otherp);
	  else DesiredMove = ChooseOffensiveMove(p, otherp);
	}
	else if (otherp->MoveNum < REACT) { /* other guy is not attacking -- try to attack */
	  if ((Rand() % 24 - Options.Level) < (4 - Options.Level)) DesiredMove = ChooseDefensiveMove(p, otherp);
	  else DesiredMove = ChooseOffensiveMove(p, otherp);
	}
	if (DesiredMove > 3 && DesiredMove < STAND) {
	  p->AttackDelay = (4 - Options.Level) << 2; /* this is sloppy, since it could be set early, or late ... */
	  if (DesiredMove > 15) p->AttackDelay *= ((PlayerMove *)(p->Keys))[DesiredMove - 16].movesize;
	}
	break;
      }
      /* satoru printf(" stored %d\n", DesiredMove);	*/
      p->CalcedMove = DesiredMove;
    }
  }
  return;
}

static void ProcessInput(ushort id)
{
  Ptcl  *p;
  register short i;
  register ushort j;
  register ushort numkeys = 4;
  register ushort movecomplete;
  register short  DesiredMove = -1;
  ulong paddbuff[8];
  /*  short DesMoves[8]; */
  
  /* copy interrupt controller buffer to work area */
  numkeys = paddbuffidx;
  for (i = 0; i < numkeys; i++) paddbuff[i] = paddbuffer[i];
  paddbuffidx = 0;
  
  /* implement player's desired moves, if possible */

  for (j = 0; j < 2; j++) {
    if (id)
      p = &Player[j];
    else
      p = &Player[1 - j];
    
    movecomplete = 0; /* flag for AI processing .. hit end of loop? */
    p->FrameNum++;
    /* check if looping needs to occur */
    if (p->FrameNum == p->LoopEnd && (p->MoveNum < 4 || p->MoveNum > STAND-1)) { /* leaving loop */
      movecomplete = 1;
      i = p->LoopCounter;
      if (i > 0) i--;
      if (i != 0) {
	RestartLoop(p, j, p->MoveNum);
	p->LoopCounter = i;
      }
    }
    
    for (i = 0; i < numkeys; i++) {
      /* determine next move */
      if (paddbuff[i] & ((padStart << 16) | padStart)) GameInProgress = 0; /* force exit */
      if (p->Human) DesiredMove = ProcessOneKey((ushort)((paddbuff[i] >> (j << 4)) & 0xffff), PMoves[j], p);
      else if (i == 0) {
	i = numkeys;
	CalcAImove(p, &Player[1 - j], movecomplete);
	if (p->AttackDelay) {
	  p->AttackDelay--;
	  DesiredMove = -1; /* change nothing */
	}
	else {
	  DesiredMove = p->CalcedMove;
	  p->CalcedMove = -1;
	}
      }
      /* *** insert combo processing here somewhere? */
      
      if (DesiredMove == p->MoveNum) DesiredMove = -1;
      if (p->NextMove != STAND) DesiredMove = p->NextMove;
      if (CombatTimer <= 0) DesiredMove = STAND;
      /* check if current move is done */
      if (p->FrameNum >= AnimDetails[j][p->MoveNum].NumFrames) {
	movecomplete = 1;
	/* switch from crouch to stand, so the correct switch() will process */
	if (p->MoveNum == STAND+1) p->MoveNum = STAND;
	if (DesiredMove < 0) { /* no designation of desired action, so ... */
	  if (p->MoveNum < 2 || (p->MoveNum >= STAND && p->MoveNum < STAND+8)) DesiredMove = p->MoveNum;
	  else DesiredMove = STAND;
	}
      }
      else {
	if (DesiredMove < 0) continue; /* go process next key */
	movecomplete = 0;
      }
      /* if (j == 0) printf("p%d move %d fnum %d lend %d lcount %d total %d mc %d\n", j, p->MoveNum, p->FrameNum, p->LoopEnd, p->LoopCounter, AnimDetails[j][p->MoveNum].NumFrames, movecomplete); */
      /* if (j == 0) FntPrint("p%d move %d des %d next %d fnum %d total %d mc %d\n", j, p->MoveNum, DesiredMove, p->NextMove, p->FrameNum, AnimDetails[j][p->MoveNum].NumFrames, movecomplete); */
      
      
      /* allow move changes if blocking, standing, crouching, inching, */
      /* or running, or when current move animation is done */
      if (!movecomplete && ((p->MoveNum > 1 && p->MoveNum < STAND) || p->MoveNum > STAND + 7)) {
	/* now, keep move if currently attacking, didn't just start it, */
	/* and desired move is another attack */
	if (DesiredMove < STAND && p->MoveNum < STAND && p->FrameNum > 1) {
	  if (p->NextMove < DesiredMove) p->NextMove = DesiredMove; /* only allow attacks of higher "priority" */
	}
      }
      else { /* move change allowed! */
	if (movecomplete || (p->MoveNum != DesiredMove)) switch(p->MoveNum) {
	  /* check if starting from crouch, and process accordingly */
	case STAND+1 : /* crouch - if not in loop, don't change move */
	  /* don't check movecomplete since completed crouch changed to stand */
	  if (p->FrameNum < p->LoopStart || p->FrameNum > p->LoopEnd) {
	    if (DesiredMove < REACT) {
	      if (p->NextMove == STAND) p->NextMove = DesiredMove;
	      break;
	    }
	  }
	  /* else fall through - player is crouching */
	case STAND+5 : /* crouch block */
	case      12 : /* low attack 1 */
	case      13 : /* low attack 2 */
	case      14 : /* low attack 3 */
	case      15 : /* low attack 4 */
	  /* if low attack/block desired, do it, else exit crouch first */
	  switch (DesiredMove) {
	  case -1 : /* no preference, so continue crouch */
	  case STAND+1 : /* crouch */
	    SetMove(p, j, STAND+1, 1); /* wanna crouch, already down, so jump into loop */
	    break;
	  case STAND+5 : /* crouch block */
	  case      12 : /* low attack 1 */
	  case      13 : /* low attack 2 */
	  case      14 : /* low attack 3 */
	  case      15 : /* low attack 4 */
	    SetMove(p, j, DesiredMove, 0);
	    break;
	  default :
	    if (DesiredMove > REACT - 3) { /* reaction - do immediately */
	      SetMove(p, j, DesiredMove, 0);
	    }
	    else { /* need to stand up before doing any other move */
	      SetMove(p, j, STAND+1, 0);
	      JumpToFrame(p, 0, p->LoopEnd); /* get out of crouch loop */
	      if (p->NextMove == STAND) p->NextMove = DesiredMove;
	    }
	    break;
	  }
	  break;
	default : /* start from crouch handled completely by this point */
	  /* if low attack/block desired, crouch first, else do it */
	  switch (DesiredMove) {
	  case STAND+1 : /* crouch */
	  case STAND+5 : /* crouch block */
	  case      12 : /* low attack 1 */
	  case      13 : /* low attack 2 */
	  case      14 : /* low attack 3 */
	  case      15 : /* low attack 4 */
	    if (p->MoveNum < REACT-2) {
	      SetMove(p, j, STAND+1, 0); /* start crouching */
	      if (DesiredMove != STAND+1) p->NextMove = DesiredMove;
	      break;
	    }
	    /* else fall through -- must allow reactions */
	  default : /* standing moves -- do them! */
	    switch(p->MoveNum) {
	    case 42 : /* react to ground hit face down */
	    case 44 : /* fall forwards */
	    case 45 :
	    case 46 :
	    case 50 : SetMove(p, j, REACT-2, 0); /* lie face down */
	      p->HitRune = 0;
	      if (p->HPleft > 0) p->LoopCounter = 3; /* max damage */
	      else p->LoopCounter = 255;
	      break;
	    case 43 : /* react to ground hit face up */
	    case 47 : /* fall backwards */
	    case 48 :
	    case 49 :
	    case 51 : SetMove(p, j, REACT-1, 0); /* lie face up */
	      p->HitRune = 0;
	      if (p->HPleft > 0) p->LoopCounter = 3; /* max damage */
	      else p->LoopCounter = 255;
	      break;
	    case STAND+4 : /* standing block */
	      if (!movecomplete) { /* block in progress */
		if (DesiredMove == STAND || DesiredMove == STAND+5) {
		  /* stop block cleanly before starting next anim */
		  if (p->FrameNum < p->LoopEnd) JumpToFrame(p, p->FrameNum, p->LoopEnd);
		  if (p->NextMove == STAND) p->NextMove = DesiredMove;
		  break;
		}
	      } /* else end of move, so do next one */
	      SetMove(p, j, DesiredMove, 0);
	      break;
	    case 0       : /* run forwards */
	    case 1       : /* run back */
	    case STAND+2 : /* inch forwards */
	    case STAND+3 : /* inch back */
	      if (!movecomplete && DesiredMove == STAND) {
		/* stop running cleanly before starting next anim */
		if (p->FrameNum < p->LoopEnd) JumpToFrame(p, p->FrameNum, p->LoopEnd);
		if (p->NextMove == STAND) p->NextMove = DesiredMove;
		break;
	      }
	      /* else ok to change move now - don't fall through, why waste time on the if? */
	      SetMove(p, j, DesiredMove, 0);
	      break;
	    default : if ((p->MoveNum < REACT-2) || movecomplete) SetMove(p, j, DesiredMove, 0);
	      break;
	    }
	    break;
	  }
	  break;
	}
      }
    }
  }
  return;
}


#ifdef DRAW_SHADOWS

#ifdef FAKE_SHADOW

static void ProcessShadows()
{
  float    camHeight, shadowHeight, *tmp;
  float    start_seg, delta_seg;
  int32	   i;
  Err      err;
  Bitmap 	     *bitmap;


  shadowHeight = 0.1+SHADOW_Y*( (Shad_Player_Dist+100.0)*RC_SHADOW_SIZE );

  /*
   * Make safe area around texture
   */

  camHeight = FIXED_CAMERA_HEIGHT;  /* Make safe area around texture */

  /*
   * Now modify the shadow quad
   */
  camHeight *= 0.5;

  start_seg = -1.0;
  delta_seg = 2.0*RC_SHADOW_SEG;

  for ( i=0; i<SHADOW_SEG; i++, start_seg += delta_seg )
    {
      tmp = shadowPods[i].pgeometry->pvertex;
      tmp[1] = tmp[7] = tmp[13] = tmp[19] = shadowHeight;
      tmp[0] = tmp[12] = -1*camHeight + Player[0].position.x;
      tmp[6] = tmp [18] = camHeight + Player[0].position.x;      
      
      tmp[2] = tmp[8] = camHeight * start_seg + Player[0].position.z;
      tmp[14] = tmp[20] = camHeight *(start_seg+delta_seg) + Player[0].position.z;
  }
  /*
   * Switch buffers
   */
  
  shadowHeight = 0.5+SHADOW_Y*((Shad_Player_Dist+100.0)*RC_SHADOW_SIZE);

  camHeight = FIXED_CAMERA_HEIGHT;  /* Make safe area around texture */
  
  /* Now modify the shadow quad */
  camHeight *= 0.5;
  
  start_seg = -1.0;
  delta_seg = 2.0*RC_SHADOW_SEG;
  for (i=SHADOW_SEG; i<ALLOC_SHADOW_SEG; i++, start_seg += delta_seg)
    {
      tmp = shadowPods[i].pgeometry->pvertex;
      
      tmp[1] = tmp[7] = tmp[13] = tmp[19] = shadowHeight;
      tmp[0] = tmp[12] = -1*camHeight + Player[1].position.x;
      tmp[6] = tmp [18] = camHeight + Player[1].position.x;      

      tmp[2] = tmp[8] = camHeight * start_seg + Player[1].position.z;
      tmp[14] = tmp[20] = camHeight *(start_seg+delta_seg) + Player[1].position.z;
    }
}

#else  /* Not a FAKE SHADOW */

#ifdef BLINN_SHADOW

static void ProcessShadows()
{
  float    camHeight, shadowHeight, *tmp;
  float    start_seg, delta_seg;
  int32	   i, numPods;
  Err      err;
  Bitmap 	     *bitmap;
  Matrix   squashMatrix = 
  {
    1.0, 0.0, 0.0,
    0.0, 0.0, -1.0,
    0.0, 0.0, 1.0,
    0.0, SHADOW_Y, 1.0
  };


  numPods = shadowTrans[0][0].numNodes;
  for (i=1; i<= numPods; i++)
    {
      Matrix_Copy(&shadowTrans[0][i].Squash, shadowTrans[0][i].Original);
#if 0
      Matrix_Translate(&(shadowTrans[0][i].Squash), 0.0, 0.0, -50.0);
#else
      Matrix_Multiply(&(shadowTrans[0][i].Squash), &squashMatrix);
#endif
    }
  numPods = shadowTrans[1][0].numNodes;
  for (i=1; i<= numPods; i++)
    {
      Matrix_Copy(&shadowTrans[1][i].Squash, shadowTrans[1][i].Original);
#if 0
      Matrix_Translate(&(shadowTrans[1][i].Squash), 0.0, 0.0, 50.0);
#else
      Matrix_Multiply(&(shadowTrans[1][i].Squash), &squashMatrix);
#endif
    }
}

#else /* Not A Blinn Shadow */

static void ShadowEnv_Create(GraphicsEnv **shadowEnv)
{
  GraphicsEnv *shadowEnv0;
  Err err;
  
  *shadowEnv = shadowEnv0 = GraphicsEnv_Create();
  if (shadowEnv0 == NULL) 
    {
      printf("Failed to allocate Shadow Graphics Environment\n");
      exit(1);
    }
  /*
   * Just allocate a bitmap, not a display
   */
  shadowEnv0->d = NULL;  /* NO DISPLAY! */
  err = GS_AllocBitmaps( shadowEnv0->bitmaps, IB_SHADOW_WIDTH, IB_SHADOW_HEIGHT, BMTYPE_16, 1, 0 );
  shadowEnv0->gs = GS_Create();
  if (err < 0) {
    printf("Couldn't allocate bitmaps.  Exiting.\n");
    PrintfSysErr(err);
    exit(err);
  }
  err = GS_AllocLists(shadowEnv0->gs, kNumCmdListBufs, kCmdListBufSize);
  GS_SetDestBuffer(shadowEnv0->gs, shadowEnv0->bitmaps[0]);
  
  if (err < 0)
    {
      printf("Couldn't initialize shadow environment 0.  Exiting.\n");
      PrintfSysErr(err);
      exit(err);
    }
}

static void SetupShadows(GraphicsEnv *shadowEnv, AppData* shadowData)
{
  ViewPyramid  shadowVP;

  shadowData->curRenderScreen = 0;
  shadowData->close = M_Init(2000, 0, shadowEnv->gs);
  printf("Here ShadowEnv->gs=%x\t ShadowData->close=%x\n", shadowEnv->gs, shadowData->close);
#ifdef SHADOW_USE_MP
  M_InitMP(shadowData->close, 50, 32*1024 , 300);
#endif
  if (shadowData->close == NULL) {
    printf("Couldn't init Mercury. Exiting\n");
    exit(1);
  }

  shadowData->close->fwclose = 1.01;
  shadowData->close->fwfar = 100000.0;
  shadowData->close->fscreenwidth = IB_SHADOW_WIDTH;
  shadowData->close->fscreenheight = IB_SHADOW_HEIGHT;
  shadowData->close->depth = IB_SHADOW_DEPTH;

  shadowData->matrixSkew = AllocMem(sizeof(Matrix), MEMTYPE_NORMAL);
  if (shadowData->matrixSkew == NULL) {
    printf("Couldn't allocate skew shadow matrix.  Exiting\n");
    exit(1);
  }

  /*
   * Perfect square View Pyramid for shadow
   */
  shadowVP.left = -1.0;
  shadowVP.right = 1.0;
  shadowVP.top = 1.0;
  shadowVP.bottom = -1.0;
  shadowVP.hither = 2.0;  /* 1.2 */
  Matrix_Perspective(shadowData->matrixSkew, &shadowVP, 0.0, (float)(IB_SHADOW_WIDTH),
		     0.0, (float)(IB_SHADOW_HEIGHT), 1.0/10.0);

  printf("Additional camera setup...\n");
  shadowData->matrixCamera = Matrix_Construct();
  if (shadowData->matrixCamera == NULL) {
    printf("Couldn't allocate shadow camera matrix.  Exiting\n");	exit(1);
  }

  Matrix_Identity(shadowData->matrixCamera);
  Matrix_RotateX(shadowData->matrixCamera, -3.14927/2.0);
  Matrix_Translate(shadowData->matrixCamera, 0.0, 800, 0.0 );
  M_SetCamera(shadowData->close, shadowData->matrixCamera, shadowData->matrixSkew);

}

#ifdef SINGLE_SHADOW

static void ProcessShadows(GraphicsEnv *shadowEnv, AppData* shadowData, Pod* shadowPods,
			   Pod *shadowPod0, Ptcl *player, float offset)
{
#define SHADOW_LATENCY 4*1024
  Vector3  CameraTarget;
  float    camHeight, shadowHeight, *tmp;
  float    start_seg, delta_seg;
  int32	   i;
  Err      err;
  Bitmap 	     *bitmap;

  camHeight = Shad_Player_Dist;

  CameraTarget.x = Player[0].position.x - ((Player[0].position.x - Player[1].position.x) / 2);
  CameraTarget.y = 50.0f;
  CameraTarget.z = Player[0].position.z - ((Player[0].position.z - Player[1].position.z) / 2);

  GS_SetDestBuffer(shadowEnv->gs, shadowEnv->bitmaps[0]);
  GS_SetDestBuffer(shadowEnv->gs, shadowEnv->bitmaps[1]);
  GS_SetDestBuffer(shadowEnv->gs, shadowEnv->bitmaps[0]);


  M_DBNoBlend(GS_Ptr(shadowEnv->gs));
  CLT_ClearFrameBuffer(shadowEnv->gs, 0.0, 0.0, 0.0, 1.0, TRUE, FALSE);

  Matrix_Identity( shadowData->matrixCamera );
  Matrix_RotateX( shadowData->matrixCamera, -3.14927/2.0 );

  shadowHeight = offset+SHADOW_Y*( (camHeight+100.0)*RC_SHADOW_SIZE );

  /*
   * Make safe area around texture
   */

  camHeight += 120.0;  /* Make safe area around texture */
  Matrix_Translate(shadowData->matrixCamera, CameraTarget.x, camHeight, CameraTarget.z );

  M_SetCamera( shadowData->close, shadowData->matrixCamera, shadowData->matrixSkew );

  /*
   * Now modify the shadow quad
   */
  camHeight *= 0.5;

  start_seg = -1.0;
  delta_seg = 2.0*RC_SHADOW_SEG;

  for ( i=0; i<SHADOW_SEG; i++, start_seg += delta_seg )
    {
      tmp = shadowPods[i].pgeometry->pvertex;
      tmp[1] = tmp[7] = tmp[13] = tmp[19] = shadowHeight;
      tmp[0] = tmp[12] = -1*camHeight + CameraTarget.x;
      tmp[6] = tmp [18] = camHeight + CameraTarget.x;      
      
      tmp[2] = tmp[8] = camHeight * start_seg + CameraTarget.z;
      tmp[14] = tmp[20] = camHeight *(start_seg+delta_seg) + CameraTarget.z;
    }

  CLT_Sync(GS_Ptr(shadowEnv->gs));


#ifdef SHADOW_USE_LOW_LATENCY
  err = GS_LowLatency(shadowEnv->gs, 1, SHADOW_LATENCY);
  if (err<0)
    PrintfSysErr(err);
#endif
  M_Draw(shadowPod0, shadowData->close);
#ifdef SHADOW_USE_LOW_LATENCY
  GS_SendLastList(shadowEnv->gs);
#else
  GS_SendList(shadowEnv->gs);    
#endif
#ifdef SHADOW_USE_MP
  M_DrawEnd(shadowData->close);
#endif

  bitmap = (Bitmap *)LookupItem(shadowEnv->bitmaps[0]);  
  /*
   * Set the texture to point to this guy
   */
  
  for (i=0; i<SHADOW_SEG; i++)
    {
      shadowPods[i].ptexture->ptexture = 
	(uint32 *)(((uint32)bitmap->bm_Buffer)+
		   (uint32)(i*2*IB_SHADOW_WIDTH*IB_SHADOW_HEIGHT*RC_SHADOW_SEG));
    }
}

#else /* NOT A SINGLE SHADOW*/

static void ProcessShadows(GraphicsEnv *shadowEnv, AppData* shadowData, Pod *shadowPods, 
			   Pod *shadowPod0, Ptcl *player, float offset)
{
#define SHADOW_LATENCY 4*1024
  float    camHeight, shadowHeight, *tmp;
  float    start_seg, delta_seg;
  int32	   i, start, finish;
  Err      err;
  Bitmap 	     *bitmap;

  /*
    GS_Reserve(shadowEnv->gs, M_DBInit_Size);
    M_DBInit(GS_Ptr(shadowEnv->gs), 0, 0, IB_SHADOW_WIDTH, IB_SHADOW_HEIGHT);
    */

  M_DBNoBlend(GS_Ptr(shadowEnv->gs));
  CLT_ClearFrameBuffer(shadowEnv->gs, 0.0, 0.0, 0.0, 1.0, TRUE, FALSE);

  Matrix_Identity( shadowData->matrixCamera );
  Matrix_RotateX( shadowData->matrixCamera, -3.14927/2.0 );

  shadowHeight = offset+SHADOW_Y;
  
  /*
   * Make safe area around texture
   */

  camHeight = FIXED_CAMERA_HEIGHT;  /* Make safe area around texture */

  Matrix_Translate(shadowData->matrixCamera, player->position.x, camHeight, 
		   player->position.z );

  M_SetCamera( shadowData->close, shadowData->matrixCamera, shadowData->matrixSkew );

  /*
   * Now modify the shadow quad
   */
  camHeight *= 0.5;

  start_seg = -1.0;
  delta_seg = 2.0*RC_SHADOW_SEG;

  for ( i=0; i<SHADOW_SEG; i++, start_seg += delta_seg ) {

    tmp = shadowPods[i].pgeometry->pvertex;
    tmp[1] = tmp[7] = tmp[13] = tmp[19] = shadowHeight;
    tmp[0] = tmp[12] = -1*camHeight + player->position.x;
    tmp[6] = tmp [18] = camHeight + player->position.x;      
    
    tmp[2] = tmp[8] = camHeight * start_seg + player->position.z;
    tmp[14] = tmp[20] = camHeight *(start_seg+delta_seg) + player->position.z;
  }

  CLT_Sync(GS_Ptr(shadowEnv->gs));
  
#ifdef SHADOW_USE_LOW_LATENCY
  err = GS_LowLatency(shadowEnv->gs, 1, SHADOW_LATENCY);
  if (err<0)
    PrintfSysErr(err);
#endif
  M_Draw(shadowPod0, shadowData->close);
#ifdef SHADOW_USE_LOW_LATENCY
  GS_SendLastList(shadowEnv->gs);
#else
  GS_SendList(shadowEnv->gs);    
#endif
#ifdef SHADOW_USE_MP
  M_DrawEnd(shadowData->close);
#endif

  bitmap = (Bitmap *)LookupItem(shadowEnv->bitmaps[0]);  
  /*
   * Set the texture to point to this guy
   */
    
  for (i=0; i<SHADOW_SEG; i++)
    {
      shadowPods[i].ptexture->ptexture = 
	(uint32 *)(((uint32)bitmap->bm_Buffer)+
		   (uint32)(i*2*IB_SHADOW_WIDTH*IB_SHADOW_HEIGHT*RC_SHADOW_SEG));
    }
}

#endif  /* SINGLE_SHADOW */
#endif /* BLINN_SHADOW */
#endif /* FAKE_SHADOW */
#endif /* DRAW_SHADOWS */

/* #define CAMERA_AWAY 38025 */
#define CAMERA_AWAY  7000

static void ProcessCamera(AppData* appData)
{
  float temp;
  int32 camcheck, i;
  Vector3 CameraTarget, CameraVec, tempsvec;
  Vector3  templvec, newloc, satoru;
  
  /* target is midway between the 2 players */
  CameraTarget.x = Player[0].position.x - ((Player[0].position.x - Player[1].position.x) / 2);
  CameraTarget.y = 50;
  CameraTarget.z = Player[0].position.z - ((Player[0].position.z - Player[1].position.z) / 2);
  
  /* need to pull camera back from CameraTarget */
  /* the vector equation of a line is vr = vr0 + s(vm) */
  /* in this case vr = newloc (where we want it to be), */
  /*              vr0 = CameraTarget (the midpoint between the players), and */
  /*              vm = normal vector to line between players (= templvec) */
  
  /* calculate normal of line between players -- note abs on x, not on z! */
  CameraVec.x = tempsvec.x = abs(Player[1].position.x - Player[0].position.x);
  tempsvec.y = 0;
  CameraVec.y = 10;
  CameraVec.z = tempsvec.z = Player[1].position.z - Player[0].position.z;
  
  /*	Vec3_Cross(&tempsvec, &CameraVec);	/*    CrossProduct0(&tempsvec, &CameraVec); */
  /*	newloc.x = tempsvec.x; */
  /*	newloc.y = tempsvec.y; */
  /*	newloc.z = tempsvec.z; */
  
  Vector3D_Cross(&newloc, &tempsvec, &CameraVec);
  
  /*	Vec3_Normalize(&newloc);	/*    VectorNormal(&tempsvec, &newloc); */
  Vector3D_Normalize(&newloc);
  
  /* now calculate the point that is as far in front of the 2 characters as */
  /* they are apart from each other */
  
  /*	temp = Pt3_Distance(&Player[0].position, &Player[1].position); */
  
  Vector3D_Subtract(&satoru, &Player[0].position, &Player[1].position);
 
  temp = Vector3D_Length(&satoru);
  Shad_Player_Dist = temp;

  newloc.y = (temp / 6); /* set camera's fixed vertical distance from floor */
  
  /***********************************************************/
  if (newloc.y < 60) newloc.y = 60; /* otherwise floor will clip */
  else if (newloc.y > 65) newloc.y = 65; /* otherwise bottom of backdrop will show */
  /*temp += 100; /* to keep players on camera better */
  temp += 50; /* to keep players on camera better */
  
  /* make sure the camera's not too close to either player */
  camcheck = 1;
 

  while (camcheck) {
    CameraVec.x = (newloc.x * temp) + CameraTarget.x;
    CameraVec.y = 0; /* since char position is at 0 */
    CameraVec.z = (newloc.z * temp) + CameraTarget.z;
    camcheck = 0;
    
    if (DistanceSquared(&Player[0].position, &CameraVec) < CAMERA_AWAY) {	
      temp += 10;
      camcheck = 1;
    }
    if (DistanceSquared(&Player[1].position, &CameraVec) < CAMERA_AWAY) {
      temp += 10;           
      camcheck = 1;
    }
  }
  /***********************************************************/	
  /* now CameraVec.x & z contain the desired numbers */
  
  
  Matrix_GetTranslation(appData->matrixCamera, &CameraPosition);
  
  /* apply camera velocity to desired movement */
  templvec.x = (CameraVec.x - CameraPosition.x) / 4;
  templvec.y = (newloc.y - CameraPosition.y) / 4;
  templvec.z = (CameraVec.z - CameraPosition.z) / 4;
  
  if (templvec.x != 0 || templvec.y != 0 || templvec.z != 0) {
    /*        Vec3_Add(&CameraPosition, &templvec); /* calc new pos */
    
    /*		Char_Translate(cam, templvec.x,  */
    /*							templvec.y, */
    /*							templvec.z); */
    
    /*		Vector3D_Print(&templvec); */
    
    Matrix_TranslateByVector(appData->matrixCamera, &templvec);
    
    Matrix_GetTranslation(appData->matrixCamera, &CameraPosition);
    
    
    /* point between players */
    /*		Char_LookAt(cam, &CameraTarget, 0); */
    Matrix_LookAt(	appData->matrixCamera, 
			(Point3D*)&CameraTarget, 
			(Point3D*)&CameraPosition, 0.0f);
    
  }
  return;
}

static void CheckForHits(Ptcl *p, Ptcl *otherp)
{
  /* When a sword is swung, a point on the base and another at the tip both */
  /* travel through space.  From one animation frame to the next, a "current" */
  /* and "last" position are kept for both.  Three of these points LMN in 3D */
  /* space are used to define a plane whose normal (A, B, C) is the cross- */
  /* product of the two vectors these three points create.  The plane's */
  /* equation, then, is Ax + By + Cz = D, where D = AL0 + BL1 + CL1 (L1, L2 */
  /* and L3 are x, y and z from any one of the 3 points).  We can check where */
  /* this plane intersects the target player. */
  
  /* Each object of each player gets its base and tip points calculated */
  /* automatically in each of the x, y and z axes.  These points define a */
  /* segment of a line whose equation is x = x1 + t(x2 - x1) etc.  Hits are */
  /* detected when the intersection between the above plane and the line is */
  /* within the line segment (0 <= t <= 1) and also within the 4 points of */
  /* the plane. */
  
  /* At the point of intersection, the equation of the line == the equation */
  /* of the plane, so substitute the individual x, y, and z equations from */
  /* the line into the plane formula and solve for t. */
  /* The answer is:      AL0 - Ax1 + BL1 - By1 + CL2 - Cz1 */
  /*                 t = --------------------------------- */
  /*                     Ax2 - Ax1 + By2 - By1 + Cz2 - Cz1 */
  
  ObjDetail *Obj;
  ObjDetail *OtherObj;
  register ushort index = 0;
  register ushort otherindex = 0;
  register ushort axisnum = 0;
  register ushort otheraxis = 0;
  register ushort axisindex = 0;
  register ushort temp = 0;
  register long   tnumer = 0;
  register long   tdenom = 0;
  SVECTOR PlaneNormal, tempsv, base, tip, otherbase, othertip, lasttip, lastbase;
  Vector3D Satoru3D;
  
  /* if not attacking, leave */
  if (p->MoveNum < 4 || p->MoveNum > STAND-1){
    /*printf("Not attacking\n"); */
    return;
  }
  /* if not in attack window, leave */
  if ((p->FrameNum > (p->LoopStart >> 8) && p->FrameNum < (p->LoopStart & 255)) 
      ||
      (p->FrameNum > (p->LoopEnd >> 8) && p->FrameNum < (p->LoopEnd & 255))) 
    {
      /*	printf("In attack window\n"); */
    }
  else {
    /*printf("Not in attack window\n"); */
    return;
  }
  /* if other player is undamageable, leave */
  if (otherp->MoveNum == 2 || otherp->MoveNum == 3 || otherp->MoveNum == REACT-2 || otherp->MoveNum == REACT-1){
    /*printf("Other player is undamageable\n"); */
    return;
  }
  for (index = 0; index < 2; index++) {
    tnumer = p->AttackObj[index];
    if (tnumer < 0) continue;
    Obj = p->Obj;
    Obj = &(Obj[tnumer]);
    
    /*		printf("tnumer: %d \n", tnumer);  */
    
    axisindex = Obj->StartAxis;
    for (axisnum = 0; axisnum < 2; axisnum++, axisindex++) {
      if (axisindex == 3) axisindex = 0;
      
      /*			printf("axisindex: %hd \n", axisindex); */
      
      /*			printf("Obj->CurrTip[axisindex].x: %f\n", Obj->CurrTip[axisindex].x); */
      /*			printf("Obj->LastTip[axisindex].x: %f\n", Obj->LastTip[axisindex].x); */
      
      copyVector(&base, &Obj->CurrBase[axisindex]);
      copyVector(&tip, &Obj->CurrTip[axisindex]);
      copyVector(&lasttip, &Obj->LastTip[axisindex]);
      
      /* calculate normal to this object's plane of motion */
      
      PlaneNormal.x = tip.x - lasttip.x;
      PlaneNormal.y = tip.y - lasttip.y;
      PlaneNormal.z = tip.z - lasttip.z;
      
      /*            if (PlaneNormal.x == 0 && PlaneNormal.y == 0 && PlaneNormal.z == 0) continue; /* no movement??!! */
      tempsv.x = tip.x - base.x;
      tempsv.y = tip.y - base.y;
      tempsv.z = tip.z - base.z;
      /*            if (PlaneNormal.x == 0 && PlaneNormal.y == 0 && PlaneNormal.z == 0) continue; /* no movement??!! */
      
      /*Satoru Mer	Vec3_Cross(&PlaneNormal, &tempsv); /*  CrossProduct0(&PlaneNormal, &tempsv); */
      Vector3D_Cross(&Satoru3D, &PlaneNormal, &tempsv);
      copyVector(&PlaneNormal, &Satoru3D);
      
      
      if (PlaneNormal.x == 0 && PlaneNormal.y == 0 && PlaneNormal.z == 0) continue; /* no normal??!! */
      
      /* printf("normal = (%d,%d,%d)\n", PlaneNormal.x, PlaneNormal.y, PlaneNormal.z); */
      
      for (otherindex = 0, OtherObj = otherp->Obj; otherindex < otherp->NumOfTMDs; otherindex++, OtherObj++) {
	for (otheraxis = 0; otheraxis < 3; otheraxis++) {
	  /* calculate t -- if 0 <= t <= 1, plane cuts line */
	  
	  copyVector(&othertip, &OtherObj->CurrTip[otheraxis]);
	  copyVector(&otherbase, &OtherObj->CurrBase[otheraxis]);
	  tnumer = (long)PlaneNormal.x * ((long)tip.x - (long)otherbase.x) +
	    (long)PlaneNormal.y * ((long)tip.y - (long)otherbase.y) +
	    (long)PlaneNormal.z * ((long)tip.z - (long)otherbase.z);
	  tdenom = (long)PlaneNormal.x * ((long)othertip.x - (long)otherbase.x) +
	    (long)PlaneNormal.y * ((long)othertip.y - (long)otherbase.y) +
	    (long)PlaneNormal.z * ((long)othertip.z - (long)otherbase.z);
	  if ((tnumer > 0 && tdenom < 0) || (tnumer < 0 && tdenom > 0)) {
	    continue; /* point is below base, not between base and tip */
	  }
	  
	  /* check for no intersection on line segment or line parallel to plane */
	  if ((labs(tnumer) > labs(tdenom)) || (tdenom == 0)) continue;
	  if (tnumer == 0) copyVector(&tempsv, &otherbase);
	  else if (tnumer == tdenom) copyVector(&tempsv, &othertip);
	  else { /* intersects somewhere on line segment -- calculate it! */
	    tempsv.x = otherbase.x + (short)((tnumer * (long)(othertip.x - otherbase.x)) / tdenom);
	    tempsv.y = otherbase.y + (short)((tnumer * (long)(othertip.y - otherbase.y)) / tdenom);
	    tempsv.z = otherbase.z + (short)((tnumer * (long)(othertip.z - otherbase.z)) / tdenom);
	  }
	  /* tempsv now contains valid intersection point, but is it */
	  /* within the quadratic defined by the 4 base-tip points of */
	  /* the attacker (i.e. is it in the weapon's range)? */
	  /* to answer, basically draw a line from each of the 4 points */
	  /* to the intersection point (along each axis), and if ANY line */
	  /* is in the opposite direction of the other lines, the point */
	  /* is within, and if all lines go in the same direction, it's */
	  /* outside */
	  copyVector(&lastbase, &Obj->LastBase[axisindex]);
	  
	  if (base.x - tempsv.x < 0) temp = 0;
	  else temp = 1;
	  
	  
	  if (temp == (lastbase.x - tempsv.x < 0 ? 0 : 1)) {
	    if (temp == (tip.x - tempsv.x < 0 ? 0 : 1)) {
	      if (temp == (lasttip.x - tempsv.x < 0 ? 0 : 1)) continue;
	    }
	  }
	  
	  if (base.y - tempsv.y < 0) temp = 0;
	  else temp = 1;
	  if (temp == (lastbase.y - tempsv.y < 0 ? 0 : 1)) {
	    if (temp == (tip.y - tempsv.y < 0 ? 0 : 1)) {
	      if (temp == (lasttip.y - tempsv.y < 0 ? 0 : 1)) continue;
	    }
	  }
	  
	  if (base.z - tempsv.z < 0) temp = 0;
	  else temp = 1;
	  if (temp == (lastbase.z - tempsv.z < 0 ? 0 : 1)) {
	    if (temp == (tip.z - tempsv.z < 0 ? 0 : 1)) {
	      if (temp == (lasttip.z - tempsv.z < 0 ? 0 : 1)) continue;
	    }
	  }
	  
	  otheraxis = 3; /* stop checking this object after first hit */
	  /* save # of object that hit me, plus 1 cuz it might be 0 */
	  if (OtherObj->HitDetected == 0) {
	    OtherObj->HitDetected = p->AttackObj[index] + 1;
	  }
	  copyVector(&OtherObj->HitLoc, &tempsv);
	  
	  /*printf("tnumer = %ld, tdenom = %ld, plane normal = %d,%d,%d\n", tnumer, tdenom, PlaneNormal.x, PlaneNormal.y, PlaneNormal.z); */
	  /*printf("hit at %d,%d,%d between %d,%d,%d and %d,%d,%d (%d,%d)\n", tempsv.x, tempsv.y, tempsv.z, otherbase.x, otherbase.y, otherbase.z, othertip.x, othertip.y, othertip.z, otherp->position.x, otherp->position.z); */
	  /*printf("       attacker between %d,%d,%d and %d,%d,%d (%d,%d)\n", base.x, base.y, base.z, tip.x, tip.y, tip.z, p->position.x, p->position.z); */
	  /*printf("           and last pos %d,%d,%d and %d,%d,%d\n", lastbase.x, lastbase.y, lastbase.z, lasttip.x, lasttip.y, lasttip.z); */
	}
      }
    }
  }
  return;
}

static void ProcessHits(void)
{
  Ptcl *p;
  Ptcl *otherp;
  
  ObjDetail *Obj;
  POLY_FT4  *sp;
  Poof      *poof;
  register ushort i;
  register ushort index;
  register ushort hitoccurred;
  register short  hitDirection;
  register ushort blocked;
  register short  MaxY;
  register short  MinY;
  register short  ycenter;
  register ushort hit1;
  register ushort hit2;
  short centers[20];
  SVECTOR posoffset;
  
  /* determine min and max y of player's objects, and whether a hit occurred */
  
  for (i = 0, p = Player, otherp = &Player[1]; i < 2; i++, p = &Player[1], otherp = Player) 
    {
      hitoccurred = 0;
      for (index = 0, Obj = p->Obj; index < p->NumOfTMDs; index++, Obj++) {
	if (Obj->HitDetected) hitoccurred++;
      }
      if (hitoccurred == 0) continue;
      hitoccurred = MaxY = 0;
      MinY = -500;
      hit2 = 0; /* temp usage -- flag if weapon hit */
      for (index = 0, Obj = p->Obj; index < p->NumOfTMDs; index++, Obj++) {
	/* calculate object midpoint in y */
	hit1 = Obj->StartAxis; /* temp usage */
	ycenter = (Obj->CurrTip[hit1].y - Obj->CurrBase[hit1].y) / 2 + Obj->CurrBase[hit1].y;
	if (ycenter < MaxY) MaxY = ycenter; /* don't forget y is negative */
	else if (ycenter > MinY) MinY = ycenter;
	if (Obj->HitDetected) {
	  hitoccurred++;
	  if (index == p->Weapon1 || index == p->Weapon2) hit2 = 1; /* hit a weapon */
	  /* get hitting object (other player) */
	  /*                if (hitoccurred == 1) Hitter = &otherp->Obj[Obj->HitDetected - 1]; */
	  centers[index] = ycenter;
	}
	else centers[index] = 0;
      }
      if (hitoccurred > 0) { /* this player was hit */
	/* determine if hit is blocked */
	blocked = 0;
	hitDirection = p->OpponentIsInFront; /* 1 = react back */
	if (hitDirection) { /* block doesn't work when hit from behind */
	  if (p->MoveNum == STAND+4 && otherp->MoveNum < 12) blocked = 1;
	  if (p->MoveNum == STAND+5 && otherp->MoveNum > 7) blocked = 1;
	}
	/* move char back by a combination of distance between players and */
	/* some fraction of attacker's (CurrTip - LastTip) distance */
	if ((hitoccurred > 1) || !hit2) { /* don't do it if a weapons-only hit */
	  if (hitoccurred > 9) hit1 = 1; /* calc divisor = 10 - # of hits */
	  else hit1 = 10 - hitoccurred;
	  hit1 <<= 2;
	  /* DEBUG */
	  if (hit1 == 0)
	    {
	      printf("Here's your problem! Hit1=0\n");
	      return;
	    }
	  posoffset.x = (p->position.x - otherp->position.x) / hit1;
	  posoffset.y = 0;
	  posoffset.z = (p->position.z - otherp->position.z) / hit1;
	  /* printf("hit distance = x %d z %d, divisor = %d\n", posoffset.x, posoffset.z, hit1); */
	  if (posoffset.x > 25) posoffset.x = 25;
	  if (posoffset.z > 25) posoffset.z = 25;
	  
	  /*				Vec3_Add(&p->position, &posoffset);	/*addVector(&p->position, &posoffset); */
	  Vector3D_Add(&p->position, &p->position, &posoffset);
	  /*Satoru Mer	Char_Translate(Char_Obj[i][0], posoffset.x,  */
	  /*												posoffset.y, */
	  /*												posoffset.z); */
	  Matrix_Translate(Char_Obj[i][0], posoffset.x, 
			   posoffset.y,
			   posoffset.z);
	}
	
	if (hitoccurred == 1 && hit2 == 1) { /* hit weapon only */
	  /* **** play sound of weapon-weapon hit */
	}
	else if (blocked) { /* ***** play blocked sound */
	}
	else {
	  /* calculate damage, falldown percentage */
	  hit1 = 0; /* damage accumulator */
	  if (otherp->Damage > 0) { /* first time!!! */
	    hit1 = otherp->Damage;
	    otherp->Damage = 0;
	    if (Rand() % 100 < otherp->Knockdown) blocked = 1; /* temp usage - now a knockdown flag!! */
	    RecalcProbabilities(otherp->AttackProb, otherp->MoveNum, 2);
	    /* ***** play sound!! */
	  }
	  if (otherp->RDamageCtr > 0) {
	    hit1++;
	    otherp->RDamageCtr--;
	  }
	  if (hit1 > p->Armor) hit1 -= p->Armor;
	  else hit1 = 0;
	  /*                p->HPleft -= hit1; /* apply final damage amount */
	  if (p->HPleft <- 0) blocked = 1; /* force fall down when dead (temp usage) */
	  
	  /* set player's new move (react if not blocked) */
	  if (!p->HitRune) switch (p->MoveNum) {
	  case STAND+1 : /* crouch */
	  case STAND+5 : /* crouch block */
	  case      12 : /* low attack 1 */
	  case      13 : /* low attack 2 */
	  case      14 : /* low attack 3 */
	  case      15 : /* low attack 4 */
	    if (blocked) { /* fall down!! */
	      SetMove(p, i, REACT+16+hitDirection, 0);
	    }
	    else SetMove(p, i, REACT+6+hitDirection, 0);
	    break;
	  case      32 : /* lying face down */
	    SetMove(p, i, REACT+8, 0);
	    break;
	  case      33 : /* lying face up */
	    SetMove(p, i, REACT+9, 0);
	    break;
	  default :
	    if (p->MoveNum < REACT) {
	      /* determine if high, med or low react required */
	      MaxY -= MinY; /* get actual height of char */
	      MaxY /= 4; /* get 1/4 actual height */
	      index = MaxY * 3; /* temp usage */
	      hit1 = hit2 = hitoccurred = 0; /* temp usage */
	      for (index = 0; index < p->NumOfTMDs; index++) {
		ycenter = centers[index];
		if (ycenter != 0) {
		  ycenter -= MinY;
		  /* 4 ranges for hit1-3, 0=hi 1&2=med 3=low */
		  /* don't forget, y DECREASES as height INCREASES (-y is up) */
		  if (ycenter > (MaxY << 1)) hitoccurred += 1; /* low hit */
		  else if (ycenter > MaxY * 3) hit2 += 1; /* medium hit */
		  else hit1 += 1; /* high hit */
		}
	      }
	      if (hit1 > hit2) {
		if (hit1 > hitoccurred) ycenter = 0; /* high react */
		else ycenter = 2; /* low react */
	      }
	      else {
		if (hit2 > hitoccurred) ycenter = 1; /* med react */
		else ycenter = 2; /* low react */
	      }
	      
	      /* 3 times hitDirection */
	      if (blocked) { /* fall down!! */
		SetMove(p, i, REACT + hitDirection + hitDirection +
			hitDirection + ycenter + 10, 0);
	      }
	      else {
		SetMove(p, i, REACT + hitDirection + hitDirection +
			hitDirection + ycenter, 0);
	      }
	    }
	    break;
	  }
	  blocked = 0; /* reset to proper value */
	}
	/* set up block/blood poofs */
	hit1 = 0;
	for (index = 0, Obj = p->Obj; index < p->NumOfTMDs; index++, Obj++) {
	  if (Obj->HitDetected) {
	    Obj->HitDetected = 0;
	    /* only keep first one */
	    hit1++;
	    if (hit1 > 1) continue;
	    if (!blocked && (index == p->Weapon1 || index == p->Weapon2)) continue; /* ignore weapons */
	    /*satoru*/
#if 1
	    poof = &poofs[nextPoof];
	    copyVector(&poof->pos, &Obj->HitLoc);
	    if (blocked)
	      {
		poof->pooftype = 1; /* 1 = block, 0 = blood */
		poof->framenum = 8;
	      }
	    else
	      {
		poof->pooftype = 0;
		poof->framenum = 4;
	      }
	    sp = poof->sprite;
	    
	    /*					Trans_Init(poof->mat, NULL); */
	    /*					Trans_PostMul(poof->mat, Char_GetTransform(cam)); */
	    
	    
	    poof->vec1.x = poof->vec1.y = poof->vec2.y = poof->vec3.x = -12;
	    poof->vec1.z = poof->vec2.z = poof->vec3.z = poof->vec4.z = 0;
	    poof->vec2.x = poof->vec3.y = poof->vec4.x = poof->vec4.y = 12;
	    /*                        sp->tpage = sp[1].tpage = GetTPage(0, 0, 704, 0); */
	    /*                        sp->clut = sp[1].clut = GetClut(256, 483); */
	    sp[0].tc_data[0].u = 31;
	    sp[0].tc_data[0].v =  0;
	    sp[0].tc_data[1].u = 31;
	    sp[0].tc_data[1].v = 31;
	    sp[0].tc_data[2].u =  0;
	    sp[0].tc_data[2].v =  0;
	    sp[0].tc_data[3].u =  0;
	    sp[0].tc_data[3].v = 31;
	    
	    sp[0].color = 2130706432;
	    
	    /* setRGB0(sp, 255, 255, 255);  setRGB0(&sp[1], 255, 255, 255); */
	    nextPoof++;
	    if (nextPoof >= MAX_POOFS) nextPoof = 0;
#endif
	    /*satoru*/
	  }
	}
      }
    }
}


#define DUST_DELTAY  0.6

static void DrawPoofs(ushort id, AppData* appData, GState* gs)
{
  register ushort i;
  register int32 temp;
  Poof *poof;
  POLY_FT4 *sp;
  Pod *pod;

  uint32 *snipData;

  SVECTOR	tempv;

  
  for (i = 0, poof = poofs; i < MAX_POOFS; i++, poof++) {
    if (poof->framenum > 0) {
      sp = &poof->sprite[0];
      if (poof->pooftype < 10) {          /* calc poofs that always face camera	*/
	Matrix_Identity(&poof->mat);
	Matrix_MultiplyOrientation(&poof->mat, appData->matrixCamera);
      }
      pod = &(poofPods[i]);
      
      copyVector(&sp->vtx_data[0], &poof->vec1);
      copyVector(&sp->vtx_data[1], &poof->vec2);
      copyVector(&sp->vtx_data[2], &poof->vec3);
      copyVector(&sp->vtx_data[3], &poof->vec4);
      
      Vector3D_OrientateByMatrix(&sp->vtx_data[0] ,&poof->mat);	
      Vector3D_OrientateByMatrix(&sp->vtx_data[1] ,&poof->mat);
      Vector3D_OrientateByMatrix(&sp->vtx_data[2] ,&poof->mat);
      Vector3D_OrientateByMatrix(&sp->vtx_data[3] ,&poof->mat);
      
      Vector3D_Add(&sp->vtx_data[0], &sp->vtx_data[0], &poof->pos);
      Vector3D_Add(&sp->vtx_data[1], &sp->vtx_data[1], &poof->pos);
      Vector3D_Add(&sp->vtx_data[2], &sp->vtx_data[2], &poof->pos);
      Vector3D_Add(&sp->vtx_data[3], &sp->vtx_data[3], &poof->pos);
      
      if (/*(flg & 0x80000000) == 0*/1) { /* no errors */
	switch (poof->pooftype) {
	case 0 : /* blood */
	  pod->ptexture = &(gBSDF5->textures[2]);
	  switch(poof->framenum) {
	  case  4 : 
	      sp->tc_data[0].u = 0.0;
	      sp->tc_data[1].u = 0.5;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 0.5;
	      sp->tc_data[0].v = 0.0;
	      sp->tc_data[1].v = 0.0;
	      sp->tc_data[2].v = 0.5;
	      sp->tc_data[3].v = 0.5;
	      break;
	    case  3 :
	      sp->tc_data[0].u = 0.5;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.5;
	      sp->tc_data[3].u = 1.0;
	      break;
	    case  2 : 	
	      sp->tc_data[0].v = 0.5;
	      sp->tc_data[1].v = 0.5;				
	      sp->tc_data[2].v = 1.0;		
	      sp->tc_data[3].v = 1.0;
	      sp->tc_data[0].u = 0.0;
	      sp->tc_data[1].u = 0.5;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 0.5;
	      break;
	    default :
	      sp->tc_data[0].u = 0.5;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.5;
	      sp->tc_data[3].u = 1.0;
	      break;
	  }
	  /* printf("blood\n"); */
	  break;
	case 1 : /* block */
	  pod->ptexture = &(gBSDF5->textures[3]);
	  switch(poof->framenum/2) 
	    {
	    case  4 : 
	      if (Rand() & 1) {
		poof->framenum = 6;
	      }
	      else {
		sp->tc_data[0].u = 0;
		sp->tc_data[1].u = 0.5;
		sp->tc_data[2].u = 0.0;
		sp->tc_data[3].u = 0.5;
		sp->tc_data[0].v = 0.0;
		sp->tc_data[1].v = 0.0;
		sp->tc_data[2].v = 0.5;
		sp->tc_data[3].v = 0.5;
	      }
	      break;
	    case  3 :
	      sp->tc_data[0].u = 0.5;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.5;
	      sp->tc_data[3].u = 1.0;
	      break;
	    case  2 : 	
	      sp->tc_data[0].v = 0.5;
	      sp->tc_data[1].v = 0.5;				
	      sp->tc_data[2].v = 1.0;		
	      sp->tc_data[3].v = 1.0;
	      sp->tc_data[0].u = 0.0;
	      sp->tc_data[1].u = 0.5;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 0.5;
	      break;
	    default :
	      sp->tc_data[0].u = 0.5;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.5;
	      sp->tc_data[3].u = 1.0;
	      break;
	    }
	  /*	  printf("Block\n"); */
	  break;

	default : /* rune poofs */
	  /*	  printf("Rune Poof\n"); */
	  temp = sp->color - 0x04000000;	/*temp = sp->r0 - 16  /*32; */
	  if (temp < 0x0C000000)
	    poof->framenum = 1;
	  sp->color = temp;	/*setRGB0(sp, temp, temp, temp); */
	  pod->ptexture = &(gBSDF5->textures[0]);
#if 0
	  switch (poof->framenum & 3))
	    {
	    case 3 :
	      sp->tc_data[0].u = 0.5;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.5;
	      sp->tc_data[3].u = 1.0;
	      break;
	    case 2 :
	      sp->tc_data[0].v = 0.5;
	      sp->tc_data[1].v = 0.5;				
	      sp->tc_data[2].v = 1.0;	
	    sp->tc_data[3].v = 1.0;
	    break;
	    case 1 :	
	      sp->tc_data[0].u = 0;
	      sp->tc_data[1].u = 0.5;
	      sp->tc_data[2].u = 0;
	      sp->tc_data[3].u = 0.5;
	      break;
	    default :
	      sp->tc_data[0].u = 0;
	      sp->tc_data[1].u = 0.5;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 0.5;
	      sp->tc_data[0].v = 0.0;
	      sp->tc_data[1].v = 0.0;
	      sp->tc_data[2].v = 0.5;
	      sp->tc_data[3].v = 0.5;
	      break;
	    }
#else
	  switch (Rand() & 3)
	    {
	    case 3 :
	      sp->tc_data[0].u = 0;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 1.0;
	      sp->tc_data[0].v = 0.0;
	      sp->tc_data[1].v = 0.0;
	      sp->tc_data[2].v = 0.25;
	      sp->tc_data[3].v = 0.25;
	      break;
	    case 2 :
	      sp->tc_data[0].u = 0;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 1.0;
	      sp->tc_data[0].v = 0.25;
	      sp->tc_data[1].v = 0.25;
	      sp->tc_data[2].v = 0.5;
	      sp->tc_data[3].v = 0.5;
	    break;
	    case 1 :
	      sp->tc_data[0].u = 0;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 1.0;
	      sp->tc_data[0].v = 0.5;
	      sp->tc_data[1].v = 0.5;
	      sp->tc_data[2].v = 0.75;
	      sp->tc_data[3].v = 0.75;
	      break;
	    default :
	      sp->tc_data[0].u = 0;
	      sp->tc_data[1].u = 1.0;
	      sp->tc_data[2].u = 0.0;
	      sp->tc_data[3].u = 1.0;
	      sp->tc_data[0].v = 0.75;
	      sp->tc_data[1].v = 0.75;
	      sp->tc_data[2].v = 1.0;
	      sp->tc_data[3].v = 1.0;
	      break;
	    }
	  snipData = pod->ptexture->ptpagesnippets->pselectsnippets[0].snippet.data;
	  snipData[23] = snipData[25] = (uint32)(sp->color);
#endif	  

	}
	pod->pgeometry->pvertex[0] = sp->vtx_data[0].x;
	pod->pgeometry->pvertex[1] = sp->vtx_data[0].y;
	pod->pgeometry->pvertex[2] = sp->vtx_data[0].z;
	  
	pod->pgeometry->pvertex[6] = sp->vtx_data[1].x;
	pod->pgeometry->pvertex[7] = sp->vtx_data[1].y;
	pod->pgeometry->pvertex[8] = sp->vtx_data[1].z;
	  
	pod->pgeometry->pvertex[12] = sp->vtx_data[2].x;
	pod->pgeometry->pvertex[13] = sp->vtx_data[2].y;
	pod->pgeometry->pvertex[14] = sp->vtx_data[2].z;
	  
	pod->pgeometry->pvertex[18] = sp->vtx_data[3].x;
	pod->pgeometry->pvertex[19] = sp->vtx_data[3].y;
	pod->pgeometry->pvertex[20] = sp->vtx_data[3].z;
	  	  
	pod->pgeometry->puv[0] = sp->tc_data[0].u;
	pod->pgeometry->puv[1] = sp->tc_data[0].v;
	pod->pgeometry->puv[2] = sp->tc_data[1].u;
	pod->pgeometry->puv[3] = sp->tc_data[1].v;
	pod->pgeometry->puv[4] = sp->tc_data[2].u;
	pod->pgeometry->puv[5] = sp->tc_data[2].v;
	  
	pod->pgeometry->puv[6] = sp->tc_data[2].u;
	pod->pgeometry->puv[7] = sp->tc_data[2].v;
	pod->pgeometry->puv[8] = sp->tc_data[3].u;
	pod->pgeometry->puv[9] = sp->tc_data[3].v;
	pod->pgeometry->puv[10] = sp->tc_data[1].u;
	pod->pgeometry->puv[11] = sp->tc_data[1].v;  
	poof->framenum = poof->framenum-1;
      }
    }
  }
 
  for (i = 0, poof = dustPoofs; i < MAX_DUST; i++, poof++) {
    if (poof->framenum > 0) {
      /*           sp = &poof->sprite[id]; */
      sp = &poof->sprite[0];
      if (poof->pooftype < 10) { /* calc poofs that always face camera */
	Matrix_Identity(&poof->mat);
	Matrix_MultiplyOrientation(&poof->mat, appData->matrixCamera);
      }
      else  /*SetMulMatrix(&world, &poof->mat)*/;
      
      poof->vec1.y += DUST_DELTAY;
      poof->vec2.y += DUST_DELTAY;
      poof->vec3.y += DUST_DELTAY;
      poof->vec4.y += DUST_DELTAY;
      copyVector(&sp->vtx_data[0], &poof->vec1);
      copyVector(&sp->vtx_data[1], &poof->vec2);
      copyVector(&sp->vtx_data[2], &poof->vec3);
      copyVector(&sp->vtx_data[3], &poof->vec4);
      

      Vector3D_OrientateByMatrix(&sp->vtx_data[0] ,&poof->mat);	
      Vector3D_OrientateByMatrix(&sp->vtx_data[1] ,&poof->mat);
      Vector3D_OrientateByMatrix(&sp->vtx_data[2] ,&poof->mat);
      Vector3D_OrientateByMatrix(&sp->vtx_data[3] ,&poof->mat);
      
      Vector3D_Add(&sp->vtx_data[0], &sp->vtx_data[0], &poof->pos);
      Vector3D_Add(&sp->vtx_data[1], &sp->vtx_data[1], &poof->pos);
      Vector3D_Add(&sp->vtx_data[2], &sp->vtx_data[2], &poof->pos);
      Vector3D_Add(&sp->vtx_data[3], &sp->vtx_data[3], &poof->pos);
      
      if (/*(flg & 0x80000000) == 0*/1) { /* no errors */
	temp = sp->color - 0x02000000;	/*temp = sp->r0 - 16  /*32; */
	if (temp < 0x03000000) 
	  poof->framenum = 1;
	sp->color = temp;	/*setRGB0(sp, temp, temp, temp); */
	  switch (((poof->framenum)>>2) & 3) {
	  case 3 :
	    sp->tc_data[2].u = 1.0;
	    sp->tc_data[0].u = 1.0;
	    sp->tc_data[3].u = 0.5;
	    sp->tc_data[1].u = 0.5;
	    break;
	  case 2 :
	    sp->tc_data[2].v = 0.5;
	    sp->tc_data[0].v = 1.0;
	    sp->tc_data[3].v = 0.5;
	    sp->tc_data[1].v = 1.0;
	    break;
	  case 1 :
	    sp->tc_data[2].u = 0.5;
	    sp->tc_data[0].u = 0.5;
	    sp->tc_data[3].u = 0.0;
	    sp->tc_data[1].u = 0.0;
	    break;
	  default :
	    sp->tc_data[2].v = 0.0;
	    sp->tc_data[0].v = 0.5;				
	    sp->tc_data[3].v = 0.0;		
	    sp->tc_data[1].v = 0.5;
	    break;
	  }
	  pod = &(dustPods[i]);

	  pod->pgeometry->pvertex[0] = sp->vtx_data[0].x;
	  pod->pgeometry->pvertex[1] = sp->vtx_data[0].y;
	  pod->pgeometry->pvertex[2] = sp->vtx_data[0].z;
	  
	  pod->pgeometry->pvertex[6] = sp->vtx_data[1].x;
	  pod->pgeometry->pvertex[7] = sp->vtx_data[1].y;
	  pod->pgeometry->pvertex[8] = sp->vtx_data[1].z;
	  
	  pod->pgeometry->pvertex[12] = sp->vtx_data[2].x;
	  pod->pgeometry->pvertex[13] = sp->vtx_data[2].y;
	  pod->pgeometry->pvertex[14] = sp->vtx_data[2].z;
	  
	  pod->pgeometry->pvertex[18] = sp->vtx_data[3].x;
	  pod->pgeometry->pvertex[19] = sp->vtx_data[3].y;
	  pod->pgeometry->pvertex[20] = sp->vtx_data[3].z;
	  
	  pod->pgeometry->puv[0] = sp->tc_data[0].u;
	  pod->pgeometry->puv[1] = sp->tc_data[0].v;
	  pod->pgeometry->puv[2] = sp->tc_data[1].u;
	  pod->pgeometry->puv[3] = sp->tc_data[1].v;
	  pod->pgeometry->puv[4] = sp->tc_data[2].u;
	  pod->pgeometry->puv[5] = sp->tc_data[2].v;
	  
	  pod->pgeometry->puv[6] = sp->tc_data[2].u;
	  pod->pgeometry->puv[7] = sp->tc_data[2].v;
	  pod->pgeometry->puv[8] = sp->tc_data[3].u;
	  pod->pgeometry->puv[9] = sp->tc_data[3].v;
	  pod->pgeometry->puv[10] = sp->tc_data[1].u;
	  pod->pgeometry->puv[11] = sp->tc_data[1].v;

	  snipData = pod->ptexture->ptpagesnippets->pselectsnippets[0].snippet.data;
	  snipData[23] = snipData[25] = (uint32)(sp->color);
      }
      poof->framenum--;
      poof->alpha -= poof->dAlpha;
      if (poof->alpha <= 0)
	poof->framenum = 0;
    }
  }
}

/*************************/

SpriteObj			*BG_Sprite;
int first_time_mainloop = 1;
int first_time_sprite = 1;
uint32 latency=4*1024;

int poofdist(Poof **p1, Poof **p2)
{
  return((int)((*p2)->distance - (*p1)->distance));
}


void mainloop(GraphicsEnv* genv, AppData* app)
{
  Pod                   *firstPod;
  Pod                   *firstPod2;
  Pod                   *firstPod3;
  Pod                   *firstPod4;
  Pod                   *firstPod5;
  
  bool   alternate = TRUE;
  
  Pod                   *tempPod;
  Pod                   *dustPodList;
  Pod                   *poofPodList;
  bool                  done = FALSE;
  Poof *sortedDustList[MAX_DUST], *poof;
  Poof *sortedPoofList[MAX_POOFS];
  float dist1, dist2;
  int   activeDustPoofs, activePoofs;

#ifdef USE_STENCIL
  TextStencil           *stencil;
  TextStencilInfo       stencilInfo;
#endif

  BlitObject *blitBack = NULL;
  BlitterSnippetHeader *snippets[2];

  
  int32 i = 0;
  int32 j = 0;
  Err					err;
  
  register ushort id = 0;
  register uint32		frameCount = 0;
  TimeVal		startTime, endTime, deltaTime;
  register float		deltaSecs;
  
  PenInfo pen;
  Item font;
  uchar string_buffer[12];
  float FPS = 0.0;
  
  /*  PodTexture* macrotexture; */
  /*  uint32 numPages; */
  
  /* 3DO Change to use blitter folio instead of 2D FrameWork */
  err = OpenGraphicsFolio();
  err = OpenBlitterFolio();
  if (err < 0) 
    {
      printf("Couldn't open Blitter Folio.  Exiting.\n");
      PrintfSysErr(err);
      exit(err);
  }
#ifdef USE_BLITTER_BG
  err  = Blt_LoadUTF(&blitBack, PData[char1].bg);
  if (err < 0) 
    {
      printf("Failed to load the background \"%s\".  Exiting.\n",
	     PData[char0].bg);
      PrintfSysErr(err);
      exit(err);
  }
#endif
  font = OpenFont("default_14");
  printf("Font = 0x%lx\n", font);
  memset(&pen, 0, sizeof(PenInfo));
  pen.pen_XScale = 2.0;
  pen.pen_YScale = 2.0;
  pen.pen_X       = 50;
  pen.pen_Y       = 80;
  pen.pen_FgColor = 0x00ffffff;
  pen.pen_FgW     = 0.999998;
  pen.pen_Flags   = FLAG_PI_W_VALID;
  sprintf(string_buffer, " FPS: %4.1f ", FPS);  
  
  /***	Flame Stuff	***/
  
  memset(FlameArray, 0, sizeof(FlameArray));
  memset(&FlameArray[kXleft], 255, kXright - kXleft + 1);
  
  if(first_time_sprite == 1){
    err = Blt_LoadUTF(&FlamePot0, "Pot0.utf");	if (err<0) printf("Error loading Pot0.utf\n");
    err = Blt_LoadUTF(&FlamePot1, "Pot1.utf");	if (err<0) printf("Error loading Pot1.utf\n");
    err = Blt_LoadUTF(&Flame0, "FlameR.utf");	if (err<0) printf("Error loading FlameR.utf\n");
    err = Blt_LoadUTF(&Flame1, "FlameL.utf");	if (err<0) printf("Error loading FlameL.utf\n");
    
    snippets[0] = &FlamePot0->bo_dbl->dbl_header;
    SetDBlendAttr (snippets,	DBLA_BlendEnable, 0);
    SetDBlendAttr (snippets,	DBLA_DiscardRGB0, 1);
    snippets[0] = &(FlamePot0->bo_tbl->txb_header);
    snippets[1] = &(FlamePot0->bo_pip->ppl_header);
    SetTextureAttr (snippets,	TXA_TextureEnable, 1);
    SetTextureAttr (snippets,	TXA_ColorOut, TX_BlendOutSelectTex);

    snippets[0] = &FlamePot1->bo_dbl->dbl_header;
    SetDBlendAttr (snippets,	DBLA_BlendEnable, 0);
    SetDBlendAttr (snippets,	DBLA_DiscardRGB0, 1);
    snippets[0] = &FlamePot1->bo_tbl->txb_header;
    snippets[1] = &FlamePot1->bo_pip->ppl_header;
    SetTextureAttr (snippets,	TXA_TextureEnable, 1);
    SetTextureAttr (snippets,	TXA_ColorOut, TX_BlendOutSelectTex);

    snippets[0] = &Flame0->bo_dbl->dbl_header;
    SetDBlendAttr (snippets,	DBLA_DiscardAlpha0, 1);

    Flame0->bo_dbl->dbl_userGenCntl |= (FV_DBUSERCONTROL_BLENDEN_MASK |
					 FV_DBUSERCONTROL_SRCEN_MASK);
    Flame0->bo_dbl->dbl_txtMultCntl =        
      CLA_DBAMULTCNTL(RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR,
		      RC_DBAMULTCNTL_AMULTCOEFSELECT_TEXALPHA,
		      0,
		      0);
    Flame0->bo_dbl->dbl_srcMultCntl =        
      CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
		      RC_DBBMULTCNTL_BMULTCOEFSELECT_TEXALPHACOMPLEMENT,
		      0,
		      0);
    Flame0->bo_dbl->dbl_aluCntl =
      CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A_PLUS_BCLAMP, 0);

    snippets[0] = &Flame0->bo_tbl->txb_header;
    snippets[1] = &Flame0->bo_pip->ppl_header;
    SetTextureAttr (snippets,	TXA_TextureEnable, 1);
    SetTextureAttr (snippets,	TXA_ColorOut, TX_BlendOutSelectTex);
    SetTextureAttr (snippets,	TXA_PipAlphaSelect, TX_PipSelectColorTable);
    SetTextureAttr (snippets,	TXA_AlphaOut, TX_BlendOutSelectTex);


    snippets[0] = &Flame1->bo_dbl->dbl_header;
    SetDBlendAttr (snippets,	DBLA_DiscardAlpha0, 1);

    Flame1->bo_dbl->dbl_userGenCntl |= (FV_DBUSERCONTROL_BLENDEN_MASK |
					 FV_DBUSERCONTROL_SRCEN_MASK);
    Flame1->bo_dbl->dbl_txtMultCntl =        
      CLA_DBAMULTCNTL(RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR,
		      RC_DBAMULTCNTL_AMULTCOEFSELECT_TEXALPHA,
		      0,
		      0);
    Flame1->bo_dbl->dbl_srcMultCntl =        
      CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
		      RC_DBBMULTCNTL_BMULTCOEFSELECT_TEXALPHACOMPLEMENT,
		      0,
		      0);
    Flame1->bo_dbl->dbl_aluCntl =
      CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A_PLUS_BCLAMP, 0);

    snippets[0] = &Flame1->bo_tbl->txb_header;
    snippets[1] = &Flame1->bo_pip->ppl_header;
    SetTextureAttr (snippets,	TXA_TextureEnable, 1);
    SetTextureAttr (snippets,	TXA_ColorOut, TX_BlendOutSelectTex);
    SetTextureAttr (snippets,	TXA_PipAlphaSelect, TX_PipSelectColorTable);
    SetTextureAttr (snippets,	TXA_AlphaOut, TX_BlendOutSelectTex);
     
     
    Blt_MoveVertices(FlamePot0->bo_vertices, P2_Pot0.x, P2_Pot0.y);
    Blt_MoveVertices(FlamePot1->bo_vertices, P2_Pot1.x, P2_Pot1.y);
    
    Blt_SetTexture(Flame0, (void *)flame_buffer);
    Blt_SetTexture(Flame1, (void *)flame_buffer);

    bltMat[0][0] = 1.5;
    bltMat[1][1] = 2.0;
    
    Blt_MoveVertices(Flame0->bo_vertices, 0.0, -kMaxY); /*Scale from the bottom left */
    Blt_MoveVertices(Flame1->bo_vertices, 0.0, -kMaxY);

    Blt_TransformVertices(Flame0->bo_vertices, bltMat);
    Blt_TransformVertices(Flame1->bo_vertices, bltMat);
    Blt_MoveVertices(Flame0->bo_vertices, P2_Flame0.x, P2_Flame0.y);
    Blt_MoveVertices(Flame1->bo_vertices, P2_Flame1.x, P2_Flame1.y);
    
    first_time_sprite=0;
  }
  /***	End Flame Stuff	***/	
  
  SampleSystemTimeTV (&startTime);
  
  firstPod  = gBSDF->pods;
  firstPod2 = gBSDF2->pods;
  firstPod3 = gBSDF3->pods;
  firstPod4 = gBSDF4->pods;
  firstPod5 = gBSDF5->pods;


  /* First setup DBlender so that the clear will also clear Z, etc. */
  GS_Reserve(genv->gs, M_DBInit_Size);
  M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
  
  app->curRenderScreen = 0;
  GS_SetDestBuffer(genv->gs, genv->bitmaps[app->curRenderScreen]);
  
  /*	Video_Buffer =	(Bitmap *) LookupItem(GS_GetDestBuffer(genv->gs)); */
  
  app->rx = app->ry = app->rz = 0.0;
  app->tx = app->ty = app->tz = 0.0;	
  
  firstPod3->flags |= nocullFLAG;
  
  firstPod4->flags |= nocullFLAG;
  firstPod4->pnext->flags |= nocullFLAG;
  
  firstPod5->flags |= nocullFLAG;


  /*	 */
  /*	ReadInTextureFile(&firstPod4->ptexture, &numPages, "BLTZTL.page.utf", AllocMem); */
  /*	printf("loadcount = %d \n", firstPod4->ptexture->ptpagesnippets->loadcount); */
  /*		 */
  /*	firstPod4->pnext->ptexture->ptpagesnippets->loadcount = 1; */
  /*	ReadInTextureFile(&firstPod4->pnext->ptexture, &numPages, "BALTSTN4.page.utf", AllocMem); */
  /*	printf("loadcount = %d \n", firstPod4->pnext->ptexture->ptpagesnippets->loadcount); */
  /* */
  
  firstPod5 = M_Sort(1,	firstPod5,	NULL);
  
  firstPod4 = M_Sort(2, firstPod4, NULL);
  firstPod3 = M_Sort(1,	firstPod3,	NULL);
  firstPod  = M_Sort(modelPodCount,	firstPod,	NULL);	
  firstPod2 = M_Sort(modelPodCount2,	firstPod2,	NULL);
  
#ifdef USE_STENCIL
  stencilInfo.tsi_Font = font;
  printf(" =%d, 9=%d, .=%d\n", ' ','9','.');
  stencilInfo.tsi_MinChar = ' '; 
  stencilInfo.tsi_MaxChar = '9';
  stencilInfo.tsi_NumChars = 5;
  stencilInfo.tsi_FgColor = 0x00ffffff;
  stencilInfo.tsi_BgColor = 0x00000000;
  stencilInfo.tsi_reserved = 0x0;
  
  err = CreateTextStencil(&stencil, &stencilInfo);
    if (err<0)
      PrintfSysErr(err);

#endif


#ifdef USE_LOWLATENCY
  /* 3DO change */
  {
    Pod		*fighter1EndPod;
    Pod		*fighter2EndPod;
    Pod		*baseEndPod;
    Pod		*shadowEndPod;
    Pod		*ringEndPod;

#ifdef POD_SHADOW
  printf("Patching pods\n");
  shadowPod0 = shadow0BSDF->pods;
  shadowPod1 = shadow1BSDF->pods;

  /*
   * Add the second shadow POD list to the end of the first
   */
  tempPod = shadowPod0;
  while (tempPod->pnext != NULL)
    tempPod = tempPod->pnext;
#ifdef SINGLE_SHADOW
  tempPod->pnext = shadowPod1;
#endif
#ifdef BLINN_SHADOW
  tempPod->pnext = shadowPod1;
#endif

  printf("Done\n");
#endif /* PodShadow */

    /*
     * Fighter 1
     */
    fighter1EndPod = firstPod;
    while (fighter1EndPod->pnext != NULL)
      fighter1EndPod = fighter1EndPod->pnext;

    /*
     * Fighter 2
     */
    fighter2EndPod = firstPod2;
    while (fighter2EndPod->pnext != NULL)
      fighter2EndPod = fighter2EndPod->pnext;

    /*
     * Ring
     */
    ringEndPod = firstPod3;
    while (ringEndPod->pnext != NULL)
      ringEndPod = ringEndPod->pnext;

    /*
     * Base
     */
    baseEndPod = firstPod4;
    while (baseEndPod->pnext != NULL)
      baseEndPod = baseEndPod->pnext;

    ringEndPod->pnext = firstPod4;
#ifdef RENDER_SHADOW
    shadowEndPod = shadowPods;
    while (shadowEndPod->pnext != NULL)
      shadowEndPod = shadowEndPod->pnext;
    baseEndPod->pnext = shadowPods;
    shadowEndPod->pnext = firstPod;
#else

#ifdef BLINN_SHADOW
    baseEndPod->pnext = shadowPod0;
    shadowEndPod = shadowPod0;
    while (shadowEndPod->pnext != NULL)
      shadowEndPod = shadowEndPod->pnext;
    shadowEndPod->pnext = firstPod;
#else
    baseEndPod->pnext = firstPod;
#endif

#endif /* RENDER_SHADOW */
    fighter1EndPod->pnext = firstPod2;

  }
  /* End Change */
#endif  /* USE_LOWLATENCY */


  SetMove(&Player[0], 0, STAND, 0);
  SetMove(&Player[1], 1, STAND, 0);
  
#ifdef USE_LOWLATENCY
  err = GS_LowLatency(genv->gs, 1, latency);
  if (err<0)
    PrintfSysErr(err);
#endif

  if(first_time_mainloop == 1){
    first_time_mainloop = 0;
  }

#ifdef USE_BLITTER_BG
  Blt_SetVertices(blitBack->bo_vertices, Bg_Vertices);
  err = Blt_BlitObjectToBitmap(genv->gs, blitBack,  genv->bitmaps[app->curRenderScreen], 0);
   if (err < 0) {
    printf("Blit Failed.\n");
    PrintfSysErr(err);
  }
  err = Blt_GetTexture(blitBack, (void **)&Bg_Address);
#endif
    /********************************************************************/
  /******************* MAIN LOOP **************************************/
  /********************************************************************/
  cbvsync(app);
#if 1
  GS_SetView (genv->gs, genv->d->view);
#endif

  do {
    id = 1 - id;

#ifdef USE_LOWLATENCY
    err = GS_LowLatency(genv->gs, 1, latency);
    if (err<0)
      PrintfSysErr(err);
#endif


    /**************************************/
    /* Debugging purpose controller read. */
    /**************************************/
    /*#if 0   */
    GetControlPad (1, FALSE, &app->cped);
    if (app->cped.cped_ButtonBits & ControlX)
      done = TRUE;
    /*#endif */
    ProcessCamera(app);
    cbvsync(app);
    /************************************/
    /* Set the Mercury camera			*/
    /************************************/

    M_SetCamera(app->close, app->matrixCamera, app->matrixSkew);

    DrawBG(blitBack, app);
    err = Blt_BlitObjectToBitmap(genv->gs, blitBack,  genv->bitmaps[app->curRenderScreen], 0);
    if (err < 0) {
      printf("Blit Failed.\n");
      PrintfSysErr(err);
    }

#if 1
  /* First setup DBlender so that the clear will also clear Z, etc. */
  GS_Reserve(genv->gs, M_DBInit_Size);
  M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
#endif

    /************************************/
    /* Clear screen and Z-buffer		*/
    /************************************/
#if 1
    CLT_ClearFrameBuffer(genv->gs, 0.0, 0.0, 0.02, 1.0, 0, TRUE);
#endif
    
    /*		cbvsync(app); */
    /************************************/
    /* Now reset DBlender state, since	*/
    /*	CLT_ClearFB changed its state	*/
    /************************************/
    
    /* ??? */	GS_Reserve(genv->gs, M_DBInit_Size);
    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
    
    /*		cbvsync(app); */
    ProcessInput(id + 1);
    /*		cbvsync(app); */
    PositionObjects(app);
    ProcessHits();
    CheckForHits(Player, &Player[1]);
    CheckForHits(&Player[1], Player);
    
    /************************************/
    /* Draw the model 					*/
    /************************************/
    /* This was the key to the speed up for the backgound picture */
#ifndef USE_LOWLATENCY
   GS_SendList(genv->gs);
#endif

#ifdef USE_LOWLATENCY

    M_Draw(firstPod3, app->close);   /* Draw All the pods in one shot */
#else
    M_Draw(firstPod3, app->close);
    M_Draw(firstPod4, app->close);

#ifdef RENDER_SHADOW
    M_Draw(shadowPods, app->close);
#else
#ifdef BLINN_SHADOW
    M_Draw(shadowPod0, app->close);
#endif
#endif /*RENDER_SHADOW */

    M_Draw(firstPod2, app->close);

    GS_SendList(genv->gs);

  /* Print Out Debug Information */

    M_Draw(firstPod, app->close);

    GS_SendList(genv->gs);

#endif  /* USE_LOWLATENCY */


    /************************************/
    /*	Flame routine for the life bar	*/	
    /************************************/	
    if (alternate)
      {
	CalcFire(Player->HPleft, Player->HitPoints, Player[1].HPleft, Player[1].HitPoints);
      }
    alternate = !alternate;


    if(Player[0].HPleft > 0) {
      err = Blt_BlitObjectToBitmap(genv->gs, Flame0, genv->bitmaps[app->curRenderScreen], 0);
      if (err < 0) {
	printf("Flame0 Blit Failed.\n");
	PrintfSysErr(err);
      }
    }

    if(Player[1].HPleft > 0){
      err = Blt_BlitObjectToBitmap(genv->gs, Flame1, genv->bitmaps[app->curRenderScreen], 0);
      if (err < 0) {
	printf("Flame1 Blit Failed.\n");
	PrintfSysErr(err);
      }
    }

    err = Blt_BlitObjectToBitmap(genv->gs, FlamePot0, genv->bitmaps[app->curRenderScreen], 0);
    if (err < 0) {
      printf("FlamePot0 Blit Failed.\n");
      PrintfSysErr(err);
    }

    err = Blt_BlitObjectToBitmap(genv->gs, FlamePot1, genv->bitmaps[app->curRenderScreen], 0);
    if (err < 0) {
      printf("FlamePot1 Blit Failed.\n");
      PrintfSysErr(err);
    }
    pen.pen_X       = 50;
    pen.pen_Y       = 80;

#ifdef USE_STENCIL
    sprintf(string_buffer, "%4.1f", FPS);
    err = DrawTextStencil(genv->gs, stencil, &pen, string_buffer, 4);
    if (err<0)
      PrintfSysErr(err);
#else
    sprintf(string_buffer, " FPS: %4.1f ", FPS);
    err = DrawString(genv->gs, font, &pen, string_buffer, 12);	
    if (err<0)
      PrintfSysErr(err);
#endif


    /************************************/
    /* Once it's done being rendered,	*/
    /* show the bitmap 					*/
    /************************************/
    /* ??? */


    cbvsync(app);
    
    /*    WaitForAnyButton(&(app->cped)); */
    CLT_Sync(GS_Ptr(genv->gs));

#if 1
    /* First setup DBlender so that the clear will also clear Z, etc. */
    GS_Reserve(genv->gs, M_DBInit_Size);
    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
#endif

    DrawPoofs(id, app, genv->gs);

    /************************************/
    /* Here's where the cmd list is		*/
    /*	sent to the Triangle Engine		*/
    /************************************/

    activeDustPoofs=0;
    for (i=0, poof = dustPoofs; i<MAX_DUST; i++, poof++)
      {
	if (poof->framenum > 0)
	  {
	    sortedDustList[activeDustPoofs] = poof; 
	    dist2 = CameraPosition.z - poof->pos.z;
	    dist2 *= dist2;
	    dist1 = CameraPosition.x - poof->pos.x;
	    dist1 *= dist1;
	    dist1 += dist2;
	    poof->distance = dist1;
	    poof->podIndex = i;
	    activeDustPoofs++;
	  }
      }
    if (activeDustPoofs >0)
      {
	qsort(sortedDustList, activeDustPoofs, sizeof(Poof *), 
	      (int (*)(const void *, const void *))poofdist);
	
	tempPod = dustPodList = NULL;
	for (i=0; i<activeDustPoofs; i++)
	  {
	    if (dustPodList == NULL)
	      {
		tempPod = dustPodList = &dustPods[sortedDustList[i]->podIndex];
	      }
	    else
	      {
		tempPod->pnext = &dustPods[sortedDustList[i]->podIndex];
		tempPod = tempPod->pnext;
	    }
	  }
    
	if (dustPodList != NULL)
	  {
	
	    /* First setup DBlender so that the clear will also clear Z, etc. */
	    GS_Reserve(genv->gs, M_DBInit_Size+16);
	    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
	    
	    tempPod->pnext = NULL;

	    M_Draw(dustPodList, app->close);

#ifndef USE_LOWLATENCY
	    GS_SendList(genv->gs);
#endif
	    /*    WaitForAnyButton(&(app->cped)); */
	    CLT_Sync(GS_Ptr(genv->gs));
	    CLT_DBUSERCONTROL(GS_Ptr(genv->gs), 1, 1, 0, 0, 0, 1, 0, 15); /* Turn OFF dblend */
	  }
      }

    activePoofs=0;
    for (i=0, poof = poofs; i<MAX_POOFS; i++, poof++)
      {
	if (poof->framenum > 0)
	  {
	    sortedPoofList[activePoofs] = poof; 
	    dist2 = CameraPosition.z - poof->pos.z;
	    dist2 *= dist2;
	    dist1 = CameraPosition.x - poof->pos.x;
	    dist1 *= dist1;
	    dist1 += dist2;
	    poof->distance = dist1;
	    poof->podIndex = i;
	    activePoofs++;
	  }
      }
    
    if (activePoofs >0)
      {
	qsort(sortedPoofList, activePoofs, sizeof(Poof *), 
	      (int (*)(const void *, const void *))poofdist);
      
	tempPod = poofPodList = NULL;
	for (i=0; i<activePoofs; i++)
	  {
	    if (poofPodList == NULL)
	      {
		tempPod = poofPodList = &poofPods[sortedPoofList[i]->podIndex];
	      }
	    else
	      {
		tempPod->pnext = &poofPods[sortedPoofList[i]->podIndex];
		tempPod = tempPod->pnext;
	      }
	  }
    
	if (poofPodList != NULL)
	  {
	    /* First setup DBlender so that the clear will also clear Z, etc. */
	    GS_Reserve(genv->gs, M_DBInit_Size+16);
	    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
	    
	    tempPod->pnext = NULL;
	    
	    M_Draw(poofPodList, app->close);	    
	    
#ifndef USE_LOWLATENCY
	    GS_SendList(genv->gs);
#endif
	  }
      }

    /*    WaitForAnyButton(&(app->cped)); */
    CLT_Sync(GS_Ptr(genv->gs));
    CLT_DBUSERCONTROL(GS_Ptr(genv->gs), 1, 1, 0, 0, 0, 1, 0, 15); /* Turn OFF dblend */
    
#ifdef USE_MP
    M_DrawEnd(app->close);
#endif

#ifdef USE_LOWLATENCY
    GS_SendLastList(genv->gs);
#endif

#if 0
    GS_WaitIO(genv->gs);
    ModifyGraphicsItemVA(genv->d->view,
				VIEWTAG_BITMAP, 
				GS_GetDestBuffer(genv->gs),
				TAG_END);
#else
    GS_EndFrame(genv->gs);
#endif
    /********************************/
    /*	Switch the Double Buffer    */
    /********************************/
    /*
      WaitForAnyButton(&(app->cped));
      */

#ifdef DRAW_SHADOWS
#ifdef FAKE_SHADOW
    ProcessShadows();
#else
#ifdef BLINN_SHADOW
    ProcessShadows();
#else
    ProcessShadows(shadowEnv0, shadowData0, shadowPods, shadowPod0, &(Player[0]), 0.1);
#ifndef SINGLE_SHADOW
    ProcessShadows(shadowEnv1, shadowData1, &(shadowPods[SHADOW_SEG]),shadowPod1,
		   &(Player[1]), 0.5);
#endif
#endif /* BLINN_SHADOW */
    M_DBNoBlend(GS_Ptr(genv->gs));
#endif /* FAKE_SHADOW */
#endif /* DRAW_SHADOWS */


    GS_BeginFrame(genv->gs);

    app->curRenderScreen = 1 - app->curRenderScreen;
    
    GS_SetDestBuffer(genv->gs, genv->bitmaps[app->curRenderScreen]);
    CLT_SetSrcToCurrentDest(genv->gs);
#if 0
    Video_Buffer =	(Bitmap *) LookupItem(GS_GetDestBuffer(genv->gs));
    printf("Video_Buffer->bm_Buffer = %p\n", Video_Buffer->bm_Buffer);
#endif
    
    if(frameCount == 10){
      SampleSystemTimeTV (&endTime);
      SubTimes (&startTime, &endTime, &deltaTime);
      deltaSecs = ((gfloat) (deltaTime.tv_usec + deltaTime.tv_sec * 1000000)) / 1000000.0;			
      /*			printf ("( %4.1f FPS ^.^ )\n", ((float) frameCount) / deltaSecs);	*/
      if (deltaSecs!=0.0)
	FPS = ((float) frameCount) / deltaSecs;
      frameCount = 0;
      SampleSystemTimeTV (&startTime);	
    }		
    frameCount++;
  } while (!done);
  /****************/
  /*End of do loop*/
  /****************/
  M_End(app->close);	

#ifdef POD_SHADOW
#ifndef BLINN_SHADOW
  M_End(shadowData0->close);
#ifndef SINGLE_SHADOW
  M_End(shadowData1->close);
#endif
#endif
#endif

  /*	FreeMem(BG_Sprite, sizeof(SpriteObj)); */
  
  for(i=0; i<2; i++){
    for(j=0; j<20; j++){
      FreeMem(Char_Obj[i][j], sizeof(Matrix));
      Char_Obj[i][j] = NULL;
    }
  }
  FreeMem(gBSDF, sizeof(BSDF));
  FreeMem(gBSDF2, sizeof(BSDF));
  FreeMem(gBSDF3, sizeof(BSDF));
  FreeMem(gBSDF4, sizeof(BSDF));
  gBSDF = NULL;
  gBSDF2 = NULL;
  gBSDF3 = NULL;
  gBSDF4 = NULL;
  done = FALSE;
}

/* --------------------------------------------------------------------- */


#define kNumCmdListBufs     2
#define kCmdListBufSize     64*1024	/* 150000 words = 600k per list */

int main( int argc, char **argv )
{
  Err            err;
  GraphicsEnv*   genv;
  AppData*       appData;
  int loop = TRUE;
  int first_time = 1;
  char **arv = argv;
  char *cp;
  uint32 ac = argc;


  while(loop){
    /************************************/
    /* Print usage						*/
    /************************************/
    printf("Sample program by Satoru HOSOGAI\n");
    

    if (argc >1)
      char0 = atoi(argv[1]);
    if (argc >2)
      char1 = atoi(argv[2]);
    if (argc >3)
      Options.Level = atoi(argv[3]);

    if(first_time == 1){
      /************************************/
      /* Setup Display, GState			*/
      /************************************/
      genv = GraphicsEnv_Create();

      if (genv == NULL) {
	exit(1);
      }
      /************************************/
      /* "Default" environment			*/
      /************************************/
      err = GraphicsEnv_Init(genv, IB_SCREENWIDTH, IB_SCREENHEIGHT, IB_SCREENDEPTH);
      if (err < 0) {
	printf("Couldn't initialize graphics environment.  Exiting.\n");
	PrintfSysErr(err);
	exit(err);
      }

#ifdef POD_SHADOW

#ifndef BLINN_SHADOW
ShadowEnv_Create(&shadowEnv0);
#ifndef SINGLE_SHADOW
ShadowEnv_Create(&shadowEnv1);
#endif
#endif

#endif
      
      /************************************/
      /* Setup the EventBroker			*/
      /************************************/
      err = InitEventUtility(1, 0, 0);
      if (err < 0) {
	printf("Couldn't initialize EventUtility.  Exiting.\n");
	PrintfSysErr(err);
	exit(err);
      }
      
      
      first_time = 0;
    }
    /************************************/
    /* Load & initialize data, then		*/
    /*	begin							*/
    /************************************/
    Program_Init(genv, &appData);
    
    
    LoadPlatform(char1);
    InitPlayers();
    mainloop(genv, appData);
    
    
    /************************************/
    /* Cleanup							*/
    /************************************/
    printf("Program exiting.\n");
#if 0  	
    GS_FreeBitmaps(genv->bitmaps, genv->d->numScreens + 1);
    printf("GS_FreeBitmaps\n");
    FreeSignal(genv->d->signal);
    printf("FreeSignal\n");
    Display_Delete(genv->d);
    printf("Display_Delete\n");
    GS_Delete(genv->gs);
    printf("GS_Delete\n");
    FreeMem(genv, sizeof(GraphicsEnv));
    printf("Free genv\n");
    FreeMem(appData, sizeof(AppData));
    printf("FreeMem\n");
#endif		
    loop = FALSE;
    
  }
  return 0;
}
