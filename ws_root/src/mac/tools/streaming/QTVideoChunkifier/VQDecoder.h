/******************************************************************************
**
**  @(#) VQDecoder.h 95/05/26 1.1
**
******************************************************************************/
/**
	File:		VQDecoder.h

	Contains:	High level decoder routine.

******************************************************************************/
#ifndef makeformac
#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "lists.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "graphics.h"
#include "3doHelper.h"
#define	ENTERMAIN int main(int argc, char *argv[])
#define printf kprintf
#endif

#ifdef makeformac
#include	<Types.h>
#include 	<ctype.h>
#include 	<fcntl.h>
#include 	<string.h>
#include	<stdLib.h>
#include 	<stdio.h>
#include	<ErrMgr.h>
#include	<Strings.h>
#include	<Errors.h>
#include	<Memory.h>
#include	<QuickDraw.h>
#include	<Movies.h>
//@@@@@#include 	"MacHelper.h"	// Includes 3DO routines an definitions
#define	ENTERMAIN main(int argc, char *argv[])
#endif





enum {
	kKeyType			= 0x00000000,
	kInterType			= 0x01000000,

	kFrameType			= 0x00000000,
	kKeyFrameType		= kKeyType + kFrameType,
	kInterFrameType		= kInterType + kFrameType,

	kTileType			= 0x10000000,
	kTileKeyType		= kKeyType + kTileType,
	kTileInterType		= kInterType + kTileType,

	kCodeBookType		= 0x20000000,
	kFullDBookType		= 0x00000000 + kCodeBookType,
	kPartialDBookType 	= 0x01000000 + kCodeBookType,
	kFullSBookType		= 0x02000000 + kCodeBookType,
	kPartialSBookType 	= 0x03000000 + kCodeBookType,

	kCodesType			= 0x30000000,
	kIntraCodesType		= 0x00000000 + kCodesType,
	kInterCodesType		= 0x01000000 + kCodesType,
	kAllSmoothCodesType	= 0x02000000 + kCodesType,



	kKeyType8			= 0x00,
	kInterType8			= 0x01,

	kFrameType8			= 0x00,
	kKeyFrameType8		= kKeyType8 + kFrameType,
	kInterFrameType8	= kInterType8 + kFrameType8,

	kTileType8			= 0x10,
	kTileKeyType8		= kKeyType8 + kTileType8,
	kTileInterType8		= kInterType8 + kTileType8,

	kCodeBookType8		= 0x20,
	kFullDBookType8		= 0x00 + kCodeBookType8,
	kPartialDBookType8 	= 0x01 + kCodeBookType8,
	kFullSBookType8		= 0x02 + kCodeBookType8,
	kPartialSBookType8 	= 0x03 + kCodeBookType8,

	kCodesType8			= 0x30,
	kIntraCodesType8	= 0x00 + kCodesType8,
	kInterCodesType8	= 0x01 + kCodesType8,
	kAllSmoothCodesType8 = 0x02 + kCodesType8,
	kFrameAlign8 		= 0xFE,
	kFrameNumberType8 	= 0xFF
};

typedef struct {
	long	frameSize;
	short	frameWidth;
	short	frameHeight;
	short	frameTileCount;
} FrameHeader, *FrameHeaderPtr;

typedef struct {
	long	tileSize;
	long	tileRectTopLeft;
	long	tileRectBotRight;
} TileHeader, *TileHeaderPtr;
