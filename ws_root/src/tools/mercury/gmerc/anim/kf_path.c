#include <math.h>
#include "kf_eng.h"
#include "kf_path.h"
#include "kf_quat.h"
#include "geoitpr.h"

#define	Array_GetData(a)		((Array *) (a))->m_Data
#define	Array_GetSize(a)		((Array *) (a))->m_Size
#define	Array_GetObj(a, i) \
	(*(((GfxObj**) ((Array*) (a))->m_Data) + (i)))
#define	Eng_GetDuration(e)	((EngineData*) (e))->m_Duration

/* prototype declarations */
extern char *strdup ( const char *s1 );

/*
**	Create a spline path out of attribute keys of an object
*/
void
Kf_CreatePath( 
	KfEngine *inKfData
	)
{
	int i;
	float sn, cs;
	KfPosTrack *pt = &inKfData->mObjPos;
	KfRotTrack *rt = &inKfData->mObjRot;
	KfScaleTrack *st = &inKfData->mObjScl;
	AxisRot3D *mRots = ( AxisRot3D * ) Array_GetData( rt->mRots );
	SplineProps *splProps;
	float *fd;
	int num_frames;

	/* Position Spline	 */
	if ( pt->mSplData == NULL )  splProps = NULL;
	else splProps = ( SplineProps *)Array_GetData( pt->mSplData );
	inKfData->mSplPos = Spl_Create( 
						(Int32)Array_GetSize( pt->mFrames ), 
						(Int32)3, 	 
						(float *)Array_GetData( pt->mFrames ), 
						(float *)Array_GetData( pt->mPnts ),
						splProps
						);

	num_frames = ( int ) Array_GetSize( pt->mFrames );
	fd = (float *)Array_GetData( pt->mFrames );
	inKfData->startFrame = fd[0];
	inKfData->endFrame =  fd[num_frames-1];
	
#if 0
	Spl_Print( inKfData->mSplPos );
#endif

	/* Rotation Spline	 */
	for( i = 0; i < Array_GetSize( rt->mFrames ); i++ )		
	{
		/* convert axis rotation to quaternion space */
		cs = (float)cos( (float)mRots[i].ang / 2 );
		sn = (float)sin( (float)mRots[i].ang / 2 );
		mRots[i].ang = cs;
		mRots[i].x *= sn;
		mRots[i].y *= sn;
		mRots[i].z *= sn;
	}
		
	if ( rt->mSplData == NULL )  splProps = NULL;
	else splProps = ( SplineProps *)Array_GetData( rt->mSplData );
	inKfData->mSplRot = Spl_Create( 
						(Int32)Array_GetSize( rt->mFrames ), 
						(Int32)4, 	 
						(float *)Array_GetData( rt->mFrames ), 
						(float *)Array_GetData( rt->mRots ), 
						splProps
						);

	num_frames = ( int ) Array_GetSize( rt->mFrames );
	fd = (float *)Array_GetData( rt->mFrames );
	if ( inKfData->startFrame > fd[0]) inKfData->startFrame = fd[0];
	if( inKfData->endFrame <  fd[num_frames-1]) inKfData->endFrame =  fd[num_frames-1];

#if 0
	Spl_Print( inKfData->mSplRot );
#endif
	
	/* Scaling Spline	 */
	if ( st->mSplData == NULL )  splProps = NULL;
	else splProps = ( SplineProps *)Array_GetData( st->mSplData );
	inKfData->mSplScl = Spl_Create( 
						(Int32)Array_GetSize( st->mFrames ), 
						(Int32)3, 	 
						(float *)Array_GetData( st->mFrames ), 
						(float *)Array_GetData( st->mScls ),  
						splProps
						);
		
	num_frames = ( int ) Array_GetSize( st->mFrames );
	fd = (float *)Array_GetData( st->mFrames );
	if ( inKfData->startFrame > fd[0]) inKfData->startFrame = fd[0];
	if( inKfData->endFrame <  fd[num_frames-1]) inKfData->endFrame =  fd[num_frames-1];

#if 0
	Spl_Print( inKfData->mSplScl );
#endif
}

/*
**	Create a spline paths for different objects
*/
void
KF_MakePaths( 
	Array *inKfEng
	)
{
	int i;

	for ( i = 0; i < Array_GetSize(inKfEng); i++ )
	{
		Kf_CreatePath( ( KfEngine *)Array_GetObj( inKfEng, i ) );
	}
}

/*
** Evaluate the keyframe spline path for position,
** rotation and scale values at a given time 
*/  
Int32
Kf_EvaluateAt( 
	KfEngine	*inKfPath,
	float		inTime, 
	Point3 		*outPnt,
	Quat		outRot,
	Point3 		*outScl	 
	)
{
	Int32 errorCode = error_None;
	float sn, halfAng, len;

	errorCode = Spl_EvaluateAt( 
							inKfPath->mSplPos,
							inTime,
							(float *)outPnt
							);
	errorCode = Spl_EvaluateAt( 
							inKfPath->mSplRot,
							inTime,
							(float *)outRot
							);
	errorCode = Spl_EvaluateAt( 
							inKfPath->mSplScl,
							inTime,
							(float *)outScl
							);
	return errorCode;
}

