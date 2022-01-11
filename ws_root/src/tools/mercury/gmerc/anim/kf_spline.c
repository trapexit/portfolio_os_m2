#include <math.h>
#include "kf_types.h"
#include "kf_spline.h"

/* Hermite spline basis functions */
static float sHermiteBasis[4][4] =
	{ {  2.0, -2.0,  1.0,  1.0 },
	  { -3.0,  3.0, -2.0, -1.0 },
	  {  0.0,  0.0,  1.0,  0.0 },
	  {  1.0,  0.0,  0.0,  0.0 } };

/*
** Create source and destination tangents for each key
*/
void
Spl_CreateTangents(
	KfSpline		*inSpl,
	SplineProps		*inCntrlKeys,
	float			*inAttKeys,
	float			*outSrcTngt,
	float			*outDstTngt
	)
{
  int i, j;
  float c11, c12, c21, c22, a1, a2, t1, t2;
  float t, b, c;
  UInt32 nAtt = inSpl->mNumAttribs;
  UInt32 nKeys = inSpl->mNumSegs + 1;
  float *aCur = inAttKeys;
  float *aPrev = aCur;
  float *aNext = aCur + nAtt;
  float endMultiplier;
  SplineProps	*cCur  = inCntrlKeys;
  SplineProps	*cPrev = cCur;
  SplineProps	*cNext = cCur + 1;
  float *sTngt = outSrcTngt;
  float *dTngt = outDstTngt;
  float *tVect = GetTimeVector( inSpl );
  float *tCur = tVect;
  float *tPrev = tCur;
  float *tNext = tCur + 1;
  double multiplier;
  float  absc;
  
  for( i = 0; i < nKeys ; i++ )
    {
      t = cCur->mTension;
      b = cCur->mBias;
      c = cCur->mContinuity;
      
      /* adjustment for parameter step size */
      t1 = 2.0 * ( *tCur - *tPrev ) / ( *tNext - *tPrev );
      t2 = 2.0 * ( *tNext - *tCur ) / ( *tNext - *tPrev );
      /*	t1 = t2 = 1.0; */
      /*printf ( "---Time Data 1: %f, %f, %f\n",*tPrev, *tCur, *tNext ); */
      
      /*  Continuity correction.  3DS needs this.  LightWave?  */
      absc = fabs(c);
      t1 = (t1 + absc) - absc*t1;
      t2 = (t2 + absc) - absc*t2;
      
      /* spline control factors */
      c11 = 0.5*( ( 1 - t ) * ( 1 - c ) * ( 1 + b ) );
      c12 = 0.5*( ( 1 - t ) * ( 1 + c ) * ( 1 - b ) );
      c21 = 0.5*( ( 1 - t ) * ( 1 + c ) * ( 1 + b ) );
      c22 = 0.5*( ( 1 - t ) * ( 1 - c ) * ( 1 - b ) );
      
      for( j = 0; j < nAtt; j++ )
	{ 
	  /* data to be interpolated */
	  a1 = aCur[ j ] - aPrev[ j ];
	  a2 = aNext[ j ] - aCur[ j ];
	  
	  if (i==(nKeys-1))
	    *sTngt =(float)(1.5*(c11+c12)*a1);
	  else
	    *sTngt = (float)(t1*((c11*a1)+(c12*a2)));
	  if (i==0)
	    *dTngt =(float)(1.5*(c21+c22)*a2);
	  else
	    *dTngt = (float)(t2*((c21*a1)+(c22*a2)));
	  
	  sTngt++;
	  dTngt++;
	}
      /* time vector */
      tPrev = tCur;
      tCur  += 1;		
      /* spline properties */
      cPrev = cCur;
      cCur  += 1;
      /* attribute data */
      aPrev = aCur;
      aCur  += nAtt;
      
      /* last key */
      if ( i >= ( nKeys - 2 ) )
	{
	  aNext = aCur;
	  cNext = cCur;
	  tNext = tCur;
	} else {
	  aNext = aCur + nAtt;
	  cNext = cCur + 1;
	  tNext = tCur + 1;
	}  
    }
}

