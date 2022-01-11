#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream.h>

#ifndef BASIC_TYPES
#define BASIC_TYPES

#define PRINT_ERROR( cond, str ) \
  { \
    if( cond ) { \
     fprintf( stderr, "ERROR: %s\n", str ); \
     exit( 0 ); \
    } \
  } 
  
// Include Slice&Dice code
//#define SPLIT_TG
// Debug flag
//#define DEBUG
#ifdef DEBUG
	extern int VtxList_numObjs;
	extern int FacetList_numObjs;
	extern int Material_numObjs;
	extern int Texture_numObjs;
	extern int Model_numObjs;
#endif

// smallest value
#define EPSILON 1.0e-4
// approximate equal
#define AEQUAL( a, b ) ( fabs( (double)(a - b) ) < EPSILON )

// Basic Primitive types
typedef char Int8;
typedef unsigned char UInt8;
typedef short Int16;
typedef unsigned short UInt16;
typedef long Int32;
typedef unsigned long UInt32;
typedef unsigned char Byte;

typedef unsigned short ErrCode;
enum  { 
	noError, 
	ioError,
	vtxStyleError,
	unknownError 
};

#ifndef BOOLEAN // comes from Mac include files <Reddy 3-16-95> 
typedef unsigned char Boolean;
#if !defined(TRUE)
enum  { FALSE, TRUE };
#endif
#endif

// To treat enum types as bit fields
#define BITFIELD(E, T) typedef int T

// Used for entity use count and temporary indexing
typedef struct ObjUse 
{
	UInt16 refCount : 16; // Obj reference count
	Int16 objID     : 16; // Obj index ID
} ObjUse;

// Used for permanent dynamic pointer or temporary
// static array index
typedef union 
{
	void *ptr;
	Int32 indx;
} IPtr;

typedef struct Point2D 
{
	double x, y;
} Point2D;

typedef struct Point2DS 
{
	float x, y;
} Point2DS;

typedef struct Point3DS
{
       float x, y, z;
} Point3DS;

typedef struct Point3D 
{
	double x, y, z;
} Point3D;

typedef struct Vector3D 
{
	double x, y, z;
} Vector3D;

typedef struct TlColor 
{
	double r, g, b;
} TlColor;

typedef struct UVPointDS 
{
	float u, v;
} UVPointDS;

typedef struct UVPoint 
{
	double u, v;
} UVPoint;

// Structure to conveniently access the vertex data
typedef struct VertexPtr
{
	Int32 indx;
        Int32 dummy;
	Point3D pnt;
	void *data;
} VertexPtr;

//typedef double TlTransform[4][4];
typedef double Matrix4x4[4][4];

// Local memory allocator - exists only in its context
class MemPtr
{
  public:	
	void *mPtr;
	MemPtr( size_t numBytes = 0 );
	virtual ~MemPtr();
}; 
	
// Object Types
#define	CHAR_TYPE		1
#define	GROUP_TYPE		2
#define	MODEL_TYPE		3
#define	CAMERA_TYPE		4
#define	LIGHT_TYPE		5
#define	LOD_TYPE		6
#define	POV_TYPE		7
#define	BB_TYPE			8
#define	SCENE_TYPE		9

// function prototypes
void WRITE_SDF( ostream& os, char *sdump );														
void BEGIN_SDF( ostream& os, char *sdump );										
void END_SDF( ostream& os, char *sdump );										
const char *ChopString( char *in, char ch );
const char *ChopStringLast( char *in, char ch );
const char *LowerCase( char *in );

#endif // BASIC_TYPES 

