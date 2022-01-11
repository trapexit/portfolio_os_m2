#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <graphics/fw.h> */
/* #include <fw.h> */
#include "kerneltypes.h"

#ifndef Basic_INC
#define Basic_INC 1

/* Basic Primitive types */
typedef char Int8;
typedef unsigned char UInt8;
typedef short Int16;
typedef unsigned short UInt16;
typedef long Int32;
typedef unsigned long UInt32;
typedef unsigned char Byte;

typedef struct Point2D 
{
	float x, y;
} Point2D;

typedef struct Point3D 
{
	float x, y, z;
} Point3D;

typedef struct 
{
	float x, y, z, ang;
} AxisRot3D;

typedef struct Vector3D 
{
	float x, y, z;
} Vector3D;

typedef struct Color 
{
	float r, g, b;
} Color;

typedef struct UVPoint 
{
	float u, v;
} UVPoint;

#define MemAlloc( size )  malloc( size )

/* error codes */
#define	error_None		0	                        
#define	error_OutOfMemory	-1                      
#define	error_OutOfBounds	-2	
	

#endif /* Basic_INC */

