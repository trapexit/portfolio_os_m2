#ifndef _H_FIGHT
#define _H_FIGHT

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <graphics:graphics.h>
#include <graphics:blitter.h>
#include <graphics:view.h>
#include <graphics:clt:gstate.h>
#include <misc:event.h>
#include <:graphics:frame2d:f2d.h>
#include <:graphics:font.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/blitter.h>
#include <graphics/view.h>
#include <graphics/clt/gstate.h>
#include <misc/event.h>
#include <graphics/frame2d/f2d.h>
#include <graphics/font.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mercury.h"
#include "matrix.h"

#include "sounds.h"
#if 0
#include "play-s3m.h"
#include "sound-m2.h"
#endif

#include "misc.h"
#include "controller.h"
#include "graphicsenv.h"
#include "bsdf_read.h"
#include "filepod.h"
#include "particles.h"


#ifndef NULL
    #define NULL ((void *)0)
#endif

#ifndef uchar
    #define uchar unsigned char
#endif

#ifndef ushort
	#define ushort unsigned short
#endif

#ifndef ulong
	#define ulong unsigned long
#endif

#define	Vector3	Vector3D
#define	SVECTOR	Vector3D
#define	MATRIX	Matrix

#define MAX_POLY    7500

#define OTSIZE      4736
#define OTMASK (OTSIZE-1)
#define SCR_Z       0

/*
 * How many moves per character
 */
#define NUMOFMOVES 56
#define MOVEKEYS 11

/* total number of ObjAnim frames stored in system */
/* = 56 anims/char @ 18 frames/anim with 35 objects (char 1 = 18, char 2 = 17) */
#define NUMOFANIMFRAMES 35280

/* total number of xyz character position frames (56 x 35 x 2 bytes per short) */
#define NUMOFPOSFRAMES 2240

/* move number for the standing animation (see moves.txt) */
#define STAND 24

/* move number for the first react animation (see moves.txt) */
#define REACT 34

/* how many vblanks the system will wait before cancelling a move */
#define KEYDELAY 20


typedef struct {
	ulong    SquarePlat;
	ulong    MaxDistance;
/*	ulong    ot; */
	ulong    PolyCount;
/*	PlatPoly Polys[806]; */
} PlatformStruct;


typedef struct {
	short rotx;
	short roty;
	short rotz;
	short tranx;
	short trany;
	short tranz;
} ObjAnim;

typedef struct {
                ushort   LoopStart;
                ushort   LoopEnd;
                short    LoopCount;
                uchar    NumFrames;
                uchar    Sound;    /* sound # (array lookup) */
                uchar    SFrame1;  /* first frame to start sound on */
                uchar    SFrame2;  /* second frame to start sound on */
                uchar    SndProb;  /* probability (1-8) of playing sound 1 */
                uchar    Sound2;   /* sound to play instead */
                uchar    Damage;
                uchar    Knockdown;
                uchar    KicksUpDust;
                uchar    Bleeds;
				signed char    AttackObjNum[4];
                ulong    OptimalDistance;
                short   *PosOffsetPtr; /* offset into PosOffset array */
                ObjAnim *AnimDataPtr; /* offset into AnimData array */
} AnimDetail;

typedef struct {
	ushort      NumOfPolys;
	ushort      NumOfShadowPolys;
	ushort      StartAxis; /* for attacks */
	ushort      HitDetected;
	AnimDetail *HitAnim;
	SVECTOR     HitLoc;    /* point in 3-space where this obj was hit */
	SVECTOR     Base[3];
	SVECTOR     Tip[3];
	SVECTOR     CurrBase[3];
	SVECTOR     CurrTip[3];
	SVECTOR     LastBase[3];
	SVECTOR     LastTip[3];
} ObjDetail;

typedef struct {
	short       *posoffset;
	ObjAnim     *animdata;
	uchar        MoveNum;
	uchar        FrameNum;
	ushort       LoopStart;  /* also char AttackWinStart1, End1 */
	ushort       LoopEnd;    /* also char AttackWinStart2, End2 */
	short        LoopCounter;
	short        SpinAmount;
	ushort       lastpadd;
	ushort       vbcount;
	ushort       firstpoly;
	ushort       HitPoints;
	short        HPleft;
	uchar        NextMove;
	uchar        NumOfTMDs;
	uchar        FacingRight;
	uchar        OpponentIsInFront;
	uchar        Weapon1;
	uchar        Weapon2;
	uchar        LawChar;
	uchar        Damage;
	uchar        RepeatDamage;
	char         RDamageCtr;
	uchar        Knockdown;
	char         HitRune;
	uchar        KicksUpDust;
	uchar        Armor;
	uchar        Human;
	char         filler;
	ulong        CloseToRune;
	ushort       AttackDelay;
	short        CalcedMove;
	SVECTOR      position;
	MATRIX       *orientation;
	short        AttackObj[4];
	ObjDetail    *Obj;
	ushort       *Keys;
	uchar        AttackProb[24];
} Ptcl;

typedef struct {
	ushort movesize;
	ushort movematch;
	ushort movetable[8];
} PlayerMove;

typedef struct {
	Point3	vtx_data[4];
	TexCoord tc_data[4];
	int32	color;
}POLY_FT4;