/*
** Compute coefficients for each spline segment
*/
void
Spl_ComputeCoefficients( 
	KfSpline		*inSpl,
	SplineProps		*inCntrlKeys,
	float			*inAttKeys 
	)
{
	int i, j, k;
	UInt32 nAtt = inSpl->mNumAttribs;
	UInt32 nKeys = inSpl->mNumSegs + 1;
	float  *aCur = inAttKeys;
	float  *aNext = aCur + nAtt;
	float  *cCur = GetAttData( inSpl );
	float  *srcTngts = ( float * ) MemAlloc( sizeof( float ) * nKeys * nAtt ); 	
	float  *dstTngts = ( float * ) MemAlloc( sizeof( float ) * nKeys * nAtt );
	float  *sNext = srcTngts + nAtt;
	float  *dCur = dstTngts;
	float  p[ 4 ];
	 
	
	Spl_CreateTangents( inSpl, inCntrlKeys, inAttKeys, srcTngts, dstTngts );
	
	for( i = 0; i < ( nKeys - 1 ) ; i++ )
	{		
		for( j = 0; j < nAtt; j++ )
		{ 
			/* coefficients for the i'th spline segment */
			p[ 0 ] = aCur[ j ];
			p[ 1 ] = aNext[ j ];
			p[ 2 ] = dCur[ j ];
			p[ 3 ] = sNext[ j ];
			/*printf( "--%f, %f, %f, %f\n", p[0], p[1],p[2],p[3] ); */
			for( k= 0; k < 4; k++ )
				cCur[ k ] = sHermiteBasis[ k ][ 0 ] * p[ 0 ] +
				            sHermiteBasis[ k ][ 1 ] * p[ 1 ] +
				            sHermiteBasis[ k ][ 2 ] * p[ 2 ] +
				            sHermiteBasis[ k ][ 3 ] * p[ 3 ];
			cCur     += 4;
		}
		sNext += nAtt;
		dCur  += nAtt;
		aCur   = aNext;
		aNext  = aCur + nAtt;
	}
	
	free( srcTngts );
	free( dstTngts );		
}

/*
** Print spline data
*/
void
Spl_Print( 
	KfSpline		*inSpl,
	UInt32 nAtt
	)
{
	int i, j;
	UInt32 nKeys = inSpl->mNumSegs + 1;
	float  *cCur = GetAttData( inSpl );
	float  *tCur = GetTimeVector( inSpl );

	if ( nKeys == 1 )
	{
		printf( "KfSpline \n" );
		printf( "    Time value: %f\n", *tCur );
		printf( "    Attributes : " );
		for( j = 0; j < nAtt; j++ ) printf( "%f ", cCur[j] );
		printf( "\n" );
	} else {		
		for( i = 0; i < ( nKeys - 1 ); i++ )
		{	
			printf( "\tKfSpline Segment_%d\n", i );
			
			printf( "\t    Time values: %f, %f\n", tCur[i], tCur[i+1] );	
			
			for( j = 0; j < nAtt; j++ )
			{ 
				printf( "\t    Attribute_%d coefficients\n", j );
				printf( "\t        %f, %f, %f, %f\n",
								cCur[ 0 ], cCur[ 1 ],
								cCur[ 2 ], cCur[ 3 ] );
				
				cCur     += 4;
			}
		}
	}
}

/*
** Create spline path with default spline controls
** tension = 0.0, bias = 0.0, continuity = 0.0
** given are the data to be interpolated and time
** parameter values at each key
*/
KfSpline*
Spl_Create( 
	Int32			inNumKeys, 
	Int32			inNumAtts, 	 
	float			*inTimeKeys, 
	float			*inAttKeys, 
	SplineProps		*inCntrlKeys
	)
{
	int				i;
	KfSpline 		*spl;
	
	if ( inCntrlKeys == NULL )
	{
		SplineProps		*splCntrls;
		
		splCntrls = ( SplineProps * ) MemAlloc( inNumKeys * 
		                              sizeof( SplineProps ) );
		for( i = 0; i < inNumKeys; i++ )
		{
			splCntrls[ i ].mTension = default_SplineTension;                    
			splCntrls[ i ].mBias = default_SplineBias;                     
			splCntrls[ i ].mContinuity = default_SplineContinuity;                    
			splCntrls[ i ].mEaseIn = default_SplineEaseIn;                     
			splCntrls[ i ].mEaseOut = default_SplineEaseOut;
		}                     
		spl = Spl_CreatePrivate( 
			inNumKeys, 
			inNumAtts, 
			inTimeKeys, 
			inAttKeys, 
			splCntrls 
			);
		free( splCntrls );
	} else 
		spl = Spl_CreatePrivate( 
			inNumKeys, 
			inNumAtts, 
			inTimeKeys, 
			inAttKeys, 
			inCntrlKeys 
			);
	 	
	return ( spl );
}

