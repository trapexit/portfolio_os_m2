#ifndef Spline_INC
#define Spline_INC 1

#include "kf_types.h"

/* spline control values, at "i", for a single animation key */
typedef struct
{
	float	mTension;		/* relative tangent magnitude */
	float	mBias;			/* relative tangent direction  */
	float	mContinuity;	/* tangential continuiety at this key */
	float	mEaseIn;		/* relative approching velocity */
	float	mEaseOut;		/* relative leaving velocity */
} SplineProps;

/* time vector for the whole path, size =  mNumSegs + 1 */
#define GetTimeVector( spl )	( spl->mData )
/* data to be interpolated, size = mNumSegs * mNumAttribs * 4  */
#define GetAttData( spl )	( spl->mData + ( spl->mNumSegs + 1 ) )
	
/* cubic spline path data structure */
typedef struct 
{
	UInt32  refCount;
	UInt32	mNumAttribs;	/* number of float attributes to be */
	                        /* interpolated - eg : position = 3 */
	UInt32	mNumSegs;		/* number of path segments */
	                        /* if number of segments = 0 - */
	                        /* means there is only one key */
	float	mData[1];		/* time vector and attribute data */
} KfSpline;

/* function prototyping */
KfSpline*
Spl_Create( 
	Int32			inNumKeys, 
	Int32			inNumAtts, 	 
	float			*inTimeKeys, 
	float			*inAttKeys,
	SplineProps		*inCntrlKeys
	);
KfSpline*
Spl_CreatePrivate( 
	Int32			inNumKeys, 
	Int32			inNumAtts, 	 
	float			*inTimeKeys, 
	float			*inAttKeys, 
	SplineProps		*inCntrlKeys
	);
KfSpline*
Spl_CreateOneKey( 
	Int32			inNumAtts, 	 
	float			*inTimeKeys, 
	float			*inAttKeys 
	);
void
Spl_CreateTangents(
	KfSpline			*inSpl,
	SplineProps		*inCntrlKeys,
	float			*inAttKeys,
	float			*outSrcTngt,
	float			*outDstTngt
	);
void
Spl_ComputeCoefficients( 
	KfSpline			*inSpl,
	SplineProps		*inCntrlKeys,
	float			*inAttKeys 
	);
void
Spl_Print( 
	KfSpline			*inSpl,
	UInt32				nAtts
	);
void
Spl_Delete(
	KfSpline		*inSpline
	);
void
Spl_IncUsage(
	KfSpline		*inSpline
	);
Int32
Spl_FindSegIndex(
	KfSpline			*inSpl,
	float			*inParm,
	UInt32			*outSegIndx
	);
Int32
Spl_EvaluateAt( 
	KfSpline		*inSpline, 
	float		inAtTime, 
	float		*outAttribs 
	);

/* default spline values */
#define	default_SplineTension		0.0                   
#define	default_SplineBias	    	0.0                      
#define	default_SplineContinuity	0.0                      
#define	default_SplineEaseIn		0.0                      
#define	default_SplineEaseOut	    0.0                      
                        
#endif /* Spline_INC */