typedef struct {
	ushort   framenum;
	ushort   pooftype;
	POLY_FT4 sprite[1];
	SVECTOR  pos;
	SVECTOR  vec1;
	SVECTOR  vec2;
	SVECTOR  vec3;
	SVECTOR  vec4;
	Matrix   mat;/*	MATRIX   mat; */
        signed int   alpha;
        ushort   dAlpha;
        float    distance;
        int      podIndex;
} Poof;

typedef struct {
	uchar TimerType;
	uchar GameType;       /* campaign, fight or training ground */
	uchar PlayersOnTeam;  /* number of players allowed on teams */
	uchar Level;
} OptionsRec;

/* Screen Characteristics */

#define IB_SCREENWIDTH  640L
#define IB_SCREENHEIGHT 480L
#define IB_SCREENDEPTH  32L

/********************************************************************/
/* data.c */

/*  DoubleBuffer db1, db2; /* current double buffer */
extern Ptcl		Player[2];
extern ObjDetail	Objects[36];
extern PlayerMove	PMoves[2][MOVEKEYS];
extern AnimDetail	AnimDetails[2][NUMOFMOVES]; /* header data for all moves */
extern short		PosOffset[NUMOFPOSFRAMES * 3]; /* position data for all moves */
extern ObjAnim		AnimData[NUMOFANIMFRAMES]; /* animation data for all moves */

extern PlatformStruct	Platform;

extern OptionsRec	Options;
/*#define Rand() (RandSeed = (RandSeed * 0x343FD + 0x269EC3) >> 16) */

/* following variables are on the scratchpad */
/*	MATRIX	unitmat	= {ONE, 0, 0, 0, ONE, 0, 0, 0, ONE, 0, 0, 0}; */
extern SVECTOR	zerovec;
extern SVECTOR	CameraPosition;
/*MATRIX  CameraOrientation	= {0}; */
/*MATRIX  world 			= {0}; */
/*DoubleBuffer *CurrDB		= 0; */
extern ushort  nextPoof;
/*short   RuneOffset			= 0; */
extern int32	RuneOffset;
extern int32	rune;
extern short	CombatTimer;
extern short   GameInProgress;
/*ulong	RandSeed			= 0; */
extern ulong	PlayerSep;
extern uchar   FlameArray[28];
/* make sure this  ^^  is kMaxX + 1, from MainLoop.c */
/* End of data.c ****************************************************/
/********************************************************************/
/* 3d.c */

static copyVector(Vector3 *VecOut, Vector3 *VecIn)
{
	VecOut->x = VecIn->x;
	VecOut->y = VecIn->y;
	VecOut->z = VecIn->z;
}


/* End of 3d.c ******************************************************/

/* Controller event bits */

#define padForward   (1 << 10)
#define padBackward  (1 << 9)

#define padLup       (1 << 12)
#define padLdown     (1 << 14)
#define padLleft     (1 << 15)
#define padLright    (1 << 13)
#define padLdul      padLup | padLleft
#define padLddl      padLleft | padLdown
#define padLddr      padLright | padLdown
#define padLdur      padLup | padLright
#define padLtop      (1 << 2)
#define padLbottom   (1 << 0)

#define padRup       (1 << 4)
#define padRdown     (1 << 6)
#define padRleft     (1 << 7)
#define padRright    (1 << 5)
#define padRtop      (1 << 3)
#define padRbottom   (1 << 1)

#define padTriangle  (1 << 4)
#define padX         (1 << 6)
#define padSquare    (1 << 7)
#define padCircle    (1 << 5)

#define padSelect    (1 << 8)
#define padStart     (1 << 11)

/* these two are artificial -- determined at run time */
#if 0
#define padForward		0x0002
#define padBackward		0x0001

#define padLup			0x4000
#define padLdown		0x8000
#define padLleft		0x1000
#define padLright		0x2000
#define padLdul      (padLup | padLleft)
#define padLddl      (padLleft | padLdown)
#define padLddr      (padLright | padLdown)
#define padLdur      (padLup | padLright)
#define padLtop      	0x0004
#define padLbottom		0x0200

#define padRup			0x0008
#define padRdown		0x0800
#define padRleft		0x0010
#define padRright		0x0400
#define padRtop			0x0020
#define padRbottom		0x0040

#define padTriangle		0x0008
#define padX			0x0800
#define padSquare		0x0010
#define padCircle		0x0400

#define padSelect		0x0080
#define padStart		0x0100


#define ControlDown          0x80000000
#define ControlUp            0x40000000
#define ControlRight         0x20000000
#define ControlLeft          0x10000000
#define ControlA             0x08000000
#define ControlB             0x04000000
#define ControlC             0x02000000
#define ControlStart         0x01000000
#define ControlX             0x00800000
#define ControlRightShift    0x00400000
#define ControlLeftShift     0x00200000

/*****
 The following bits are present in the M2 extended control pad, when used
 on an M2 system.  They are not present in data sent from an Opera control
 pad, or from an M2 control pad which is connected to an Opera console.
*****/

#define ControlD             0x00100000
#define ControlE             0x00080000
#define ControlF             0x00040000

#endif

#define	REGULAR 	((50 << 16) + (150 << 8) + 230)
#define	BACKGROUND 	((0 << 16) + (0 << 8) + 5)
#define FONT		( "Geneva.36pt" )

/*mainloop.c */
/*static ulong ThreeDOtoSONY_Controler(uint32 padd); */

#endif