/*
** Create spline segment with one key
*/
KfSpline*
Spl_CreateOneKey( 
	Int32			inNumAtts, 	 
	float			*inTimeKeys, 
	float			*inAttKeys
	)
{
	int i;
	KfSpline *spl;
	int splSize;
	float *tVect;
	float *aVect;
	
	/* allocate memory	 */
	/* (inNumAtts is already decremented by 1) */
	splSize = sizeof(KfSpline) + sizeof(float) * inNumAtts;
	spl = ( KfSpline *) MemAlloc( splSize );
	spl->refCount = 1;
	
	spl->mNumSegs = 0;
	spl->mNumAttribs = inNumAtts;
	tVect = GetTimeVector( spl );
	*tVect = *inTimeKeys;
	aVect = GetAttData( spl );
	
	for( i = 0; i < inNumAtts; i++ ) aVect[ i ] = inAttKeys[ i ];
		
	return ( spl );
}

/*
** Create spline path with given spline controls
** given are the data to be interpolated and time
** parameter values at each key
*/
KfSpline*
Spl_CreatePrivate( 
	Int32			inNumKeys, 
	Int32			inNumAtts, 	 
	float			*inTimeKeys, 
	float			*inAttKeys, 
	SplineProps		*inCntrlKeys
	)
{
	int i;
	KfSpline *spl;
	int splSize;
	float *tVect;

	if ( inNumKeys == 1 )
		/* create a spline with one attribute key */
		spl = Spl_CreateOneKey( inNumAtts,inTimeKeys,inAttKeys );
	else {
		/* allocate memory for interpolation spline */
		splSize = sizeof(KfSpline) +
		    (inNumKeys + (inNumKeys - 1) * inNumAtts * 4 - 1) *
			sizeof(float);
		spl = ( KfSpline *) MemAlloc( splSize );
		spl->refCount = 1;
		
		spl->mNumSegs = inNumKeys - 1;
		spl->mNumAttribs = inNumAtts;
		tVect = GetTimeVector( spl );
		for( i = 0; i < inNumKeys; i++ ) tVect[ i ] = inTimeKeys[ i ];
			
		Spl_ComputeCoefficients( spl, inCntrlKeys,inAttKeys );
	}
	return ( spl );
}

/*
** Delete the spline
*/
void
Spl_Delete(
	KfSpline	*inSpline
	)
{
    if (--(inSpline->refCount) <= 0)
	free( inSpline );
}

void
Spl_IncUsage(KfSpline *inSpline)

{
    inSpline->refCount++;
}

/*
** Find out the spline segment on which this parameter exists
*/
Int32
Spl_FindSegIndex(
	KfSpline		*inSpl,
	float			*inParm,
	UInt32			*outSegIndx
	)
{
	int nSegs      = inSpl->mNumSegs;
	float *tVect   = GetTimeVector( inSpl );
	UInt32 i       = 0;

	if ( ( (*inParm) < tVect[ 0 ] ) )
	{
		*outSegIndx =  0;
		*inParm     = tVect[ 0 ];
		return error_OutOfBounds;
	} else if ( (*inParm) > tVect[ nSegs ] ) {
		*outSegIndx = nSegs - 1;
		*inParm     = tVect[ nSegs ];
		return error_OutOfBounds;
	} else {
		while( !( ( tVect[i] <= (*inParm) ) && ( tVect[i+1] >= (*inParm) ) ) ) 
		{
			/*printf( "tVect[%d] = %f\n", i,tVect[i] );  */
			i++;
		}
		*outSegIndx = i;
		return error_None;
	}
}
 
/*
** Evaluate the spline at a given time for the 
** interpolated attributes. This function assumes
** that the "outAttribs" array size is enough to
** fit "mNumAttribs".
*/  
Int32
Spl_EvaluateAt( 
	KfSpline	*inSpl, 
	float		inTime, 
	float		*outVal 
	)
{
	int i, j;
	Int32 errorCode = error_None;
	UInt32 nAtt     = inSpl->mNumAttribs;
	float *tVect    = GetTimeVector( inSpl );
	float  *cCur    = GetAttData( inSpl );
	float  atFrame  = inTime;
	float  s;
	UInt32 si; 

	/* if spline has only one key then simply copy original attributes */
	if ( inSpl->mNumSegs == 0 )
	{
		for( i = 0; i < inSpl->mNumAttribs; i++ ) outVal[ i ] = cCur[ i ];
	/* interpolate the attribute values if it has more than one key */
	} else {
		errorCode = Spl_FindSegIndex( inSpl, &atFrame, &si );
		
		cCur += ( si * nAtt * 4 );
		s = ( atFrame - tVect[ si ]  ) / ( tVect[ si+1 ] - tVect[ si ] );
		/*	printf ( " ****Indx = %d, Parm = %f, %f\n", si, atFrame, s ); */
			
		for( j = 0; j < nAtt; j++ )
		{ 
			outVal[ j ] = s * ( s * ( s * cCur[ 0 ] + cCur[ 1 ] ) + 
			              cCur[ 2 ] ) + cCur[ 3 ];
			
			cCur     += 4;
		}	
	}
	
	return errorCode;
}

