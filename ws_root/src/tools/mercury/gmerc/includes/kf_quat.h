/* 
** Quaternion routines
*/

#ifndef QUATERNION_INC
#define QUATERNION_INC

#include "kf_types.h"

#define EPSILON 0.00001
#define HALFPI 1.570796326794895

enum { X, Y, Z, W };

typedef float Matrix4D[4][4];
typedef float Quat[4];

void
AxisRotToMat( 
	AxisRot3D v, 
	Matrix4D mat 
	);
void 
Quat_ToMatrix( 
	Quat q, 
	Matrix4D mat 
	);
void 
Quat_FromMatrix( 
	Matrix4D mat, 
	Quat q 
	);
void 
Quat_Mul( 
	Quat qL, 
	Quat qR, 
	Quat qq 
	);
void 
Quat_Slerp( 
	Quat qL, 
	Quat qR, 
	float t, 
	Quat qq 
	);
void 
Quat_Conj( 
	Quat q, 
	Quat qq 
	);

#endif
