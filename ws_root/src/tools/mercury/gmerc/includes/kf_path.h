#ifndef KeyFrame_INC
#define KeyFrame_INC 1

#include "kf_types.h"
#include "kf_spline.h"
#include "kf_quat.h"
#include "kf_eng.h"

/* function prototypes */
void
Kf_CreatePath(
    KfEngine *inKfData
    );

Int32
Kf_EvaluateAt( 
	KfEngine	*inKfPath,
	float		inTime, 
	Point3 		*outPnt,
	Quat		outRot,
	Point3 		*outScl	 
	);
Int32
Kf_UpdateObjAt(
    KfEngine   *inKfPath,
    float       inTime
    );
#endif /* KeyFrame_INC */	

