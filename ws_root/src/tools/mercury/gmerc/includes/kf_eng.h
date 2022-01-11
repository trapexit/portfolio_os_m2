#ifndef KfData_INC
#define KfData_INC 1

#include "kf_types.h"
#include "fw.h"
#include "kf_spline.h"

#define KF_NAME_LEN	32


typedef struct
{
	Array		*mFrames;		/* floatarray */
	Array		*mPnts;			/* pointarray */
	Array		*mSplData;		/* splinearray */
} KfPosTrack;

typedef struct
{
	Array		*mFrames;		/* floatarray */
	Array		*mRots;			/* axisrotarray */
	Array		*mSplData;		/* splinearray */
} KfRotTrack;

typedef struct
{
	Array		*mFrames;		/* floatarray */
	Array		*mScls;			/* pointarray */
	Array		*mSplData;		/* splinearray */
} KfScaleTrack;

typedef struct
{
	Array		*mFrames;		/* floatarray */
	Array		*mData;			/* floatarray */
	Array		*mSplData;		/* splinearray */
} KfTrack;
	
typedef struct EngineData
   {
	GfxObj	m_Obj;
	int16	m_Status;
	int16	m_Control;
	float	m_StartTime;
	float	m_Speed;
	float	m_Duration;
	float	m_Elapsed;
   } EngineData;

/* Structure that will be read from SDF file */
typedef struct KfEngine
{
	EngineData		mEngine;	/* Framework Engine is base class */
	struct CharData*		mObj;
	Point3			mObjPivot;	/* object pivot point */
	Point3			mPobjPivot;	/* object's parents pivot point */
	KfPosTrack		mObjPos;	/* object position keys */
	KfRotTrack		mObjRot;	/* object rotation keys */
	KfScaleTrack	mObjScl;	/* object scale keys */
	KfSpline		*mSplPos;
	KfSpline		*mSplRot;
	KfSpline		*mSplScl;
	float			startFrame;
	float			endFrame;
} KfEngine;
 

/*
 * Makes the spline paths for every KfEngine in the given array
 */
void KF_MakePaths(Array*);

#endif /* KfData_INC */	

