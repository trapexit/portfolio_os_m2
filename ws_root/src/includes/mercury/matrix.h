/*
 * @(#) matrix.h 96/05/08 1.10
 *
 */
#ifndef _MATRIX_H_
#define _MATRIX_H_

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <misc:event.h>
#include <string.h>
#include <math.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <misc/event.h>
#include <string.h>
#include <math.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/*
 * General type definitions
 */
typedef unsigned char			gbyte;
typedef unsigned int			dword;
typedef unsigned long			gdword;
typedef char	 				gchar;
typedef int						gint;
typedef long					glong;
typedef float                   gfloat;
typedef double					gdouble;

/*
typedef enum { FALSE,  TRUE } 	gbool;
 */
typedef int						gbool;
#define bool 					gbool

#define AXIS_X 'X'
#define AXIS_Y 'Y'
#define AXIS_Z 'Z'

#define AXIS_PITCH 'X'
#define AXIS_YAW   'Y'
#define AXIS_ROLL  'Z'

#define AXIS_ELEVATION 'X'
#define AXIS_HEADING   'Y'
#define AXIS_BANK      'Z'

/*
 * Math Constants
 */
#ifndef PI
#define PI			(3.141592654f)
#endif
#define INVERSE_PI	(0.318309886f)
#define DEG_TO_RAD	(0.017453292f)
#define RAD_TO_DEG	(57.29577951f)

#define RAD0	    (0.0f)
#define RAD90		(1.570796327f)
#define RAD180		(3.141592654f)
#define RAD270      (4.712388981f)
#define RAD360		(6.283185307f)

/*
 * Math precision-error constants
 */
#define ZERO		(0.001f)
#define SMALL		(0.00001f)
#define CLOSE		(0.1f)
#define NEAR		(10.0f)
#define LARGE		(1000000.0f)
#define INFINITY	(1.0e+20)

/*
 * Random numbers
 */
#define  randomize() srand ( time (NULL) )
#define  random(num) (int)(((long)rand()*(num))/(RAND_MAX+1))

/*
 * Single-valued functions
 */
#define RAD(x)	((x) * DEG_TO_RAD)
#define DEG(x)	((x) * RAD_TO_DEG)

#define COS(x)  (cosf( (x) ))
#define SIN(x)  (sinf( (x) ))
#define COSD(x) (cosf( RAD( (x) )))
#define SIND(x) (sinf( RAD( (x) )))
#define ACOS(x) (acosf( (x) ))
#define ASIN(x) (asinf( (x) ))
#define ACOSD(x) (DEG (acosf((x))) )
#define ASIND(x) (DEG (asinf((x))) )

#define ABS(x)	(((x) < 0) ? (-(x)) : (x))

#define NINT(x)		((glong)((x) >= 0.0) ? ((x) + 0.5) : ((x) - 0.5)))
#define NINTPOS(x)	((glong)((x) + 0.5))
#define FLOOR(x)	((x) > 0 ? (gint)(x) : -(gint)(x))
#define CEILING(x)	((x) == (gint)(x) ? (x) : (x) > 0 ? (1+(gint)(x)) : -(1+(gint)(-x))
#define ROUND(x)	((gint)(x) + 0.5)
#define ROUND_2(x)	((x) > 0 ? (gint)((x) + 0.5) : -(gint)(0.5 - (x)))
#define TRUNC(x)	((gint)(x))
#define FRAC(x)		((x) - (gdouble)( (gint)(x) ))

#define SQR(x)		((x) * (x))
#define SQRT(x)		(sqrt( (x) ))
#define CUBE(x)		((x) * (x) * (x))

#define POWER(x,y)  (pow( (x), (y) ))

#define SIGN(x)             \
	{ 						\
		if (x < 0) 			\
			return(-1); 	\
		else				\
		{ 					\
			if (x > 0) 		\
				return(1); 	\
			else 			\
				return(0);	\
		}					\
	}

/*
 * Definitions for Vectors, Points, Normals & Matrices
 */

/*
 * Vector2D definitions
 */
typedef struct {
	int	x;
	int	y;
} Vector2Di, *pVector2Di;

#define Normal2Di Vector2Di
#define pNormal2Di pVector2Di

#define Point2Di Vector2Di
#define pPoint2Di pVector2Di

typedef struct {
    float   x;
    float   y;
} Vector2D, *pVector2D;

#define Normal2D Vector2D
#define pNormal2D pVector2D

