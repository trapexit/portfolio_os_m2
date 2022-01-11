/****
 *
 * @(#) gfxtypes.h 95/08/21 1.49
 * Copyright 1994, The 3DO Company
 *
 * Basic types and constants for 3D libraries
 *
 ****/
#ifndef _GPTYPES
#define _GPTYPES

#ifndef EXTERNAL_RELEASE
#ifdef NUPUPSIM
#include <gpNupup.h>
#endif
#else
#include <gpNupup.h>
#endif

#ifndef EXTERNAL_RELEASE
#ifdef macintosh
#include "gpMac.h"
#endif

#ifdef UNIXSIM
#include <gpUnix.h>
#endif
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef NULL
#define NULL	0
#endif

#ifndef PI
#define PI 3.141592653589793
#endif

/***
 *
 * C Pipeline Objects 
 *
 ***/
#define Point3 Vector3

typedef struct Vector3 Vector3;
typedef struct Point4 Point4;
typedef struct Box3 Box3;
typedef struct Color4 { float r, g, b, a; } Color4;
typedef struct Geometry Geometry;
typedef struct QuadMesh QuadMesh;
typedef struct GfxObj GfxObj;
typedef struct GfxObj GP;
typedef struct GfxObj Surface;

#define	Col_SetRGB(c, R, G, B) 						\
	((c)->r = ((float)R), (c)->g = ((float)G),	\
	 (c)->b = ((float)B), (c)->a = 1.0)


#endif /* GPTYPES */