#if 0
/*
** For each KeyFrame object 
** evaluate the keyframe spline path for position,
** rotation and scale values and then print them 
*/  
Int32
Kf_Print(
	FILE		*fp,
    KfObjPath   *inKfPaths,
    int			inNumFrames
    )
{
	int i, j, k;
	KfObjPath   *kfCur;
	Int32 errorCode = error_None;
	Point3D     pnt;
	Quat   		rot;
	Point3D     scl;
	Matrix4D	mat;

	for ( k = 0; k <= inNumFrames; k++ )
	{
		kfCur = inKfPaths;
		while( kfCur )
		{
			Kf_EvaluateAt( kfCur, (float)k, &pnt, rot, &scl );
			fprintf(fp,  "p %s %g %g %g %g %g %g %g %g %g ",
					 (char *)kfCur->mObj,
					 pnt.x, pnt.y, pnt.z,
					 kfCur->mObjPivot.x,
					 kfCur->mObjPivot.y,
					 kfCur->mObjPivot.z,
					 kfCur->mPobjPivot.x,
					 kfCur->mPobjPivot.y,
					 kfCur->mPobjPivot.z
					 );
									
	    	Quat_ToMatrix( rot, mat );
			for( i=0; i < 3; i++ )
				for( j=0; j < 3; j++ )
					fprintf( fp, "%g ", mat[i][j] );
			fprintf( fp, "\n" );
			
			kfCur = kfCur->mObjNext;
		}
		fprintf( fp, "; End of frame %d\n", k);
		fprintf( fp, "d\n" );
	}

	return errorCode;
}  
#endif

/*
** Evaluate the keyframe spline path for Transform
** and update the object transform information
*/ 
Int32
Kf_UpdateObjAt(
	KfEngine   *inKfPath,
	float       inTime
	)
{
	Int32 errorCode = error_None;
#if 0
	Point3      pnt;
	Quat		rot;
	Point3      scl;
	Matrix4D	mat;
	Transform	*rt, *tr, *pmat;
	Point3 		tmp;
	float		cur_frame;
	float		duration;

	duration = Eng_GetDuration(&inKfPath->mEngine);
	if ( duration > 0 )
		cur_frame = ( ( inKfPath->endFrame - inKfPath->startFrame ) * inTime ) /  duration;
	else cur_frame = 0;

#if 0
	printf( "%f - %f, %f, %f, %f\n",  cur_frame, inKfPath->startFrame, inKfPath->endFrame,
			inTime, Eng_GetDuration(&inKfPath->mEngine ));
#endif

#if 0
	fprintf( stderr, " Char 2 = %s\n", Char_GetUserData( inKfPath->mObj )); 
#endif
	errorCode = Kf_EvaluateAt(
                            inKfPath,
                             cur_frame,
                            &pnt,
                            rot,
                            &scl
                            );
	Quat_ToMatrix( rot, mat );

	/* rotation transform */
	rt = Trans_Create( (MatrixData *)&mat );	

	/* translate to its pivot */
	tr= Trans_Create( NULL );
	Pt3_Set( &tmp,	-inKfPath->mObjPivot.x,
					-inKfPath->mObjPivot.y,
					-inKfPath->mObjPivot.z );
	Trans_Translate( tr, &tmp );

	/* apply scale */
	Trans_Scale( tr, &scl );

	/* apply rotation */
	Trans_PostMul( tr, rt );

	/* apply translation */
	Trans_Translate( tr, &pnt );

	/* translate to its parents pivot */
	Trans_Translate( tr, &(inKfPath->mPobjPivot) );

	/* apply transform to character */
	/* pmat = Char_GetTransform( inKfPath->mObj ); */
	pmat = (Transform *)&((CharData *)inKfPath->mObj)->m_Transform;
	Trans_Copy( pmat, tr );

	Trans_Delete( rt );
	Trans_Delete( tr );

#endif	
    return errorCode;
}  

#if 0
/*
** Update all of the characters at a given time
*/ 
Int32
Kf_UpdateObjsAt(
    KfObjPath   *inKfPath,
    float       inTime
    )
{
	Int32 errorCode = error_None;
	KfObjPath	*kfCur = inKfPath;

	while( kfCur )
	{
		/* update the object */
		Kf_UpdateObjAt( kfCur, inTime ); 

		kfCur = kfCur->mObjNext;
	}

	return errorCode;
}  

/*
** Delete all the object KeyFrame animation paths
*/
void
Kf_DeletePaths( 
	KfObjPath 	*inKf
	)
{
	KfObjPath 		*kfCur = inKf;
	KfObjPath 		*kfTmp;
	
	while( kfCur )
	{
		kfTmp = kfCur;
		kfCur = kfCur->mObjNext;
		
		/* delete all the attribute splines */
		Spl_Delete( kfTmp->mSplPos );
		Spl_Delete( kfTmp->mSplRot );
		Spl_Delete( kfTmp->mSplScl );
		
		free( kfTmp ); 
	}
}	
#endif