#define Point2D Vector2D
#define pPoint2D pVector2D

/*
 * Vector3D definitions
 */
typedef struct {
	int     x;
	int     y;
	int     z;
} Vector3Di, *pVector3Di;

#define Normal3Di Vector3Di
#define pNormal3Di pVector3Di

#define Point3Di Point3Di
#define pPoint3Di pPoint3Di

typedef struct {
    float   x;
    float   y;
    float   z;
} Vector3D, *pVector3D;

#define Normal3D Vector3D
#define pNormal3D pVector3D

#define Point3D Vector3D
#define pPoint3D pVector3D

/*
#define Point3 Vector3D
#define pPoint3 pVector3D
*/

/*
 * Vector4D definitions
 */
typedef struct {
	int     x;
	int     y;
	int     z;
	int     w;
} Vector4Di, *pVector4Di;

#define Normal4Di Vector4Di
#define pNormal4Di pVector4Di

#define Point4Di Vector4Di
#define pPoint4Di pVector4Di

typedef struct {
    float   x;
    float   y;
    float   z;
    float   w;
} Vector4D, *pVector4D;

#define Normal4D Vector4D
#define pNormal4D pVector4D

#define Point4D Vector4D
#define pPoint4D pVector4D

/*
 * Definitions for Matrices
 */
typedef struct {
    float   mat[2][2];
} Matrix2x2, *pMatrix2x2;

typedef struct {
    float   mat[3][3];
} Matrix3x3, *pMatrix3x3;

typedef struct {
    float mat[4][3];
} Matrix, *pMatrix;
#define Matrix3x4 Matrix
#define pMatrix3x4 pMatrix

typedef struct {
    float   mat[4][4];
} Matrix4x4, *pMatrix4x4;

extern  Matrix2x2 matrixIdentity2x2;
extern  Matrix3x3 matrixIdentity3x3;
extern  Matrix matrixIdentity;
extern  Matrix4x4 matrixIdentity4x4;

/*
 *
 */
typedef struct ViewPyramid {
    float left, right, top, bottom, hither;
} ViewPyramid, *pViewPyramid;

/*
 * Vector2D function prototypes
 */
void Vector2D_Set(Vector2D *v, float x, float y);
void Vector2D_Average(Vector2D *v, Vector2D *a, Vector2D *b);
void Vector2D_Negate(Vector2D *v);
void Vector2D_NegateCopy(Vector2D *v, Vector2D *a);
float Vector2D_Dot(Vector2D *v, Vector2D *a);
float Vector2D_Length(Vector2D *v);
void Vector2D_Normalize(Vector2D *v);
void Vector2D_Minimum(Vector2D *v, Vector2D *a, Vector2D *b);
void Vector2D_Maximum(Vector2D *v, Vector2D *a, Vector2D *b);
bool Vector2D_Compare(Vector2D *v, Vector2D *a);
void Vector2D_Copy(Vector2D *v, Vector2D *a);
void Vector2D_Add(Vector2D *v, Vector2D *a, Vector2D *b);
void Vector2D_Sub(Vector2D *v, Vector2D *a, Vector2D *b);
void Vector2D_Scale(Vector2D *v, float scaleX, float scaleY);
void Vector2D_Multiple(Vector2D *v, Vector2D *a, Vector2D *b);
void Vector2D_Zero(Vector2D *v);
void Vector2D_Print(Vector2D *v, char *msg);

/*
 * Vector3D function prototypes
 */
Vector3D* Vector3D_Construct(void);
void Vector3D_Destruct(Vector3D *v);
void Vector3D_Set(Vector3D *v, float x, float y, float z);
float Vector3D_GetX(Vector3D *v);
float Vector3D_GetY(Vector3D *v);
float Vector3D_GetZ(Vector3D *v);
void Vector3D_Average(Vector3D *v, Vector3D *a, Vector3D *b);
void Vector3D_Negate(Vector3D *v);
float Vector3D_Dot(Vector3D *v, Vector3D *a);
float Vector3D_Length(Vector3D *v);
void Vector3D_Normalize(Vector3D *v);
void Vector3D_Minimum(Vector3D *v, Vector3D *a, Vector3D *b);
void Vector3D_Maximum(Vector3D *v, Vector3D *a, Vector3D *b);
bool Vector3D_Compare(Vector3D *v, Vector3D *a);
bool Vector3D_CompareFuzzy( Vector3D *v, Vector3D *a, float fuzzy );
void Vector3D_Copy(Vector3D *v, Vector3D *a);
void Vector3D_Add(Vector3D *v, Vector3D *a, Vector3D *b);
void Vector3D_Subtract( Vector3D *v, Vector3D *a, Vector3D *b);
void Vector3D_Scale( Vector3D *v, float x, float y, float z );
void Vector3D_Multiply(Vector3D *v, Vector3D *a, Vector3D *b);
void Vector3D_Cross(Vector3D *v, Vector3D *a, Vector3D *b);
void Vector3D_Zero(Vector3D *v);
void Vector3D_Print(Vector3D *v );
void Vector3D_MultiplyByMatrix(Vector3D *v, Matrix *m);
void Vector3D_OrientateByMatrix(Vector3D *v, Matrix *m);

