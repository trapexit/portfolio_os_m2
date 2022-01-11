#include "bsdf_iff.h"
#include "writeiff.h"

#include "gp.i"
#include "tmutils.h"
#include "bsdf_proto.h"
#include "bsdf_write.h"
#include "kf_path.h"
#include "bsdf_anim.h"
#include "texpage.h"

#define INTERFACE_CODE	1
#ifdef INTERFACE_CODE
extern int mercid;
extern Endianness gEndian;
#endif
	
/*
**	Write animation data
*/
Err
ANIM_WriteChunk(
		SDFBObject* pData, 
		GfxObj* pObj
		)
{
  Err result = GFX_OK;
  uint32 objID;
  EngineData *eng = (EngineData *)pObj;
  KfEngine *kf_eng = ( KfEngine *)pObj;
  int16 data1[2];
  float data2[3];
  uint32 size[2], seg_size;
  uint32 anim_type = 0;
  float *data_ptr;
  int32 i;

  if ( ( pObj == NULL ) || (( pObj ) == NULL) )  goto err;
	
  /* save the animation node for later processing */
  if( pData->numAnimNodes == MAX_ANIM_NODES )
    {
      fprintf( stderr, "ERROR: Animation node stack over-flow\n" );
      result = -1;
      goto err;
    }
#ifndef INTERFACE_CODE
  pData->animNodes[pData->numAnimNodes++] = kf_eng;
#endif
#ifdef INTERFACE_CODE
  ((GfxObj *)((KfEngine *)kf_eng->mObj))->m_Ref = pObj;
  if ((GetAnimFlag() & GEOM_FLAG) && (!(GetAnimFlag() & ANIM_FLAG)))
    return result;
  pObj->m_Objid = mercid++;
#endif

  /* write chunk data */
#ifdef INTERFACE_CODE
  objID = pObj->m_Objid;
  /*    printf("Anim chunk data objID=%d\n", objID); */
#endif
    
  /* start writing the chunk data */
  result = PushChunk( pData->iff, 0L, ID_ANIM, IFF_SIZE_UNKNOWN_32 );
  if( result < 0 ) goto err;
	
  data1[0] = eng->m_Status;
  data1[1] = eng->m_Control;
	
  if( PutUInt32( pData->iff, objID, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
		
  /* Create splines out of key data */
  Kf_CreatePath( kf_eng );

  /* animation type flags */
  if( kf_eng->mSplPos->mNumSegs == 0 ) anim_type |= TRANS_CONST;	
  else anim_type |= TRANS_CUBIC;
  if( kf_eng->mSplRot->mNumSegs == 0 ) anim_type |= ROT_CONST;	
  else anim_type |= ROT_CUBIC;

  if( PutUInt32( pData->iff, anim_type, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	

  /* calculate the sizes of the trans and rotational data */
  if( kf_eng->mSplPos->mNumSegs == 0 ) /* constant key */
    size[0] = sizeof(float) * 3;
  else {
    seg_size = kf_eng->mSplPos->mNumAttribs * 4; /* 12 coefficients */
    size[0] = sizeof(KfSpline) - 2 * sizeof( UInt32 ) +
      ( ( kf_eng->mSplPos->mNumSegs+1 ) + 
	kf_eng->mSplPos->mNumSegs * seg_size - 1 ) *
      sizeof(float);
  }

  if( kf_eng->mSplRot->mNumSegs == 0 ) /* constant key */
    size[1] = sizeof(float) * 4;
  else {
    seg_size = kf_eng->mSplRot->mNumAttribs * 4; /* 16 coefficients */
    size[1] = sizeof(KfSpline) - 2 * sizeof( UInt32 ) +
      ( ( kf_eng->mSplRot->mNumSegs+1 ) + 
	kf_eng->mSplRot->mNumSegs * seg_size - 1 ) *
      sizeof(float);
  }

  if( PutUInt32s( pData->iff, size, 2, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	

  /* write translational spline data */
#ifndef INTERFACE_CODE
  if( anim_type & TRANS_CONST ) data_ptr = GetAttData( kf_eng->mSplPos );
  else data_ptr = &kf_eng->mSplPos->mNumSegs;
#endif
#ifdef INTERFACE_CODE
  if( anim_type & TRANS_CONST ) data_ptr = GetAttData( kf_eng->mSplPos );
  else data_ptr = (float *)&kf_eng->mSplPos->mNumSegs;
#endif

#if 0
  printf("size[0]/4 = %d\n", size[0]/4);
  for (i = 0; i < size[0] / 4; i++)
    printf("i = %d, data = %f\n", i, *(data_ptr+i));
#endif

  if( PutFloats( pData->iff, data_ptr, size[0] / 4, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	

  /* write rotational spline data */
#ifndef INTERFACE_CODE
  if( anim_type & ROT_CONST ) data_ptr = GetAttData( kf_eng->mSplRot );
  else data_ptr = &kf_eng->mSplRot->mNumSegs;
#endif
#ifdef INTERFACE_CODE
  if( anim_type & ROT_CONST ) data_ptr = GetAttData( kf_eng->mSplRot );
  else data_ptr = (float *)&kf_eng->mSplRot->mNumSegs;
#endif

  /*
    printf("size[1]/4 = %d\n", size[1]/4);
    for (i = 0; i < size[1] / 4; i++)
    printf("i = %d, data = %f\n", i, *(data_ptr+i));
    */

  if( PutFloats( pData->iff, data_ptr, size[1] / 4, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
#if 0
  printf( "------Positional Spline\n" );
  Spl_Print( kf_eng->mSplPos, 3 );
  printf( "------Rotational Spline\n" );
  Spl_Print( kf_eng->mSplRot, 4 );
#endif
	
  /* clean the spline data after it is written */
  Spl_Delete(kf_eng->mSplPos);
  Spl_Delete(kf_eng->mSplRot);
  Spl_Delete(kf_eng->mSplScl);

  result = PopChunk( pData->iff );
  if( result < 0 ) goto err;
	
err:    
  return result;
}