#define Vector3D_GetX(v) (v)->x
#define Vector3D_GetY(v) (v)->y
#define Vector3D_GetZ(v) (v)->z

/*
 * Matrix Function Prototypes
 */
Matrix* Matrix_Construct( void );
void Matrix_Destruct( Matrix *matrix );
void Matrix_Normalize(Matrix *m);
void Matrix_Copy(Matrix *m, Matrix *a);
void Matrix_Mult(Matrix *m, Matrix *a, Matrix *b);
void Matrix_Multiply(Matrix *m, Matrix *a);
void Matrix_MultOrientation(Matrix *m, Matrix *a, Matrix *b);
void Matrix_MultiplyOrientation(Matrix *m, Matrix *a);
void Matrix_Zero(Matrix *m);
void Matrix_Identity(Matrix *m);
void Matrix_GetTranslation(Matrix *m, Vector3D *v);
void Matrix_SetTranslationByVector(Matrix *m, Vector3D *v);
void Matrix_SetTranslation(Matrix *m, float x, float y, float z);
Vector3D* Matrix_GetpTranslation(Matrix *m);
void Matrix_TranslateByVector(Matrix *m, Vector3D *v);
void Matrix_Translate(Matrix *m, float x, float y, float z);
void Matrix_MoveByVector(Matrix *m, Vector3D *v);
void Matrix_Move(Matrix *m, float x, float y, float z);
void Matrix_Scale(Matrix *m, float x, float y, float z);
void Matrix_ScaleByVector(Matrix *m, Vector3D *v);
void Matrix_ScaleLocal(Matrix *m, float x, float y, float z);
void Matrix_ScaleLocalByVector(Matrix *m, Vector3D *v);
void Matrix_Rotate(Matrix *m, char axis, float r);
void Matrix_RotateX(Matrix *m, float r);
void Matrix_RotateY(Matrix *m, float r);
void Matrix_RotateZ(Matrix *m, float r);
void Matrix_RotateLocal(Matrix *m, char axis, float r);
void Matrix_RotateXLocal(Matrix *m, float r);
void Matrix_RotateYLocal(Matrix *m, float r);
void Matrix_RotateZLocal(Matrix *m, float r);
void Matrix_Turn(Matrix *m, char axis, float r);
void Matrix_TurnX(Matrix *m, float r);
void Matrix_TurnY(Matrix *m, float r);
void Matrix_TurnZ(Matrix *m, float r);
void Matrix_TurnLocal(Matrix *m, char axis, float r);
void Matrix_TurnXLocal(Matrix *m, float r);
void Matrix_TurnYLocal(Matrix *m, float r);
void Matrix_TurnZLocal(Matrix *m, float r);
void Matrix_Print(Matrix *m);
float Matrix_GetHeading(Matrix *m);
float Matrix_GetBank(Matrix *m);
float Matrix_GetElevation(Matrix *m);

void Matrix_LookAt(Matrix *m, Vector3D *pWhere, Vector3D *pMe, float twist);
Err Matrix_FullInvert(Matrix *m, Matrix *src);
void Matrix_Invert(Matrix *m, Matrix *src);
void Matrix_BillboardX(Matrix *m, Vector3D *a);
void Matrix_BillboardY(Matrix *m, Vector3D *a);
void Matrix_BillboardZ(Matrix *m, Vector3D *a);
void Matrix_CopyOrientation( Matrix *m, Matrix *b );
void Matrix_CopyTranslation( Matrix *m, Matrix *b );

void Matrix_Perspective(pMatrix, pViewPyramid,
		    float screen_xmin, float screen_xmax,
		    float screen_ymin, float screen_ymax, float wscale);

#endif
/*  End of File */
