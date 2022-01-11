/*
**	File:		PrintSDF.c++	
**
**	Contains:	Alias v6.0 wire to M2 SDF file converter
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1994 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Change History (most recent first):
**	5-30-95 Reddy 	do not write any texture coordinates if the model
**              	does not refer to a texture
**
**	To Do:
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <AlCurve.h>
#include <AlCurveCV.h>
#include <AlCurveNode.h>
#include <AlCurveOnSurface.h>
#include <AlDagNode.h>
#include <AlFace.h>
#include <AlFaceNode.h>
#include <AlGroupNode.h>
#include <AlSurface.h>
#include <AlSurfaceCV.h>
#include <AlSurfaceNode.h>
#include <AlCamera.h>
#include <AlOrthographicCamera.h> 
#include <AlPerspectiveCamera.h>
#include <AlCameraNode.h>
#include <AlCluster.h>
#include <AlClusterNode.h>
#include <AlClusterMember.h>
#include <AlPointLight.h>
#include <AlAmbientLight.h>
#include <AlDirectionLight.h>
#include <AlSpotLight.h>
#include <AlLinearLight.h>
#include <AlAreaLight.h>
#include <AlLightNode.h>
#include <AlLight.h>

/* #include <AlNewTrimRegion.h> */
/* #include <AlNewTrimBoundary.h> */
/* #include <AlNewTrimCurve.h> */

#include <AlTrimRegion.h>
#include <AlTrimBoundary.h>
#include <AlTrimCurve.h>

#include <AlSet.h>
#include <AlSetMember.h>
#include <AlUniverse.h>

#include <AlChannel.h>
#include <AlAction.h>
#include <AlParamAction.h>
#include <AlMotionAction.h>
#include <AlKeyframe.h>
/* #include <AlStream.h> */

#include <AlShader.h>
#include <AlTexture.h>
#include <AlEnvironment.h>

#include <AlPolysetNode.h>
#include <AlPolyset.h>
#include <AlPolygon.h>
#include <AlPolysetVertex.h>

#include <AlAttributes.h>
#include <AlArcAttributes.h>
#include <AlLineAttributes.h>
#include <AlCurveAttributes.h>
#include <AlConicAttributes.h>
#include <AlPlaneAttributes.h>
#include <AlRevSurfAttributes.h>
#include <AlJointNode.h>

#include <PrintSDF.h>

// Indentation parameters
static int		indentation = 0;
static char		indentationBuffer[256] = "";

// Command line  parameters
static FILE*	printFile = stdout;
static FILE*	scriptFile = NULL;
static AlTesselateTypes tessPolys = kTESSELATE_TRIANGLE;
static double tessQuality = 0.5;
static char prefixStr[256] = "";
static char topMdlName[256] = "";
static int writeTexRefs = 1;
static int writeLightInfo = 1;
static int writeLightMaterialInfo = 0;
static int has_anim = 0;

#define	INDENT		1
#define	TAI_DEBUG	1
#define	PI			3.141592653589793238462643
#define	FRAMERATE 	30.0

typedef struct M2_Material {
	double d_r;
	double d_g;
	double d_b;
	double d_a;
	double s_r;
	double s_g;
	double s_b;
	double s_a;
	double e_r;
	double e_g;
	double e_b;
	double e_a;
	int dflag;
	int sflag;
	int eflag;
	int matindex;
} M2_Material;

static int Color_Flag = 0;
void SDF_SetColorFlag(int flag)
{
	Color_Flag = flag;
}

int SDF_GetColorFlag(void)
{
	return Color_Flag;
}

static M2_Material *matlist;
static int num_material = 0;
static int cur_material = 0;
static int matcount = 0;

static void CreateMaterialList
	(
	int shadercount
	)
{
	int i;

	num_material = shadercount;
	matlist = (M2_Material *)malloc(num_material * sizeof(M2_Material));
	for (i = 0; i < num_material; i++)
	{
		matlist[i].d_r = 0.0;
		matlist[i].d_g = 0.0;
		matlist[i].d_b = 0.0;
		matlist[i].d_a = 0.0;
		matlist[i].s_r = 0.0;
		matlist[i].s_g = 0.0;
		matlist[i].s_b = 0.0;
		matlist[i].s_a = 0.0;
		matlist[i].e_r = 0.0;
		matlist[i].e_g = 0.0;
		matlist[i].e_b = 0.0;
		matlist[i].e_a = 0.0;
		matlist[i].dflag = 0;
		matlist[i].sflag = 0;
		matlist[i].eflag = 0;
		matlist[i].matindex = i;
	}
}

static int Get_M2_Material
	(
	int index
	)
{
	M2_Material *mat;
	int i;

	mat = matlist + index;

	for (i = 0; i < cur_material; i++)
	{
		if (( matlist[i].d_r == mat->d_r) &&
			(matlist[i].d_g == mat->d_g) &&
			(matlist[i].d_b == mat->d_b) &&
			(matlist[i].d_a == mat->d_a) &&
			(matlist[i].s_r == mat->s_r) &&
			(matlist[i].s_g == mat->s_g) &&
			(matlist[i].s_b == mat->s_b) &&
			(matlist[i].s_a == mat->s_a) &&
			(matlist[i].e_r == mat->e_r) &&
			(matlist[i].e_g == mat->e_g) &&
			(matlist[i].e_b == mat->e_b) &&
			(matlist[i].e_a == mat->e_a))
			return matlist[i].matindex;
	}
	return index;
}

static int Add_M2_Material
	(
	M2_Material *mat
	)
{
	int i;

	matlist[cur_material].d_r = mat->d_r;
	matlist[cur_material].d_g = mat->d_g;
	matlist[cur_material].d_b = mat->d_b;
	matlist[cur_material].d_a = mat->d_a;
	matlist[cur_material].s_r = mat->s_r;
	matlist[cur_material].s_g = mat->s_g;
	matlist[cur_material].s_b = mat->s_b;
	matlist[cur_material].s_a = mat->s_a;
	matlist[cur_material].e_r = mat->e_r;
	matlist[cur_material].e_g = mat->e_g;
	matlist[cur_material].e_b = mat->e_b;
	matlist[cur_material].e_a = mat->e_a;
	for (i = 0; i < cur_material; i++)
	{
		if (( matlist[i].d_r == mat->d_r) &&
			(matlist[i].d_g == mat->d_g) &&
			(matlist[i].d_b == mat->d_b) &&
			(matlist[i].d_a == mat->d_a) &&
			(matlist[i].s_r == mat->s_r) &&
			(matlist[i].s_g == mat->s_g) &&
			(matlist[i].s_b == mat->s_b) &&
			(matlist[i].s_a == mat->s_a) &&
			(matlist[i].e_r == mat->e_r) &&
			(matlist[i].e_g == mat->e_g) &&
			(matlist[i].e_b == mat->e_b) &&
			(matlist[i].e_a == mat->e_a))
		{
			matlist[cur_material].matindex = matlist[i].matindex;
			cur_material++;
			return i;
		}
	}
	matlist[cur_material].matindex = matcount;
	cur_material++;
	matcount++;
	return -1;
}

typedef struct animData {
	char name[80];
	double duration;
	double pvx;
	double pvy;
	double pvz;
	double pvinx;
	double pviny;
	double pvinz;
	double *XPos;
    double *YPos;
	double *ZPos;
    double *XRot;
	double *YRot;
	double *ZRot;
    double *XScl;
	double *YScl;
	double *ZScl;
	animData *next;
} animData;

animData *gAnimPtr = NULL;
animData *lastAnimPtr = NULL;

static void get_duration
	(
	double *duration,
	double *ptr
	)
{
	int numkey;
	double d;
	if (ptr)
	{
		numkey = (int)*ptr;
		d = *(ptr + numkey ) - *(ptr + 1 );
		if (d > *duration)
			*duration = d;
	}
}

static void mx_Identity(double m[4][4])
{
    int     i;
    double * p = &m[0][0];

    for (i = 0; i < 16; ++i)
        *p++ = 0.0;
    for (i = 0; i < 4; ++i)
        m[i][i] = 1.0;
}

static void mx_Rotate(double m[4][4], int axis, double angle)
{
    double cosa, sina, tmp;

    if (angle == 0)
        return;
    cosa = cosf(angle);
    sina = sinf(angle);

    switch (axis)
      {
        case 'x':
        case 'X':
        tmp = m[0][1];
        m[0][1] = cosa * tmp - sina * m[0][2];
        m[0][2] = cosa * m[0][2] + sina * tmp;
        tmp = m[1][1];
        m[1][1] = cosa * tmp - sina * m[1][2];
        m[1][2] = cosa * m[1][2] + sina * tmp;
        tmp = m[2][1];
        m[2][1] = cosa * tmp - sina * m[2][2];
        m[2][2] = cosa * m[2][2] + sina * tmp;
        tmp = m[3][1];
        m[3][1] = cosa * tmp - sina * m[3][2];
        m[3][2] = cosa * m[3][2] + sina * tmp;
        break;

        case 'y':
        case 'Y':
        tmp = m[0][0];
        m[0][0] = cosa * tmp + sina * m[0][2];
        m[0][2] = -sina * tmp + cosa * m[0][2];
        tmp = m[1][0];
        m[1][0] = cosa * tmp + sina * m[1][2];
        m[1][2] = -sina * tmp + cosa * m[1][2];
        tmp = m[2][0];
        m[2][0] = cosa * tmp + sina * m[2][2];
        m[2][2] = -sina * tmp + cosa * m[2][2];
        tmp = m[3][0];
        m[3][0] = cosa * tmp + sina * m[3][2];
        m[3][2] = -sina * tmp + cosa * m[3][2];
        break;

        case 'z':
        case 'Z':
        tmp = m[0][0];
        m[0][0] = cosa * tmp - sina * m[0][1];
        m[0][1] = cosa * m[0][1] + sina * tmp;
        tmp = m[1][0];
        m[1][0] = cosa * tmp - sina * m[1][1];
        m[1][1] = cosa * m[1][1] + sina * tmp;
        tmp = m[2][0];
        m[2][0] = cosa * tmp - sina * m[2][1];
        m[2][1] = cosa * m[2][1] + sina * tmp;
        tmp = m[3][0];
        m[3][0] = cosa * tmp - sina * m[3][1];
        m[3][1] = cosa * m[3][1] + sina * tmp;
        break;
       }
}

static void 
AxisRotation( double mat[4][4], double axis[3], double *ang )    
{
	double tr, s;
	double halfAng;
	int i, j, k;
	double q[4];
	int nxt[] = { 1, 2, 0 };
	
	tr = mat[0][0] + mat[1][1] + mat[2][2];
	if ( tr > 0.0 )
	{
		s = sqrt( tr + 1.0 );
		q[3] = s * 0.5;
		s = 0.5 / s;
		
		q[0] = ( mat[1][2] - mat[2][1] ) * s;
		q[1] = ( mat[2][0] - mat[0][2] ) * s;
		q[2] = ( mat[0][1] - mat[1][0] ) * s;
	} else {
		i = 0;
		if ( mat[1][1] > mat[0][0] ) i = 1;
		if ( mat[2][2] > mat[i][i] ) i = 2;
		j = nxt[i]; k= nxt[j];
				
		s = sqrt( ( mat[i][i] - ( mat[j][j] + mat[k][k] ) ) + 1.0 );
		
		q[i] = s * 0.5;
		s = 0.5 / s;
		
		q[3] = ( mat[j][k] - mat[k][j] ) * s;
		q[j] = ( mat[i][j] + mat[j][i] ) * s;
		q[k] = ( mat[i][k] + mat[k][i] ) * s;
	}
	 
	halfAng = acos( q[3] );
	*ang = 2.0 * halfAng;
	// normalize the vector
	s = sqrt(
			q[0] * q[0] +
			q[1] * q[1] +
			q[2] * q[2]
			);
	if ( s != 0.0 )
	{
		axis[0] = q[0] / s;
		axis[1] = q[1] / s;
		axis[2] = q[2] / s;
	}
	else
	{
		axis[0] = 0.0;
		axis[1] = 0.0;
		axis[2] = 0.0;
	}
}		

// Convenient functions
void
WRITE_SDF( FILE* os, char *sdump )

{
	fprintf( os, "%s%s", indentSpace(), sdump );
}

void
BEGIN_SDF( FILE* os, char *sdump )

{
	WRITE_SDF(os, sdump );
	setIndentation( INDENT );
}

void
END_SDF( FILE* os, char *sdump )
{
	setIndentation( -INDENT );
	WRITE_SDF( os, sdump );
}

void
SDF_SetPrefix( const char *in )
{
	strcpy( prefixStr, in );
}

void
SDF_SetTexRefOutput( int texRef )
{
	writeTexRefs = texRef;
}

void
SDF_SetLightInfo( int lightinfo )
{
	writeLightInfo = lightinfo;
}

void
SDF_SeparateLightMaterialInfo( int lightmaterialinfo )
{
	writeLightMaterialInfo = lightmaterialinfo;
}

void
SDF_HasAnim( int animinfo )
{
	has_anim = animinfo;
}

const char
*SDF_LegalName( const char *in )
{
    static char out[ 80 ];
    int i = 0;

    while ( in[i] != '\0' )
    {
		if ( ( in[i] == '#' ) || ( in[i] == '-' ) || ( in[i] == '/' ) ) 
			out[ i ] = '_';
        else out[ i ] = in[ i ];
        i++;
    }
    out[ i ] = '\0';
    return out;
}

const char
*SDF_ExtractFileName( const char *in )
{
    static char out_file[ 80 ];
    int i = 0, i1 = 0, i2 = 0;

	// locate last unix slash character
	while ( in[i] != '\0' ) 
	{
		if ( ( in[i] == '/' ) || ( in[i] == ':' ) ) i1 = i + 1;
		i++;
	}
	i = i2 = i1;
	// locate last dot character
	while ( in[i] != '\0' ) 
	{
		if ( in[i] == '.' ) i2 = i;
		i++;
	}
		
    if (( i1 == i2 ) && (in[i-1] == '.'))
        i2 = i - 1;
    else if (( i1 == i2 ) && (in[i-1] != '.'))
        i2 = i;

	i = i1;
    while ( i < i2 )
    {
        out_file[ i - i1 ] = in[ i ];
        i++;
    }
    out_file[ i - i1 ] = '\0';
	// fprintf( stderr, "%d, %d, %d = %s - %s\n", i1, i2, i, in, out_file );

    return out_file;
}

/*
**	Checks if the polygon count is zero in a given PolySet
*/
static int
SDF_IsEmptyPolySet( AlPolyset* polyset )
{
	if ( NULL == polyset ) return(1);

	return( polyset->numberOfPolygons() <= 0 );
} 

void setTessPolyType( AlTesselateTypes tp )
{
	tessPolys = tp;
}

void setTessQuality( double tq )
{
	tessQuality = tq;	
}

void setPrintFile( FILE* file )
{
	if( NULL != file)
		printFile = file;
}

void setScriptFile( FILE* file )
{
	if( NULL != file)
		scriptFile = file;
}

char* indentSpace( void )
{
	return indentationBuffer;
}

void setIndentation( int space )
{
	if( space < 0 )
	{
		if( indentation + space < 0 )
			indentation = 0;
		else
			indentation += space;
	}
	else
	{
		if( indentation + space > 255 )
			space = 255 - indentation;
		for( int i = 0; i < space; i++ )
			indentationBuffer[indentation+i] = '\t';
		indentation += space;
	}
	indentationBuffer[indentation] = '\0';
}

/*
** checks to see if the object is defined earlier
** if the object is defined earlier then it will be
** tagged with a prefix'use_' to its name
*/
int
SDF_ObjDefined( AlObject *object, int *is_char, const char *name )
{
	const char* classNam = AlObjectClassName( object );

#if 0
	// if the object is Group or model do share ( for now !! )
	if( !strcmp(classNam, "Group") || !strcmp(classNam, "Model") )
	{
		*is_char = 1;	
		return ( 0 );
	} else *is_char = 0;
#endif

	if ( strlen( name ) < 5 ) return ( 0 );

	if ( ( name[0] == 'u' ) &&
	     ( name[1] == 's' ) &&
	     ( name[2] == 'e' ) &&
	     ( name[3] == '_' ) )	return( 1 );
	else return ( 0 );
}
		
/*
** checks to see if the object is defined earlier
** if the object is defined earlier then it will be
** tagged with a prefix'use_' to its name
*/
int
SDF_AttDefined( AlObject *object, int *is_char, const char *name )
{
	const char* classNam = AlObjectClassName( object );
	char num[80];
	int ret = -1;
	int slen = strlen( name );
	int i = 4;

	if ( slen < 5 ) return ( ret );

	if ( ( name[0] == 'u' ) &&
	     ( name[1] == 's' ) &&
	     ( name[2] == 'e' ) &&
	     ( name[3] == '_' ) )	
	{
		while( i < slen ) 
		{
			num[i-4] = name[i]; 
			if ( name[i] == '_' ) 
			{
				num[i] = '\0';
				ret = atoi( num );	
				return ( ret );
			}
			i++;
		}
		
		return( ret );
	} else return ( ret );
}
		
/*
** Print SDF name&type from Alias object 
** If the object is already defined then the object name
** will be tagged with a special character 'use'. 
** The return value out of this says whether it is
** defined before ( = 1 ) or this is the first time ( = 0 )
*/
int
SDF_PrintObjectIndex( AlObject *object, int *indx , int matflag)
{
	static int nodeNum = 0;
	int obj_defined = -1;
	int is_char = 0;

	if ( NULL == object ) return ( obj_defined );

	const char*	className;
	if ( NULL != (className = AlObjectClassName( object )) )
	{
		const char* name = object->name();
		char temp_name[ 80 ], mod_name[ 80 ];
		const char *new_name;

		if ( NULL != name ) sprintf( temp_name, "%s", name );
		else 
		{
			sprintf( temp_name, "object_%d", nodeNum );
			nodeNum++;
		}

		obj_defined = SDF_AttDefined( object, &is_char, temp_name );

		if ( obj_defined >= 0 ) // object is already defined
		{

			if (!strcmp(className, "MatIndex"))
				obj_defined = Get_M2_Material(obj_defined);
			fprintf( printFile, "%s%s %d\n", indentSpace(), 
								className, 
								obj_defined );
		} else {                  // object is not defined yet
			obj_defined = -1;
			fprintf( printFile, "%s# %s\n", indentSpace(), SDF_LegalName(temp_name)  );
			sprintf( mod_name, "use_%d_%s", *indx, temp_name );	
			object->setName( mod_name );
			(*indx)++;
			if (!matflag)
				fprintf( printFile, "%s{\n", indentSpace()  );
		}
	}

	return ( obj_defined );
}

/*
** Print SDF name&type from Alias object 
** If the object is already defined then the object name
** will be tagged with a special character 'use'. 
** The return value out of this says whether it is
** defined before ( = 1 ) or this is the first time ( = 0 )
*/
int
SDF_PrintObjectName( AlObject *object )
{
	static int nodeNum = 0;
	int obj_defined = 0;
	int is_char = 0;

	if ( NULL == object ) return ( obj_defined );

	const char*	className;
	if ( NULL != (className = AlObjectClassName( object )) )
	{
		const char* name = object->name();
		char temp_name[ 80 ], mod_name[ 80 ];
		const char *new_name;

		if ( NULL != name ) sprintf( temp_name, "%s", name );
		else 
		{
			sprintf( temp_name, "object_%d", nodeNum );
			nodeNum++;
		}

#if 0
		if( (object->type()==20) || (object->type()==32) )  
		fprintf(stdout, "......... %s (0x%x) = 0x%x, %s\n", name, object->name(),
								object, className );
#endif
		obj_defined = SDF_ObjDefined( object, &is_char, temp_name );

		if ( obj_defined ) // object is already defined
		{
			fprintf( printFile, "%sUse %s %s%s\n", indentSpace(), 
								className, 
								prefixStr,
								SDF_LegalName( &temp_name[4] ) );
		} else {                  // object is not defined yet
#if 0
			if( !is_char )
			{
				sprintf( mod_name, "use_%s", temp_name );	
				object->setName( mod_name );
				fprintf( printFile, "%sDefine %s %s {\n", indentSpace(), 
								className, 
								SDF_LegalName ( temp_name ) );
			} else
			fprintf( printFile, "%s %s {\n", indentSpace(), 
								className );
#else
			sprintf( mod_name, "use_%s", temp_name );	
			object->setName( mod_name );
			// for some reason setName sometimes sets a different name !!
			new_name = object->name();
			fprintf( printFile, "%sDefine %s %s%s {\n", indentSpace(), 
								className, 
								prefixStr,
								SDF_LegalName ( &new_name[4] ) );
#endif
		}
	}

	return ( obj_defined );
}

/*
** Get the number of AlObjects that can be mapped to
** ascii SDF. If the number is non-zero then write the
** top-level AlGroup node into SDF file.	
** This function will returns one if there is a SDF 
** object else zero
*/
void
SDF_NumObjects( AlObject *object, int *num_sdf_objs )
{
	AlDagNode *child;
	AlGroupNode *group_node;

	if ( NULL == object ) return;

	switch ( object->type() )
	{
#if 0
	  case kCameraUpType: /* perspectiveCamera */
#endif

	  case kFaceNodeType:
	  case kSurfaceNodeType: /* NURBS Surface - Model */
	  case kPolysetNodeType: /* Polygon set   - Model */
        (*num_sdf_objs) += 1;
        break;

	  case kLightNodeType: /* Light node   - Light */
		if (writeLightInfo)
			(*num_sdf_objs) += 1;
		break;

	  case kGroupNodeType:   /* Alias group - Group */
		// if it is a group node then recurse
		group_node = object->asGroupNodePtr();
		if ( group_node == NULL ) return;

		child = group_node->childNode();
		while ( ( child != NULL ) && ( (*num_sdf_objs) == 0 ) ) 
		{
			SDF_NumObjects( child, num_sdf_objs );
			child = child->nextNode();
		}
		break;
      default:
		break;
	}
}

#if 0
/*
**	Map Alias object type to SDF object type
*/
const char*	
AlObjectClassName( AlObject *object )
{
	if ( NULL == object ) return NULL;

	const char *className = NULL;
	switch ( object->type() )
	{
	  case kAmbientLightType:
		className = "AlAmbientLight";
		break;
	  case kAreaLightType:
		className = "AlAreaLight";
		break;
	  case kCameraEyeType:
	  case kCameraViewType:
	  case kCameraUpType:
		className = "perspectiveCamera";
		break;
	  case kCameraType:
		className = "AlCamera";
		break;
	  case kClusterType:
		className = "AlCluster";
		break;
	  case kClusterNodeType:
		className = "AlClusterNode";
		break;
	  case kClusterMemberType:
		className = "AlClusterMember";
		break;
	  case kCurveNodeType:
		className = "AlCurveNode";
		break;
	  case kCurveType:
		className = "AlCurve";
		break;
	  case kCurveCVType:
		className = "AlCurveCV";
		break;
	  case kCurveOnSurfaceType:
		className = "AlCurveOnSurface";
		break;
	  case kDagNodeType:
		className = "AlDagNode";
		break;
	  case kDirectionLightType:
		className = "AlDirectionLight";
		break;
	  case kFaceNodeType:
		className = "Model";
		break;
	  case kFaceType:
		className = "AlFace";
		break;
	  case kGroupNodeType:
		className = "Group";
		break;
	  case kLightLookAtNodeType:
		className = "AlLightLookAtNode";
		break;
	  case kLightNodeType:
		className = "AlLightNode";
		break;
	  case kLightType:
		className = "AlLight";
		break;
	  case kLightUpNodeType:
		className = "AlLightUpNode";
		break;
	  case kLinearLightType:
		className = "AlLinearLight";
		break;
	  case kNonAmbientLightType:
		className = "AlNonAmbientLight";
		break;
	  case kPointLightType:
		className = "AlPointLight";
		break;
	  case kSpotLightType:
		className = "AlSpotLight";
		break;
	  case kSurfaceNodeType:
		className = "Model";
		break;
	  case kSurfaceType:
		className = "AlSurface";
		break;
	  case kSurfaceCVType:
		className = "AlSurfaceCV";
		break;
	  case kSetType:
		className = "AlSet";
		break;
	  case kSetMemberType:
		className = "AlSetMember";
		break;
	  case kChannelType:
		className = "AlChannel";
		break;
	  case kActionType:
		className = "AlAction";
		break;
	  case kMotionActionType:
		className = "AlMotionAction";
		break;
	  case kParamActionType:
		className = "AlParamAction";
		break;
	  case kKeyframeType:
		className = "AlKeyframe";
		break;
	  case kStreamType:
		className = "AlStream";
		break;
	  case kShaderType:
		className = "MatIndex";
		break;
	  case kTextureType:
		className = "TexIndex";
		break;
	  case kEnvironmentType:
		className = "AlEnvironment";
		break;
	  case kPolysetNodeType:
		className = "Model";  //AlPolysetNode
		break;
	  case kPolysetType:
		className = "AlPolyset";
		break;
	  case kPolygonType:
		className = "AlPolygon";
		break;
	  case kPolysetVertexType:
		className = "AlPolysetVertex";
		break;
	  case kAttributeType:
		className = "AlAttribute";
		break;
	  case kArcAttributeType:
		className = "AlArcAttribute";
		break;
	  case kLineAttributeType:
		className = "AlLineAttribute";
		break;
	  case kCurveAttributeType:
		className = "AlCurveAttribute";
		break;
	  case kPlaneAttributeType:
		className = "AlPlaneAttribute";
		break;
	  case kConicAttributeType:
		className = "AlConicAttribute";
		break;
	  case kRevSurfAttributeType:
		className = "AlRevSurfAttribute";
		break;
	  case kJointNodeType:
		className = "AlJointNode";
		break;
	  case kIKConstraintType:
		className = "AlIKConstraint";
		break;
	  case kIKPointConstraintType:
		className = "AlIKPointConstraint";
		break;
	  case kIKOrientationConstraintType:
		className = "AlIKOrientationConstraint";
		break;
	  case kIKAimConstraintType:
		className = "AlIKAimConstraint";
		break;
	  default:
		className = "unknownClassName";
		break;
	}
	return className;
}
#else
/*
**	Map Alias object type to SDF object type
*/
const char*	
AlObjectClassName( AlObject *object )
{
	if ( NULL == object ) return NULL;

	static char className[80];
	switch ( object->type() )
	{
	  case kAmbientLightType:
		/* strcpy( className, "AlAmbientLight"); */
		strcpy( className, "Light");
		break;
	  case kAreaLightType:
		/* strcpy( className, "AlAreaLight"); */
		strcpy( className, "Light");
		break;
	  case kCameraEyeType:
	  case kCameraViewType:
	  case kCameraUpType:
		strcpy( className, "perspectiveCamera");
		break;
	  case kCameraType:
		strcpy( className, "AlCamera");
		break;
	  case kClusterType:
		strcpy( className, "AlCluster");
		break;
	  case kClusterNodeType:
		strcpy( className, "AlClusterNode");
		break;
	  case kClusterMemberType:
		strcpy( className, "AlClusterMember");
		break;
	  case kCurveNodeType:
		strcpy( className, "AlCurveNode");
		break;
	  case kCurveType:
		strcpy( className, "AlCurve");
		break;
	  case kCurveCVType:
		strcpy( className, "AlCurveCV");
		break;
	  case kCurveOnSurfaceType:
		strcpy( className, "AlCurveOnSurface");
		break;
	  case kDagNodeType:
		strcpy( className, "AlDagNode");
		break;
	  case kDirectionLightType:
		/* strcpy( className, "AlDirectionLight"); */
		strcpy( className, "Light");
		break;
	  case kFaceNodeType:
		strcpy( className, "Model");
		break;
	  case kFaceType:
		strcpy( className, "AlFace");
		break;
	  case kGroupNodeType:
		strcpy( className, "Group");
		break;
	  case kLightLookAtNodeType:
		strcpy( className, "AlLightLookAtNode");
		break;
	  case kLightNodeType:
		/* strcpy( className, "AlLightNode"); */
		strcpy( className, "Light");
		break;
	  case kLightType:
		strcpy( className, "AlLight");
		break;
	  case kLightUpNodeType:
		strcpy( className, "AlLightUpNode");
		break;
	  case kLinearLightType:
		/* strcpy( className, "AlLinearLight"); */
		strcpy( className, "Light");
		break;
	  case kNonAmbientLightType:
		/* strcpy( className, "AlNonAmbientLight"); */
		strcpy( className, "Light");
		break;
	  case kPointLightType:
		/* strcpy( className, "AlPointLight"); */
		strcpy( className, "Light");
		break;
	  case kSpotLightType:
		/* strcpy( className, "AlSpotLight"); */
		strcpy( className, "Light");
		break;
	  case kSurfaceNodeType:
		strcpy( className, "Model");
		break;
	  case kSurfaceType:
		strcpy( className, "AlSurface");
		break;
	  case kSurfaceCVType:
		strcpy( className, "AlSurfaceCV");
		break;
	  case kSetType:
		strcpy( className, "AlSet");
		break;
	  case kSetMemberType:
		strcpy( className, "AlSetMember");
		break;
	  case kChannelType:
		strcpy( className, "AlChannel");
		break;
	  case kActionType:
		strcpy( className, "AlAction");
		break;
	  case kMotionActionType:
		strcpy( className, "AlMotionAction");
		break;
	  case kParamActionType:
		strcpy( className, "AlParamAction");
		break;
	  case kKeyframeType:
		strcpy( className, "AlKeyframe");
		break;
	  case kStreamType:
		strcpy( className, "AlStream");
		break;
	  case kShaderType:
		strcpy( className, "MatIndex");
		break;
	  case kTextureType:
		strcpy( className, "TexIndex");
		break;
	  case kEnvironmentType:
		strcpy( className, "AlEnvironment");
		break;
	  case kPolysetNodeType:
		strcpy( className, "Model");  //AlPolysetNode
		break;
	  case kPolysetType:
		strcpy( className, "AlPolyset");
		break;
	  case kPolygonType:
		strcpy( className, "AlPolygon");
		break;
	  case kPolysetVertexType:
		strcpy( className, "AlPolysetVertex");
		break;
	  case kAttributeType:
		strcpy( className, "AlAttribute");
		break;
	  case kArcAttributeType:
		strcpy( className, "AlArcAttribute");
		break;
	  case kLineAttributeType:
		strcpy( className, "AlLineAttribute");
		break;
	  case kCurveAttributeType:
		strcpy( className, "AlCurveAttribute");
		break;
	  case kPlaneAttributeType:
		strcpy( className, "AlPlaneAttribute");
		break;
	  case kConicAttributeType:
		strcpy( className, "AlConicAttribute");
		break;
	  case kRevSurfAttributeType:
		strcpy( className, "AlRevSurfAttribute");
		break;
	  case kJointNodeType:
		strcpy( className, "AlJointNode");
		break;
	  case kIKConstraintType:
		strcpy( className, "AlIKConstraint");
		break;
	  case kIKPointConstraintType:
		strcpy( className, "AlIKPointConstraint");
		break;
	  case kIKOrientationConstraintType:
		strcpy( className, "AlIKOrientationConstraint");
		break;
	  case kIKAimConstraintType:
		strcpy( className, "AlIKAimConstraint");
		break;
	  default:
		strcpy( className, "unknownClassName");
		break;
	}
	return className;
}

#endif

AlTexture*
SDF_TexturesExist( 
	AlShader *shader 
	)
{
	AlTexture	*texture = shader->firstTexture();

	if ( texture ) return( texture );

	return ( NULL );
}


/*
** Print all the materials and textures at the top of the file
** Reference these attributes in individual models through indices
*/
void
SDF_PrintAlShaders( char *fname )
{
	char	buf[120];
	AlShader		*shader, *tmpshader;
	int 	shadercount = 0;
	int count = 0, defined;
	AlTexture *textures_exist = NULL;
	FILE *oldfp;

	// extract top node name from the filename
	strcpy( topMdlName, SDF_ExtractFileName( fname ) );

	// find if the materials exist for this file
	count = 0;
	shader = AlUniverse::firstShader();
	if (shader != NULL)
	{
		if (writeLightMaterialInfo)
		{
			sprintf(buf, "include \"%s.mat\"\n", topMdlName);
			WRITE_SDF( printFile, buf );
			
			oldfp = printFile;
			sprintf(buf, "%s.mat", topMdlName);
			if ((printFile = fopen(buf, "w")) == (FILE *) NULL)
			{
				fprintf(stderr, "ERROR: Cannot open file %d\n", buf);
				exit(0);
			}
			fprintf( printFile, "SDFVersion 0.1\n" );
		}

		sprintf( buf, "Define MatArray %s_materials {\n", topMdlName );
		BEGIN_SDF( printFile, buf );
		
		tmpshader = AlUniverse::firstShader();
		shadercount = 0;
		while (tmpshader != NULL)
		{
			tmpshader = AlUniverse::nextShader(tmpshader);
			shadercount++;
		}
		CreateMaterialList(shadercount);
		while (shader != NULL)
		{
			defined = SDF_PrintAlMatData(shader, &count);
			
			shader = AlUniverse::nextShader(shader);
		}
		END_SDF( printFile, "}\n" );
		
		if (writeLightMaterialInfo)
		{
			fclose(printFile);
			printFile = oldfp;
		}
	}

	// find if the textures exist for this file
	shader = AlUniverse::firstShader();
	while ( ( shader != NULL ) && ( textures_exist == NULL ) )
	{
		textures_exist = SDF_TexturesExist(shader);
		shader = AlUniverse::nextShader(shader);
	}

	if( (textures_exist) && ( writeTexRefs ) )
	{
		if (writeLightMaterialInfo)
		{
			sprintf(buf, "include \"%s.tex\"\n", topMdlName);
			WRITE_SDF( printFile, buf );
			
			oldfp = printFile;
			sprintf(buf, "%s.tex", topMdlName);
			if ((printFile = fopen(buf, "w")) == (FILE *) NULL)
			{
				fprintf(stderr, "ERROR: Cannot open file %d\n", buf);
				exit(0);
			}
			fprintf( printFile, "SDFVersion 0.1\n" );
		}

		sprintf( buf, "Define TexArray %s_textures {\n", topMdlName );
		BEGIN_SDF( printFile, buf );
		
		count = 0;
		shader = AlUniverse::firstShader();
		while (shader != NULL)
		{
			defined = SDF_PrintAlTexData(shader, &count);
			
			shader = AlUniverse::nextShader(shader);
		}
		END_SDF( printFile, "}\n" );
		
		if (writeLightMaterialInfo)
		{
			fclose(printFile);
			printFile = oldfp;
		}
	}
}

/*
** Start dumping out the Alias Universe
*/
void 
printAlUniverse( char *fname )
{
	AlDagNode *child;
	char top_node_name[80], buf[120];

	fprintf( printFile, "SDFVersion 0.1\n" );

	// print all the textures and materials
	SDF_PrintAlShaders( fname );

	if( strlen( prefixStr ) == 0 )
		sprintf( top_node_name, "%s_world", topMdlName );
	else sprintf( top_node_name, "%s_world", prefixStr );

	sprintf( buf, "Define Group %s {\n", top_node_name );
	BEGIN_SDF( printFile, buf );
	child = AlUniverse::firstDagNode();
	while ( child != NULL ) 
	{
		printAlObject( child );
		child = child->nextNode();
	}
	END_SDF( printFile, "}\n" );
}

/*
** Print Alias object to SDF object
*/
void 
printAlObject( AlObject *object )
{
	if ( NULL == object ) return;

    switch ( object->type() )
	{

#if 0 
	  case kCameraType:
		printAlCamera( object->asCameraPtr() );
		break;

	  case kCameraEyeType:
	  case kCameraViewType:
	  case kCameraUpType:
		printAlCameraNode( object->asCameraNodePtr() );
		break;

	  case kClusterMemberType:
		AlClusterMember*	clusterMember = object->asClusterMemberPtr();
		break;

	  case kClusterType:
		AlCluster*	cluster = object->asClusterPtr();
		break;

	  case kClusterNodeType:
		printAlClusterNode( object->asClusterNodePtr() );
		break;

	  case kCurveType:
		printAlCurve( object->asCurvePtr() );
		break;
	  case kCurveNodeType:
		printAlCurveNode( object->asCurveNodePtr() );
		break;

	  case kCurveOnSurfaceType:
		printAlCurveOnSurface( object->asCurveOnSurfacePtr() );
		break;
#endif

	  case kFaceType:
		fprintf(stderr, "kFaceType OBJECT\n");
		break;

	  case kFaceNodeType:
		SDF_PrintAlFaceNode( object->asFaceNodePtr() );
		break;

	  case kDagNodeType:
		fprintf(printFile, "kDagNodeType\n");
		printAlDagNode( object->asDagNodePtr() );
		break;

	  case kGroupNodeType:
		SDF_PrintAlGroupNode( object->asGroupNodePtr() );
		break;

#if TAI_DEBUG
	  case kLightLookAtNodeType:
	  case kLightNodeType:
	  case kLightUpNodeType:
		printAlLightNode( object->asLightNodePtr() );
		break;

	  case kLightType:
	  case kNonAmbientLightType:
		break;

	  case kAmbientLightType:
		printAlAmbientLight( object->asAmbientLightPtr() );
		break;

	  case kPointLightType:
		printAlPointLight( object->asPointLightPtr() );
		break;

	  case kDirectionLightType:
		printAlDirectionLight( object->asDirectionLightPtr() );
		break;

	  case kSpotLightType:
		printAlSpotLight( object->asSpotLightPtr() );
		break;

	  case kLinearLightType:
		printAlLinearLight( object->asLinearLightPtr() );
		break;

	  case kAreaLightType:
		printAlAreaLight( object->asAreaLightPtr() );
		break;

	  case kSetType:
		printAlSetName( object->asSetPtr() );
		break;

	  case kSetMemberType:
		break;

#endif
	  case kActionType:
	  case kParamActionType:
	  case kMotionActionType:
		printAlAction(object->asActionPtr());
		break;

	  case kChannelType:
		printAlChannel(object->asChannelPtr());
		break;


	  case kSurfaceType:
		{
		fprintf(stderr, "kSurfaceType OBJECT\n");
		AlSurface* surface = object->asSurfacePtr();
		printAlSurface( surface );
		if( surface->trimmed() )
			printAlSurfaceTrimRegions( surface );
		}
		break;

	  case kPolysetNodeType:
		SDF_PrintAlPolysetNode( object->asPolysetNodePtr() );
		break;

	  case kSurfaceNodeType:
		SDF_PrintAlSurfaceNode( object->asSurfaceNodePtr() );
		break;

	  default:
		break;
	}
}

/*
** Print Alias DAG node as a SDF group Character
** Print the local transformation matrix of this group 
** return value indicates whether it is defined earlier or not
*/
int 
SDF_PrintAlDagNodeInfo( AlDagNode *dagNode )
{
	int obj_defined = 1;

	if ( NULL == dagNode ) return ( obj_defined );

	obj_defined = SDF_PrintObjectName( dagNode );

	setIndentation( INDENT );
	
	if (has_anim)
	{
	
/* #if TAI_DEBUG */
#if 1
    //  Print the display state of the dag Node  
    //
    if( dagNode->isDisplayModeSet( kDisplayModeInvisible ) ) {
        fprintf( printFile, "%sDag Node \"%s\" is invisible\n",
												indentSpace(), dagNode->name());
    }
    if( dagNode->isDisplayModeSet( kDisplayModeBoundingBox ) ) {
        fprintf( printFile, "%sDag Node \"%s\" is bounding boxed\n",
												indentSpace(), dagNode->name());
    }
    if( dagNode->isDisplayModeSet( kDisplayModeTemplate ) ) {
        fprintf( printFile, "%sDag Node \"%s\" is templated\n",
											indentSpace(), dagNode->name());
    }

	if( dagNode->isDisplayModeSet( kDisplayModeQuickWire ) ) {
		fprintf( printFile, "%sDag Node  \"%s\" is quick wired\n",
											indentSpace(), dagNode->name());
	}

	// If the dag node is bounding boxed, print out the corners of the
	// bounding box
	//
    if( dagNode->isDisplayModeSet( kDisplayModeBoundingBox ) ) {
        double corners[8][4];
        dagNode->boundingBox( corners );
        fprintf(printFile, "%sBounding box is:\n", indentSpace() );

        for ( int i = 0; i < 8; i++ ) {
                    fprintf(printFile, "%s   %g, %g, %g\n", indentSpace(),  
											corners[i][0],
                                            corners[i][1],
                                            corners[i][2]);
        }
    }

	// Print the clusters that this dag node is in (if any)
	//
	AlCluster *cluster = dagNode->firstCluster();
	if (cluster != NULL)
	{
		fprintf(printFile, "%sCLUSTERS this dag node is in:\n", indentSpace());
		setIndentation( INDENT );
		do {
			fprintf(printFile, "%scluster: %s\n",
						indentSpace(), cluster->clusterNode()->name());
		} while (cluster = dagNode->nextCluster(cluster));
		setIndentation( -INDENT );
	}

	// Print the animation on this dag node (if any)
	//
	AlChannel *channel = dagNode->firstChannel();
    double **ptr;
    double *XPos = NULL, *YPos = NULL, *ZPos = NULL;
    double *XRot = NULL, *YRot = NULL, *ZRot = NULL;
    double *XScl = NULL, *YScl = NULL, *ZScl = NULL;
    double duration;
    int numkey;
	if (channel != NULL)
	{
#if 0
		fprintf(printFile, "# making animation data\n");
#endif
		do {

			const char	*className	= AlObjectClassName(channel);
			AlObject	*item		= channel->animatedItem();
			const char	*field_name	= channel->parameterName();
			int			field		= channel->parameter();
            AlAction    *action;
   
            for (int i = channel->numAppliedActions(); i >= 1; i--)
            {
                action = channel->appliedAction(i);
                if (!strcmp(field_name, "X Translate"))
                {
#if 0
                    fprintf(printFile, "# This channel is X Translate\n");
#endif
                    ptr = &XPos;
                }
                else if (!strcmp(field_name, "Y Translate"))
                {
#if 0
                    fprintf(printFile, "# This channel is Y Translate\n");
#endif
                    ptr = &YPos;
                }
                else if (!strcmp(field_name, "Z Translate"))
                {
#if 0
                    fprintf(printFile, "# This channel is Z Translate\n");
#endif
                    ptr = &ZPos;
                }
                else if (!strcmp(field_name, "X Rotate"))
                {
#if 0
                    fprintf(printFile, "# This channel is X Rotate\n");
#endif
                    ptr = &XRot;
                }
                else if (!strcmp(field_name, "Y Rotate"))
                {
#if 0
                    fprintf(printFile, "# This channel is Y Rotate\n");
#endif
                    ptr = &YRot;
                }
                else if (!strcmp(field_name, "Z Rotate"))
                {
#if 0
                    fprintf(printFile, "# This channel is Z Rotate\n");
#endif
                    ptr = &ZRot;
                }
                else if (!strcmp(field_name, "X Scale"))
                {
#if 0
                    fprintf(printFile, "# This channel is X Scale\n");
#endif
                    ptr = &XScl;
                }
                else if (!strcmp(field_name, "Y Scale"))
                {
#if 0
                    fprintf(printFile, "# This channel is Y Scale\n");
#endif
                    ptr = &YScl;
                }
                else if (!strcmp(field_name, "Z Scale"))
                {
#if 0
                    fprintf(printFile, "# This channel is Z Scale\n");
#endif
                    ptr = &ZScl;
                }
                else
                {
#if 0
                    fprintf(printFile, "# other channel: %s\n", field_name);
#endif
                }
                if (action->type() == kMotionActionType)
                    printAlAnimMotionAction(action->asMotionActionPtr());
                else
                    printAlAnimParamAction(ptr, action->asParamActionPtr());
            }
			/*
			printAlObject( channel );
			fprintf(printFile, "%schannel: (", indentSpace());
			fprintf(printFile, "%s [%s], ",
									item->name(), AlObjectClassName(item));
			fprintf(printFile, "%s [%d] )\n", field_name, field);
			*/
			/* printAlObject( item ); */

		} while (channel = dagNode->nextChannel(channel));
        duration = 0;
        get_duration(&duration, XPos);
        get_duration(&duration, YPos);
        get_duration(&duration, ZPos);
        get_duration(&duration, XRot);
        get_duration(&duration, YRot); 
        get_duration(&duration, ZRot);
        get_duration(&duration, XScl);
        get_duration(&duration, YScl);
        get_duration(&duration, ZScl);
		/*
        fprintf(printFile, "%sDuration %g\n", indentSpace(), duration);
        if ((XPos) && (YPos) && (ZPos))
        {       
            numkey = *XPos;
            BEGIN_SDF( printFile,  "PosFrames {\n" );
            for (int i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g\n", indentSpace(), *(XPos + i + 1));
            END_SDF( printFile,  "}\n" );
            BEGIN_SDF( printFile,  "PosData {\n" );
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g %g %g\n", indentSpace(),
                                    *(XPos + i + 1 + numkey),
                                    *(YPos + i + 1 + numkey),
                                    *(ZPos + i + 1 + numkey));
            END_SDF( printFile,  "}\n" );
        }
        if ((XRot) && (YRot) && (ZRot))
		{
			double mat[4][4], angle, axis[3], ang;
			numkey = *XRot;
			BEGIN_SDF( printFile,  "RotFrames {\n" );
			for (int i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g\n", indentSpace(), *(XRot + i + 1));
			END_SDF( printFile,  "}\n" );
			BEGIN_SDF( printFile,  "RotData {\n" );
			for (i = 0; i < numkey; i++)
			{
				mx_Identity(mat);
				if (i == 0)
				{
					angle = *(XRot + i + 1 + numkey);
					mx_Rotate(mat, 'x', angle/PI);
					angle = *(YRot + i + 1 + numkey);
					mx_Rotate(mat, 'y', angle/PI);
					angle = *(ZRot + i + 1 + numkey);
					mx_Rotate(mat, 'z', angle/PI);
				}
				else
				{
					angle = *(XRot + i + 1 + numkey) - *(XRot + i + numkey);
					mx_Rotate(mat, 'x', angle/PI);
					angle = *(YRot + i + 1 + numkey) - *(YRot + i + numkey);
					mx_Rotate(mat, 'y', angle/PI);
					angle = *(ZRot + i + 1 + numkey) - *(ZRot + i + numkey);
					mx_Rotate(mat, 'z', angle/PI);
				}
				AxisRotation(mat, axis, &ang )    
				fprintf(printFile, "%s%g %g %g %g\n", indentSpace(),
									axis[0], axis[1], axis[2], ang);
			}
			END_SDF( printFile,  "}\n" );
		}
		if ((XScl) && (YScl) && (ZScl))
		{
			numkey = *XScl;
			BEGIN_SDF( printFile,  "SclFrames {\n" );
			for (int i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g\n", indentSpace(), *(XScl + i + 1));
			END_SDF( printFile,  "}\n" );
			BEGIN_SDF( printFile,  "SclData {\n" );
			for (i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g %g %g\n", indentSpace(),
									*(XScl + i + 1 + numkey),
									*(YScl + i + 1 + numkey),
									*(ZScl + i + 1 + numkey));
			END_SDF( printFile,  "}\n" );
		}
		*/
	}
	channel = dagNode->firstChannel();
	if (channel)
	{
		animData *anim;
		int len, i;
		const char *nname;
		AlTM imat;
		double v1[4];
		AlDagNode* parent;

		anim = (animData *)malloc(sizeof(animData));
		nname = dagNode->name();
		strcpy(anim->name, prefixStr);
		strcat(anim->name, &nname[4]);
		len = strlen(anim->name);
		for (i = 0; i < len; i++)
			if (anim->name[i] == '#')
				anim->name[i] = '_';
		dagNode->inverseGlobalTransformationMatrix(imat);
		dagNode->rotatePivot(anim->pvx, anim->pvy, anim->pvz);
		v1[0] = anim->pvx;
		v1[1] = anim->pvy;
		v1[2] = anim->pvz;
		v1[3] = 1.0;
		fprintf( printFile, "# %s vec1 (%g,%g,%g,%g)\n", indentSpace(), 
							v1[0], v1[1], v1[2], v1[3]);
		imat.transPoint(v1);
		fprintf( printFile, "# %s vec2 (%g,%g,%g,%g)\n", indentSpace(), 
							v1[0], v1[1], v1[2], v1[3]);
		anim->pvx = v1[0];
		anim->pvy = v1[1];
		anim->pvz = v1[2];
	
		dagNode->rotatePivotIn(v1[0], v1[1], v1[2]);
		fprintf( printFile, "# %s in vec1 (%g,%g,%g,%g)\n", indentSpace(), 
							v1[0], v1[1], v1[2], v1[3]);
		imat.transPoint(v1);
		fprintf( printFile, "# %s in vec2 (%g,%g,%g,%g)\n", indentSpace(), 
							v1[0], v1[1], v1[2], v1[3]);
		
		dagNode->rotatePivotOut(v1[0], v1[1], v1[2]);
		anim->pvinx = v1[0];
		anim->pviny = v1[1];
		anim->pvinz = v1[2];
		fprintf( printFile, "# %s out vec1 (%g,%g,%g,%g)\n", indentSpace(), 
							v1[0], v1[1], v1[2], v1[3]);
		imat.transPoint(v1);
		fprintf( printFile, "# %s out vec2 (%g,%g,%g,%g)\n", indentSpace(), 
							v1[0], v1[1], v1[2], v1[3]);

		parent = dagNode->parentNode();
		if (parent)
		{
			v1[3] = 1.0;
			parent->rotatePivot(v1[0], v1[1], v1[2]);
			fprintf( printFile, "# %s parent in child vec1 (%g,%g,%g,%g)\n", 
				indentSpace(), v1[0], v1[1], v1[2], v1[3]);
			imat.transPoint(v1);
			fprintf( printFile, "# %s parent in child vec2 (%g,%g,%g,%g)\n", 
				indentSpace(), v1[0], v1[1], v1[2], v1[3]);

			parent->inverseGlobalTransformationMatrix(imat);
			v1[3] = 1.0;
			parent->rotatePivot(v1[0], v1[1], v1[2]);
			fprintf( printFile, "# %s parent vec1 (%g,%g,%g,%g)\n", 
				indentSpace(), v1[0], v1[1], v1[2], v1[3]);
			imat.transPoint(v1);
			fprintf( printFile, "# %s parent vec2 (%g,%g,%g,%g)\n", 
				indentSpace(), v1[0], v1[1], v1[2], v1[3]);
		}

		anim->duration = duration;
		anim->XPos = XPos;
		anim->YPos = YPos;
		anim->ZPos = ZPos;
		anim->XRot = XRot;
		anim->YRot = YRot;
		anim->ZRot = ZRot;
		anim->XScl = XScl;
		anim->YScl = YScl;
		anim->ZScl = ZScl;
		anim->next = NULL;
		if (!gAnimPtr)
		{
			gAnimPtr = anim;
			lastAnimPtr = anim;
		}
		else
		{
			lastAnimPtr->next = anim;
			lastAnimPtr = anim;
		}
	}
#endif

	}
	// Print the local transformation matrices 
  	if ( !obj_defined )
	{ 
    	double matrix[4][4];
    	int    i;
    	dagNode->localTransformationMatrix( matrix );
	
		BEGIN_SDF( printFile,  "Transform {\n" );
    	for( i  = 0; i < 4; i ++ ) {
       	 fprintf(printFile, "%s%g %g %g %g\n", indentSpace(),
                        matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3]);
    	}
		END_SDF( printFile, "}\n" );
	}

#if 0
    dagNode->globalTransformationMatrix( matrix );
    fprintf(printFile, "%sGlobal Transform is: \n", indentSpace());
    for( i  = 0; i < 4; i ++ ) {
        fprintf(printFile, "%s%g, %g, %g, %g\n", indentSpace(),
                        matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3]);
    }
	AlJointNode*	jointNode = dagNode->jointNode();
	if ( jointNode == NULL)
		fprintf(printFile, "%sDag node has no jointNode.\n", indentSpace());
	else
		printAlJointNode(jointNode);

#endif

	setIndentation( -INDENT );

	return ( obj_defined );
}

/*
** Print Alias DAG node as a SDF character
*/
void 
printAlDagNode( AlDagNode *dagNode )
{
	if ( NULL == dagNode ) return;

	SDF_PrintAlDagNodeInfo( dagNode );
	// End define object
	fprintf( printFile, "%s}\n ", indentSpace() );
}

/*
** Print Alias group node as a SDF character with bunch of
** children as characters
*/
void 
SDF_PrintAlGroupNode( AlGroupNode *groupNode )
{
	int num_sdf_objects = 0;
	int obj_defined;

	if ( NULL == groupNode ) return;

	// Recursively find if there are any SDF objects in
	// Alias hierarchy
	SDF_NumObjects( (AlObject *)groupNode, &num_sdf_objects );

#if 0
	fprintf( stderr, "Number of SDF objects in group %s = %d\n", 
			AlObjectClassName( groupNode ), num_sdf_objects );
#endif

	if ( num_sdf_objects == 0 ) return;

	// Print define object
	obj_defined = SDF_PrintAlDagNodeInfo( groupNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	AlDagNode *child = groupNode->childNode();
	while ( child != NULL ) {
		printAlObject( child );
		child = child->nextNode();
	}
	setIndentation( -INDENT );

	// End define object 
	fprintf( printFile, "%s}\n ", indentSpace() );

	// Recurse across to print siblings
    // printAlObject( groupNode->nextNode() );
}

void printAlSurface( AlSurface *surface )
{
	if ( NULL == surface ) return;

#ifdef DEBUG
	int		 i, j, k;
	int		 numUKnots;
	int		 numVKnots;
	double	*uKnots;
	double	*vKnots;
	int		 numUCvs;
	int		 numVCvs;
	int		 numUSpans;
	int		 numVSpans;
	double	*cvs;
	int		*uMult;
	int		*vMult;
	char	*form;
    AlSurfaceCV *surfCVU;
    AlSurfaceCV *surfCVV;
#endif

	SDF_PrintObjectName( surface );
	setIndentation( INDENT );

#ifdef DEBUG
	printAttributes( surface->firstAttribute() );

	// Print out the render info on this surface
	//
    AlRenderInfo renderInfo;
    if( surface->renderInfo( renderInfo ))
	{
		fprintf(printFile, "%sSurface is %sDOUBLE SIDED.\n", indentSpace(),
				(renderInfo.doubleSided ? "" : "not "));
		fprintf(printFile, "%sSurface is %sopposite.\n", indentSpace(),
				(renderInfo.opposite ? "" : "not "));
		fprintf(printFile, "%sSurface will %scast shadows.\n", indentSpace(),
				(renderInfo.castsShadow ? "" : "not "));
    }

	fprintf(printFile, "%sSurface is %strimmed.\n", indentSpace(),
		(surface->trimmed() ? "" : "not "));

	fprintf(printFile, "%sU degree = %d\n", indentSpace(), surface->uDegree());
	fprintf(printFile, "%sV degree = %d\n", indentSpace(), surface->vDegree());

	if ( kOpen == surface->uForm() )
		form = "open";
	else if ( kClosed == surface->uForm() )
		form = "closed";
	else
		form = "periodic";
	fprintf( printFile, "%sU form   = %s\n", indentSpace(), form );

	if ( kOpen == surface->vForm() )
		form = "open";
	else if ( kClosed == surface->vForm() )
		form = "closed";
	else
		form = "periodic";
	fprintf( printFile, "%sV form   = %s\n", indentSpace(), form );

	numUSpans = surface->uNumberOfSpans();
	numVSpans = surface->vNumberOfSpans();
	numUKnots = surface->realuNumberOfKnots();
	numVKnots = surface->realvNumberOfKnots();
	numUCvs   = surface->uNumberOfCVs();
	numVCvs   = surface->vNumberOfCVs();

    fprintf( printFile, "%s# of Spans U: %d V: %d\n",
                        indentSpace(), numUSpans, numVSpans);
    fprintf( printFile, "%s# of Knots U: %d V: %d\n",
                        indentSpace(), numUKnots, numVKnots);
    fprintf( printFile, "%s# of CVs   U: %d V: %d\n",
                        indentSpace(), numUCvs, numVCvs);

	// Print out the knot vectors
	//
	uKnots = new double [ numUKnots ];
	vKnots = new double [ numVKnots ];
    surface->realuKnotVector( uKnots );
    surface->realvKnotVector( vKnots );

	fprintf( printFile, "%sU knots  =", indentSpace() );
	for ( i = 0; i < numUKnots; i++ )
	{
		fprintf( printFile, " %g", uKnots[i] );

	}
	fprintf( printFile, "\n" );

	fprintf( printFile, "%sV knots  =", indentSpace() );
	for ( i = 0; i < numVKnots; i++ )
	{
		fprintf( printFile, " %g", vKnots[i] );
	}
	fprintf( printFile, "\n" );

	delete uKnots;
	delete vKnots;

	// Print out the multiplicities
	//
	cvs   = new double [ 4 * numUCvs * numVCvs ];
    uMult   = new int [ numUCvs ];
    vMult   = new int [ numVCvs ];
    surface->CVsWorldPosition( cvs, uMult, vMult );

	fprintf( printFile, "%sU multiplicities  =", indentSpace() );
    for ( i = 0; i < numUCvs; i++ )
    {
        fprintf( printFile, " %d", uMult[i] );
    }
    fprintf( printFile, "\n" );
    fprintf( printFile, "%sV multiplicities  =", indentSpace() );
    for ( i = 0; i < numVCvs; i++ )
    {
        fprintf( printFile, " %d", vMult[i] );
    }
    fprintf( printFile, "\n" );

	delete uMult;
	delete vMult;

	// Print out the CV's
	//

	fprintf( printFile, "%sList of CV's =\n", indentSpace() );
	setIndentation( INDENT );
	for ( k = 0, i = 0; i < numUCvs; i++ )
	{
		for ( j = 0; j < numVCvs; j++, k+=4 )
		{
			fprintf( printFile, "%s%g %g %g %g\n",
					indentSpace(), cvs[k], cvs[k+1], cvs[k+2], cvs[k+3] );
		}
	}
	setIndentation( -INDENT );
	delete cvs;

    // Traverse the list of AlSurfaceCV's
    //
    fprintf( printFile, "%sTraversing CV's Forward: \n", indentSpace() );
    for (surfCVV = surface->firstCV();
         surfCVV != NULL;
         surfCVV = surfCVV->nextInV() )
	{
        for( surfCVU = surfCVV;
             surfCVU != NULL;
             surfCVU = surfCVU->nextInU() )
		{
			printAlSurfaceCV(surfCVU);
        }
    }

	// The following prints out a LOT of information. Add -DDEBUG
	// to the compile line if you want to see it.

    // Print out world position of all CVs including multiples
    //
    numUCvs   = surface->uNumberOfCVsInclMultiples();
    numVCvs   = surface->vNumberOfCVsInclMultiples();
    cvs     = new double [ 4 * numUCvs * numVCvs ];

    surface->CVsWorldPositionInclMultiples( cvs );
    fprintf( printFile, "%s List of %d CV's including multiples =\n",
            indentSpace(), numUCvs * numVCvs );

    // Print out the cvs matrix
	//
    setIndentation( INDENT );
    for ( k = 0, i = 0; i < numUCvs; i++ )
    {
        for ( j = 0; j < numVCvs; j++, k+=4 )
        {
            fprintf( printFile, "%s%g %g %g %g\n",
                    indentSpace(), cvs[k], cvs[k+1], cvs[k+2], cvs[k+3] );
        }
    }
    setIndentation( -INDENT );
    delete cvs;

    // Print out unaffected positions of all CVs including multiplies
	//
    cvs     = new double [ 4 * numUCvs * numVCvs ];
    surface->CVsUnaffectedPositionInclMultiples( cvs );
    fprintf( printFile, "%sList of %d unaffected CV's including multiples =\n",
            indentSpace(), numUCvs * numVCvs );

    setIndentation( INDENT );
    for ( k = 0, i = 0; i < numUCvs; i++ )
    {
        for ( j = 0; j < numVCvs; j++, k+=4 )
        {
            fprintf( printFile, "%s%g %g %g %g\n",
                    indentSpace(), cvs[k], cvs[k+1], cvs[k+2], cvs[k+3] );
        }
    }
    setIndentation( -INDENT );
    delete cvs;
#endif	// DEBUG


	printAlCurvesOnSurface( surface );

	if( surface->trimmed() )
		printAlSurfaceTrimRegions( surface );
	
    setIndentation( -INDENT );
}

void printAlCurvesOnSurface( AlSurface *surface ) 
{
    if( NULL == surface ) return;
    AlCurveOnSurface *cos = surface->firstCurveOnSurface();
    for( ; cos != NULL; cos = cos->nextCurveOnSurface() ) {
		printAlCurveOnSurface( cos );
	}
}

void printAlCurveOnSurface( AlCurveOnSurface *cos )    
{
    if( NULL == cos ) return;

    double  pt[4];
	double  knot;
	int     i;
	char    *form;

	SDF_PrintObjectName( cos );
	setIndentation( INDENT );

    fprintf(printFile, "%sdegree = %d\n", indentSpace(), cos->degree());
    if ( kOpen == cos->form() )
         form = "open";
    else if ( kClosed == cos->form() )
         form = "closed";
    else
         form = "periodic";

	fprintf( printFile, "%sform   = %s\n", indentSpace(), form);
	fprintf(printFile, "%snumberOfSpans = %d\n", indentSpace(),
							cos->numberOfSpans());
	fprintf(printFile, "%snumberOfKnots = %d\n", indentSpace(),
							cos->numberOfKnots());
	fprintf(printFile, "%snumberOfCVs = %d\n", indentSpace(),
							cos->numberOfControlPoints());

	fprintf(printFile, "%sKnots = \n%s", indentSpace(), indentSpace() );
	for( i = 0; i < cos->numberOfKnots(); i ++ ) {
		cos->getKnotValue( i, knot );
		fprintf(printFile, " %g ", knot );
	}
	fprintf(printFile, "\n");

	fprintf(printFile, "%sControl Points =\n", indentSpace());
	for( i = 0; i < cos->numberOfControlPoints(); i ++ ) {
		cos->getControlPoint( i, pt );
		fprintf(printFile, "%s %g, %g, %g, %g\n", indentSpace(),
								pt[0], pt[1], pt[2], pt[3]);
    }

	setIndentation( -INDENT );
}

/*
** Print Alias PolysetNode as a SDF TriMesh object
*/
void 
SDF_PrintAlPolysetNode( AlPolysetNode *polysetNode )
{
	int obj_defined;

	if ( NULL == polysetNode ) return;
	if ( SDF_IsEmptyPolySet( polysetNode->polyset() ) )  return;

	// Print define object
	obj_defined = SDF_PrintAlDagNodeInfo( polysetNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	SDF_PrintAlPolyset( polysetNode->polyset() );
	setIndentation( -INDENT );

	fprintf( printFile, "%s}\n ", indentSpace() );
}

/*
** Tesselate analytically described SurfaceNode into a PolysetNode
** then write this out as a SDF TriMesh
** NOTE : when a 'AlSurfaceNode' gets tesselated a new node of type
**       'AlPolysetNode' is created and added to the parent of
**       the surface node. A unique name is generated for this by
**       bumping up and concatenating the suffix number on this
**       surface node to avoid duplicate names in the Alias World
*/
void 
SDF_PrintAlSurfaceNode( AlSurfaceNode *surfaceNode )
{
	if ( NULL == surfaceNode ) return;
	AlDagNode* tessData;
	int obj_defined;

	// Tesselate all the objects below this group node
	/*
	tessData = AlTesselate::adaptive( *surfaceNode->asDagNodePtr(),
                                      tessPolys,
                                      1, 4, tessQuality );
	if ( tessData == NULL ) return;
	*/
	if (sSuccess != AlTesselate::adaptive( tessData, 
					surfaceNode->asDagNodePtr(), tessPolys, 
					1, 4, tessQuality ))
		return;

#if 0
	AlDagNode* parent = surfaceNode->parentNode();
	fprintf( stderr, "Surface 0x%x , (parent 0x%x) =  %s ", 
						surfaceNode, parent,
						surfaceNode->name());
	if ( parent != NULL ) fprintf( stderr, "( %s ) - ", parent->name() );
	if ( tessData != NULL ) fprintf( stderr, "%s\n", tessData->name() );
	else fprintf( stderr, "\n" );
#endif

	AlPolysetNode *polysetNode = tessData->asPolysetNodePtr(); 
	if ( NULL == polysetNode ) return;
	if ( SDF_IsEmptyPolySet( polysetNode->polyset() ) ) return;

	// Print define object
	obj_defined = SDF_PrintAlDagNodeInfo( surfaceNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	SDF_PrintAlPolyset( polysetNode->polyset() );
	setIndentation( -INDENT );

	fprintf( printFile, "%s}\n ", indentSpace() );

	// Remove and free the 'tessData Node'
	// NOTE : for some reason deleteing "tessData" slows the conversion
	// delete tessData;
}

/*
** Tesselate analytically described FaceNode into a PolysetNode
** then write this out as a SDF TriMesh
** NOTE : when a 'AlFaceNode' gets tesselated a new node of type
**       'AlPolysetNode' is created and added to the parent of
**       the surface node. A unique name is generated for this by
**       bumping up and concatenating the suffix number on this
**       surface node to avoid duplicate names in the Alias World
*/
void 
SDF_PrintAlFaceNode( AlFaceNode *faceNode )
{
	int obj_defined;

	if( NULL == faceNode ) return;

#if 0
    AlRenderInfo        renderInfo;
	AlFace				*f = NULL;

	fprintf( stderr, "-----Face = %s\n", faceNode->name() );

	for( f = faceNode->firstFace(); f; f = f->nextFace() )
	{ 
		if( f->renderInfo( renderInfo ))
    	{
       	 fprintf( stderr, "F %s normals = %d dsided = %d\n", 
							f->name(),
       	                     renderInfo.opposite ,
							 renderInfo.doubleSided );
    	}
	}
#endif

	AlDagNode* tessData;
	/*
	tessData = AlTesselate::adaptive( *faceNode->asDagNodePtr(),
                                      tessPolys,
                                      1, 4, tessQuality );
	if ( tessData == NULL ) return;
	*/
	if (sSuccess != AlTesselate::adaptive( tessData,
					faceNode->asDagNodePtr(), tessPolys, 1, 4, tessQuality ))
		return;

	AlDagNode* parent = faceNode->parentNode();

#if 0
	fprintf( stderr, "Face 0x%x , (parent 0x%x) =  %s ", 
						faceNode, parent,
						faceNode->name());
	if ( parent != NULL ) fprintf( stderr, "( %s ) - ", parent->name() );
	if ( tessData != NULL ) fprintf( stderr, "%s\n", tessData->name() );
	else fprintf( stderr, "\n" );
#endif


	AlPolysetNode *polysetNode = tessData->asPolysetNodePtr(); 
	if ( NULL == polysetNode ) return;
	if ( SDF_IsEmptyPolySet( polysetNode->polyset() ) )  return;

	// Print define object
	obj_defined = SDF_PrintAlDagNodeInfo( faceNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	SDF_PrintAlPolyset( polysetNode->polyset() );
	setIndentation( -INDENT );

	fprintf( printFile, "%s}\n ", indentSpace() );

	// Remove and free the 'tessData Node'
	// NOTE : for some reason deleteing "tessData" slows the conversion
	// delete tessData;
}

void printAlSurfaceCV(AlSurfaceCV *surfaceCV)
{
    double	x, y, z, w;
    double	ux, uy, uz, uw;
    int		multiU, multiV;

	SDF_PrintObjectName(surfaceCV);

	multiU = surfaceCV->multiplicityInU();
	multiV = surfaceCV->multiplicityInV();
	surfaceCV->worldPosition( x, y, z, w );
	surfaceCV->unaffectedPosition( ux, uy, uz, uw );

	setIndentation(INDENT);

	fprintf( printFile,
		"%s(%g %g %g %g) Multiplicity: %d %d\n",
									indentSpace(), x, y, z, w, multiU, multiV);
	fprintf( printFile,
		"%sUnaffected position: (%g %g %g %g)\n", indentSpace(),ux, uy, uz, uw);

	// Print the clusters that this CV is in (if any)
	//
	AlCluster *cluster = surfaceCV->firstCluster();
	if (cluster != NULL)
	{
		fprintf(printFile, "%sCLUSTERS this CV is in:\n", indentSpace());
		setIndentation( INDENT );
		do {
			fprintf(printFile, "%scluster: %s\n",
						indentSpace(), cluster->clusterNode()->name());
		} while (cluster = surfaceCV->nextCluster(cluster));
		setIndentation( -INDENT );
	}

	// Print the animation on this CV (if any)
	//
	AlChannel *channel = surfaceCV->firstChannel();
	if (channel != NULL)
	{
		fprintf(printFile, "%sANIMATION on this CV:\n", indentSpace());
		setIndentation( INDENT );
		do {

			const char	*className	= AlObjectClassName(channel);
			AlObject	*item		= channel->animatedItem();
			const char	*field_name	= channel->parameterName();
			int			field		= channel->parameter();

			fprintf(printFile, "%schannel: (", indentSpace());
			fprintf(printFile, "%s, ", AlObjectClassName(item));
			fprintf(printFile, "%s [%d] )\n", field_name, field);

		} while (channel = surfaceCV->nextChannel(channel));
		setIndentation( -INDENT );
	}

	setIndentation(-INDENT);
}

void printAlSurfaceTrimRegions ( AlSurface *surface )
{
	if( NULL == surface ) return;

	AlTrimRegion*	region = surface->firstTrimRegion();
	while( NULL != region )
	{
		printAlTrimRegion( region );
		region = region->nextRegion();
	}
}

void printAlTrimRegion ( AlTrimRegion *region )
{
	if( NULL == region ) return;

	AlTrimBoundary*	boundary = region->firstBoundary();
	fprintf( printFile, "%sAlNewTrimRegion\n", indentSpace() );

	setIndentation( INDENT );

	fprintf( printFile, "%s--Outer Boundary--\n", indentSpace() );
	printAlTrimBoundary( boundary );

	if( boundary = boundary->nextBoundary() )
	{
		fprintf( printFile, "%s--Inner Boundaries--\n", indentSpace() );
		do {
			printAlTrimBoundary( boundary );			
		} while( boundary = boundary->nextBoundary() );
	}

	setIndentation( -INDENT );
}

void printAlTrimBoundary ( AlTrimBoundary *boundary )
{
	if( NULL == boundary ) return;

	AlTrimCurve*	curve = boundary->firstCurve();
	fprintf( printFile, "%sAlNewTrimBoundary\n", indentSpace() );

	setIndentation( INDENT );

	while( NULL != curve )
	{
		printAlTrimCurve( curve );
		curve = curve->nextCurve();
	}
	setIndentation( -INDENT );
}

void printAlTrimCurve ( AlTrimCurve *curve )
{
	if ( NULL == curve ) return;

	int		 i, j;
	int		 numKnots;
	int		 numCvs;
	int		 numSpans;
	double	*knots;
	double	*cvs;
	char	*form;
	
	fprintf( printFile, "%sAlNewTrimCurve\n", indentSpace() );
	
	setIndentation( INDENT );

	fprintf( printFile, "%sdegree = %d\n", indentSpace(), curve->degree() );

	if ( kOpen == curve->form() )
		form = "open";
	else if ( kClosed == curve->form() )
		form = "closed";
	else
		form = "periodic";
	fprintf( printFile, "%sform   = %s\n", indentSpace(), form);
	
	numKnots = curve->numberOfKnots();
	numCvs   = curve->numberOfCVs();
	numSpans = curve->numberOfSpans();
    fprintf( printFile, "%s# of Spans   = %d\n", indentSpace(), numSpans);
    fprintf( printFile, "%s# of Knots   = %d\n", indentSpace(), numKnots);
    fprintf( printFile, "%s# of CVs     = %d\n", indentSpace(), numCvs);

	knots = new double [ numKnots ];
	cvs   = new double [ 3 * numCvs ];

	curve->CVsUVPosition( knots, (double [][3])cvs );
	fprintf( printFile, "%sknots  = ", indentSpace() );
	for ( i = 0; i < numKnots; i++ )
	{
		fprintf( printFile, " %g", knots[i] );
	}
	fprintf( printFile, "\n" );
		
	fprintf( printFile, "%sList of CV's =\n", indentSpace() );
	setIndentation( INDENT );
	for ( i = 0, j = 0; i < numCvs; i++, j+=3 )
	{
		fprintf( printFile, "%s%g %g %g\n",
				indentSpace(), cvs[j], cvs[j+1], cvs[j+2] );
	}
	setIndentation( -INDENT );

	setIndentation( -INDENT );

	delete cvs;
	delete knots;
}

void printAlCamera( AlCamera *camera )
{
    if( !AlIsValid( camera ) ) return;
    
    AlPerspectiveCamera * pcam = camera->asPerspectiveCameraPtr();

#ifdef COPIOUS_OUTOUT
	SDF_PrintObjectName( camera );

	setIndentation( INDENT );
#endif

	if (pcam)
	{
		double x,y,z;
        double ncp,fcp;

		pcam->worldEye( x,y,z);
		fprintf( printFile, "%sposition { %g %g %g }\n", indentSpace(), x, y, z );
		pcam->worldView( x,y,z);
		fprintf( printFile, "%slookat { %g %g %g }\n", indentSpace(), x, y, z );

#ifdef COPIOUS_OUTOUT
		pcam->worldUp( x,y,z);
		fprintf( printFile, "%sWorld Up   = %g %g %g\n", indentSpace(), x, y, z );
		fprintf( printFile, "%stwist      = %g\n",
			indentSpace(), camera->twistAngle() );
#endif

		fprintf( printFile, "%sAngleOfView %g\n",
			indentSpace(), pcam->angleOfView() );
		fprintf( printFile, "%stwist       %g\n",
			indentSpace(), pcam->twistAngle() );
        pcam->nearClippingPlane( ncp );
		fprintf( printFile, "%shitherDistance %g\n",
			indentSpace(), ncp );
        pcam->farClippingPlane( fcp );
		fprintf( printFile, "%syonDistance %g\n",
			indentSpace(), fcp );

#ifdef COPIOUS_OUTOUT
		// Print the animation on this camera (if any)
		//
		AlChannel *channel = pcam->firstChannel();
		if (channel != NULL)
		{
			fprintf(printFile, "%sANIMATION on this camera:\n", indentSpace());
			setIndentation( INDENT );
			do {

				const char	*className	= AlObjectClassName(channel);
				AlObject	*item		= channel->animatedItem();
				const char	*field_name	= channel->parameterName();
				int			field		= channel->parameter();

				fprintf(printFile, "%schannel: (", indentSpace());
				fprintf(printFile, "%s, ", AlObjectClassName(item));
				fprintf(printFile, "%s [%d] )\n", field_name, field);

			} while (channel = pcam->nextChannel(channel));
			setIndentation( -INDENT );
		}

		setIndentation( -INDENT );
		fprintf(printFile, "%s}\n", indentSpace() );
#endif
	}
	else
	{
        AlOrthographicCamera *ocam = camera->asOrthographicCameraPtr();
        if( !ocam ) return;
        double ncp,fcp;

#ifdef COPIOUS_OUTOUT
        printObjectName( ocam );
#endif
        setIndentation( INDENT );
        ocam->nearClippingPlane( ncp );
		fprintf( printFile, "%shitherDistance %g\n",
			indentSpace(), ncp );
        ocam->farClippingPlane( fcp );
		fprintf( printFile, "%syonDistance %g\n",
			indentSpace(), fcp );
        setIndentation( -INDENT );
	}
}

void printAlCameraNode( AlCameraNode *cameraNode )
{
	if( NULL == cameraNode ) return;

	switch( cameraNode->type())
	{
	  case kCameraEyeType:
		SDF_PrintAlDagNodeInfo( cameraNode );
		setIndentation( INDENT );
		printAlCamera( cameraNode->camera() );
		setIndentation( -INDENT );
		fprintf(printFile, "%s}\n", indentSpace());
		break;

	  case kCameraUpType:
	  case kCameraViewType:

	  default:
		break;
	}
}

void printAlLightGlow( AlLight *light )
{
	if ( light == NULL )
		return;

	if ( light->type() == kAmbientLightType )
	{
		// Ambient light's do not support glow.
		return;
	}

	double value;
	char *typestr = NULL;

	light->parameter( kFLD_LIGHT_GLOW_TYPE, &value );
	// convert glow type from double
	AlLightGlowType glowtype = (AlLightGlowType)int(value);
	switch ( glowtype )
	{
		case kLightGlowOff:
			typestr = "OFF";
			break;
		case kLightGlowLinear:
			typestr = "Linear";
			break;
		case kLightGlowExponential:
			typestr = "Exponential";
			break;
		case kLightGlowBall:
			typestr = "Ball";
			break;
		case kLightGlowSpectral:
			typestr = "Spectral";
			break;
		case kLightGlowRainbow:
			typestr = "Rainbow";
			break;
		default:
			typestr = "UNKNOWN";
			break;
	}
#if 0
	fprintf( printFile, "\n%s# Glow Type: %s\n", indentSpace(), typestr);

	// Only print glow parameters if glow is not off.
	if ( glowtype != kLightGlowOff )
	{
		light->parameter( kFLD_LIGHT_GLOW_INTENSITY, &value );
		fprintf( printFile, "%s# Glow Intensity    = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_GLOW_SPREAD, &value );
		fprintf( printFile, "%s# Glow Spread       = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_GLOW_2DNOISE, &value );
		fprintf( printFile, "%s# Glow 2D Noise     = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_GLOW_RADIAL_NOISE, &value );
		fprintf( printFile, "%s# Glow Radial Noise = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_GLOW_STAR_LEVEL, &value );
		fprintf( printFile, "%s# Glow Star Level   = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_GLOW_OPACITY, &value );
		fprintf( printFile, "%s# Glow Opacity      = %g\n", indentSpace(), value);
	}
#endif

	light->parameter( kFLD_LIGHT_HALO_TYPE, &value );
	// convert halo type from double
	AlLightHaloType halotype = (AlLightHaloType)int(value);
	switch ( halotype )
	{
		case kLightHaloOff:
			typestr = "OFF";
			break;
		case kLightHaloLinear:
			typestr = "Linear";
			break;
		case kLightHaloExponential:
			typestr = "Exponential";
			break;
		case kLightHaloBall:
			typestr = "Ball";
			break;
		case kLightHaloLensFlare:
			typestr = "Lens Flare";
			break;
		case kLightHaloRimHalo:
			typestr = "Rim Halo";
			break;
		default:
			typestr = "UNKNOWN";
			break;
	}
#if 0
	fprintf( printFile, "\n%s# Halo Type: %s\n", indentSpace(), typestr);

	// Only print halo parameters if glow is not off.
	if ( halotype != kLightHaloOff )
	{
		light->parameter( kFLD_LIGHT_HALO_INTENSITY, &value );
		fprintf( printFile, "%s# Halo Intensity    = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_HALO_SPREAD, &value );
		fprintf( printFile, "%s# Halo Spread       = %g\n", indentSpace(), value);
	}
#endif

	light->parameter( kFLD_LIGHT_FOG_TYPE, &value );
	// convert fog type and print.
	AlLightFogType fogtype = (AlLightFogType)int(value);
	switch ( fogtype )
	{
		case kLightFogOff:
			typestr = "OFF";
			break;
		case kLightFogLinear:
			typestr = "Linear";
			break;
		case kLightFogExponential:
			typestr = "Exponential";
			break;
		case kLightFogBall:
			typestr = "Ball";
			break;
		default:
			typestr = "UNKNOWN";
			break;
	}
#if 0
	fprintf( printFile, "\n%s# Fog Type: %s\n", indentSpace(), typestr);

	// Only print fog parameters if glow is not off.
	if ( fogtype != kLightFogOff )
	{
		light->parameter( kFLD_LIGHT_FOG_INTENSITY, &value );
		fprintf( printFile, "%s# Fog Intensity     = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_FOG_SPREAD, &value );
		fprintf( printFile, "%s# Fog Spread        = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_FOG_2DNOISE, &value );
		fprintf( printFile, "%s# Fog 2D Noise      = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_FOG_RADIAL_NOISE, &value );
		fprintf( printFile, "%s# Fog Radial Noise  = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_FOG_STAR_LEVEL, &value );
		fprintf( printFile, "%s# Fog Star Level    = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_FOG_OPACITY, &value );
		fprintf( printFile, "%s# Fog Opacity       = %g\n", indentSpace(), value);
	}

	// Additional parameters.
	if ( glowtype != kLightGlowOff || halotype != kLightHaloOff
		|| fogtype != kLightFogOff )
	{
		light->parameter( kFLD_LIGHT_RADIAL_FREQUENCY, &value );
		fprintf( printFile, "\n%s# Radial Frequency  = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_STAR_POINTS, &value );
		fprintf( printFile, "%s# Star Points       = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_ROTATION, &value );
		fprintf( printFile, "%s# Rotation          = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_NOISE_USCALE, &value );
		fprintf( printFile, "%s# Noise U Scale     = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_NOISE_VSCALE, &value );
		fprintf( printFile, "%s# Noise V Scale     = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_NOISE_UOFFSET, &value );
		fprintf( printFile, "%s# Noise U Offset    = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_NOISE_VOFFSET, &value );
		fprintf( printFile, "%s# Noise V Offset    = %g\n", indentSpace(), value);
		light->parameter( kFLD_LIGHT_NOISE_THRESHOLD, &value );
		fprintf( printFile, "%s# Noise Threshold   = %g\n", indentSpace(), value);
	}
#endif
}

void printAlLight( AlLight *light ) 
{
	if( NULL == light ) return;

#if 0
	SDF_PrintObjectName( light );

	setIndentation( INDENT );
#endif

	double x, y, z;

	// Print out the light parameters for this kind of light
	//
	switch( light->type() )
	{
	  case kAmbientLightType:
#if 0
		fprintf( printFile,"%s# kAmbientLightType\n",indentSpace());
		AL_ASSERT( light->asAmbientLightPtr() != NULL, 
					"Object is not an ambient light\n");
		printAlAmbientLight( light->asAmbientLightPtr() );
#endif
		break;

	  case kPointLightType:
		fprintf( printFile,"%skind     point\n",indentSpace());
		AL_ASSERT( light->asPointLightPtr() != NULL, 
					"Object is not an point light\n");
		printAlPointLight( light->asPointLightPtr() );
		printAlLightGlow( light );
		break;

	  case kDirectionLightType:
		fprintf( printFile,"%skind     directional\n",indentSpace());
		AL_ASSERT( light->asDirectionLightPtr() != NULL, 
					"Object is not an directional light\n");
		printAlDirectionLight( light->asDirectionLightPtr() );
		printAlLightGlow( light );
		break;

	  case kSpotLightType:
		fprintf( printFile,"%skind     spot\n",indentSpace());
		AL_ASSERT( light->asSpotLightPtr() != NULL, 
					"Object is not an spot light\n");
		printAlSpotLight( light->asSpotLightPtr() );
		printAlLightGlow( light );
		break;

	  case kLinearLightType:
#if 0
		fprintf( printFile,"%s# kLinearLightType\n",indentSpace());
		AL_ASSERT( light->asLinearLightPtr() != NULL, 
					"Object is not an linear light\n");
		printAlLinearLight( light->asLinearLightPtr() );
		printAlLightGlow( light );
#endif
		break;

	  case kAreaLightType:
#if 0
		fprintf( printFile,"%s# kAreaLightType\n",indentSpace());
		AL_ASSERT( light->asAreaLightPtr() != NULL, 
					"Object is not an area light\n");
		printAlAreaLight( light->asAreaLightPtr() );
		printAlLightGlow( light );
#endif
		break;

	  default:
		break;
	}

	light->worldPosition( x, y, z );

#if 0
	fprintf( printFile, "%s# Light Position: %g, %g, %g\n",
			indentSpace(), x, y, z );
#endif

	// Print out all the objects linked to this light
	//
	if( light->hasLinkedObjects() ) {
		fprintf( printFile, "%sLINKED objects of this light:\n", indentSpace(),
															light->name());
		setIndentation( INDENT );
		for( AlObject *objPtr = light->firstLinkedObject();
		 	objPtr != NULL;
		 	objPtr = light->nextLinkedObject( objPtr )) {
			fprintf( printFile,"%sObject = %s\n",indentSpace(),objPtr->name());
		}
		setIndentation( -INDENT );
	}


#if 0
	// Print the animation on this light (if any)
	//
	AlChannel *channel = light->firstChannel();
	if (channel != NULL)
	{
		fprintf(printFile, "%sANIMATION on this light:\n", indentSpace());
		setIndentation( INDENT );
		do {

			const char	*className	= AlObjectClassName(channel);
			AlObject	*item		= channel->animatedItem();
			const char	*field_name	= channel->parameterName();
			int			field		= channel->parameter();

			fprintf(printFile, "%schannel: (", indentSpace());
			fprintf(printFile, "%s, ", AlObjectClassName(item));
			fprintf(printFile, "%s [%d] )\n", field_name, field);

		} while (channel = light->nextChannel(channel));
		setIndentation( -INDENT );
	}

	// Print out the light parameters for this kind of light
	//
	switch( light->type() )
	{
	  case kAmbientLightType:
		AL_ASSERT( light->asAmbientLightPtr() != NULL, 
					"Object is not an ambient light\n");
		printAlAmbientLight( light->asAmbientLightPtr() );
		break;

	  case kPointLightType:
		AL_ASSERT( light->asPointLightPtr() != NULL, 
					"Object is not an point light\n");
		printAlPointLight( light->asPointLightPtr() );
		printAlLightGlow( light );
		break;

	  case kDirectionLightType:
		AL_ASSERT( light->asDirectionLightPtr() != NULL, 
					"Object is not an directional light\n");
		printAlDirectionLight( light->asDirectionLightPtr() );
		printAlLightGlow( light );
		break;

	  case kSpotLightType:
		AL_ASSERT( light->asSpotLightPtr() != NULL, 
					"Object is not an spot light\n");
		printAlSpotLight( light->asSpotLightPtr() );
		printAlLightGlow( light );
		break;

	  case kLinearLightType:
		AL_ASSERT( light->asLinearLightPtr() != NULL, 
					"Object is not an linear light\n");
		printAlLinearLight( light->asLinearLightPtr() );
		printAlLightGlow( light );
		break;

	  case kAreaLightType:
		AL_ASSERT( light->asAreaLightPtr() != NULL, 
					"Object is not an area light\n");
		printAlAreaLight( light->asAreaLightPtr() );
		printAlLightGlow( light );
		break;

	  default:
		break;
	}

	setIndentation( -INDENT );
#endif
}


void printAlAmbientLight( AlAmbientLight *light )
{
	if( NULL == light ) return;

	fprintf( printFile, "%s# Shade factor = %g\n",
				indentSpace(), light->shadeFactor() );

	double r,g,b;
	light->color( r, g, b);
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;
	fprintf( printFile, "%sEnabled      On\n", indentSpace());
	fprintf( printFile, "%sColor        { %g %g %g 1 }\n", indentSpace(), r, g, b );

	double intensity;
	intensity = light->intensity();
	fprintf( printFile, "%s# Intensity    = %g\n", indentSpace(), intensity );
}

void printAlPointLight( AlPointLight *light )
{
	if( NULL == light ) return;

#if 0
	fprintf( printFile, "%s# Intensity = %g\n", indentSpace(),light->intensity());
	fprintf( printFile, "%s# Decay     = %d\n", indentSpace(), light->decay() );
	fprintf( printFile, "%s# Shadows   = %d\n", indentSpace(), light->shadows() );
#endif

	double r,g,b;
	light->color( r, g, b);
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;
	fprintf( printFile, "%sEnabled      On\n", indentSpace());
	fprintf( printFile, "%sColor     { %g %g %g 1 }\n", indentSpace(), r, g, b );
}

void printAlDirectionLight( AlDirectionLight *light )
{
	if( NULL == light ) return;

#if 0
	fprintf( printFile, "%s# Intensity = %g\n", indentSpace(),light->intensity());
	fprintf( printFile, "%s# Decay     = %d\n", indentSpace(), light->decay() );
	fprintf( printFile, "%s# Shadows   = %d\n", indentSpace(), light->shadows() );
#endif

	double r,g,b;
	light->color( r, g, b);
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;
	fprintf( printFile, "%sEnabled      On\n", indentSpace());
	fprintf( printFile, "%sColor     { %g %g %g 1 }\n", indentSpace(), r, g, b );

	double x,y,z;
	light->direction( x, y, z);
#if 0
	fprintf( printFile, "%s# Direction = %g %g %g\n", indentSpace(), x, y, z );
#endif
}

void printAlSpotLight( AlSpotLight *light )
{
	if( NULL == light ) return;

#if 0
	fprintf( printFile, "%s# Intensity    = %g\n",
				indentSpace(), light->intensity() );
	fprintf( printFile, "%s# Decay        = %d\n", 
				indentSpace(), light->decay() );
	fprintf( printFile, "%s# Shadows      = %d\n", 
				indentSpace(), light->shadows() );
	fprintf( printFile, "%s# Drop off     = %g\n", 
				indentSpace(), light->dropOff() );
	fprintf( printFile, "%s# Max Bias     = %g\n", 
				indentSpace(), light->maxBias() );
	fprintf( printFile, "%s# Min Bias     = %g\n", 
				indentSpace(), light->minBias() );
	fprintf( printFile, "%s# Spread Angle = %g\n", 
				indentSpace(), light->spreadAngle() );
	fprintf( printFile, "%s# Offset       = %d\n", 
				indentSpace(), light->offset() );
	fprintf( printFile, "%s# MultFactor   = %d\n", 
				indentSpace(), light->multFactor() );
	fprintf( printFile, "%s# Shadowsize   = %d\n", 
				indentSpace(), light->shadowSize() );
	fprintf( printFile, "%s# Penumbra     = %g\n", 
				indentSpace(), light->penumbra() );
#endif

	double r,g,b;
	light->color( r, g, b);
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;
	fprintf( printFile, "%sEnabled      On\n", indentSpace());
	fprintf( printFile, "%sColor        { %g %g %g 1 }\n",
				indentSpace(), r, g, b );

	double x,y,z;
	light->direction( x, y, z );
#if 0
	fprintf( printFile, "%sDirection    = %g %g %g\n",
				indentSpace(), x, y, z );
#endif
}

void printAlLinearLight( AlLinearLight *light )
{
	if( NULL == light ) return;

#if 0
	fprintf( printFile, "%s# Intensity = %g\n", 
				indentSpace(), light->intensity() );
	fprintf( printFile, "%s# Decay     = %d\n", 
				indentSpace(), light->decay() );
	fprintf( printFile, "%s# Shadows   = %d\n", 
				indentSpace(), light->shadows() );
#endif

	double r,g,b;
	light->color( r, g, b);
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;
	fprintf( printFile, "%sEnabled      On\n", indentSpace());
	fprintf( printFile, "%sColor     { %g %g %g 1 }\n", indentSpace(), r, g, b );

	double x,y,z;
	light->axis( x,y,z);
	fprintf( printFile, "%sAxis      = %g %g %g\n", indentSpace(), x, y, z );
}

void printAlAreaLight( AlAreaLight *light )
{
	if( NULL == light ) return;

#if 0
	fprintf( printFile, "%s# Intensity = %g\n", 
				indentSpace(), light->intensity() );
	fprintf( printFile, "%s# Decay     = %d\n", 
				indentSpace(), light->decay() );
	fprintf( printFile, "%s# Shadows   = %d\n", 
				indentSpace(), light->shadows() );
#endif

	double r,g,b;
	light->color( r, g, b);
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;
	fprintf( printFile, "%sEnabled      On\n", indentSpace());
	fprintf( printFile, "%sColor     { %g %g %g 1 }\n",
				indentSpace(), r, g, b );

	double x,y,z;
	light->shortAxis( x,y,z);
	fprintf( printFile, "%sShortAxis = %g, %g, %g\n", 
				 indentSpace(), x, y, z);

	light->longAxis( x,y,z);
	fprintf( printFile, "%sLong Axis = %g, %g, %g\n", 
				 indentSpace(), x, y, z);
}

void printAlLightNode( AlLightNode *lightNode )
{
	if( NULL == lightNode ) return;

	switch( lightNode->type() )
	{
	  case kLightLookAtNodeType:
	  case kLightUpNodeType:
#if 0
		SDF_PrintAlDagNodeInfo( lightNode );
#endif
		break;

	  default:
		SDF_PrintAlDagNodeInfo( lightNode );
		setIndentation( INDENT );
		printAlLight( lightNode->light() );
		setIndentation( -INDENT );
		fprintf( printFile,"%s}\n",indentSpace());
		break;
	}
}

void printAlClusterNode( AlClusterNode *clusterNode )
{
	if( NULL == clusterNode ) return;
#ifdef DEBUG

	SDF_PrintAlDagNodeInfo( clusterNode );

	setIndentation( INDENT );
	printAlCluster( clusterNode->cluster() );
	setIndentation( -INDENT );
#endif
}

void printAlCluster( AlCluster *cluster )
{
	if (NULL == cluster ) return;
#ifdef DEBUG

	SDF_PrintObjectName( cluster );
	setIndentation( INDENT );
	fprintf( printFile, "%sType         = %s\n", indentSpace(),
		(cluster->clusterRestrict() == kMultiCluster ? "MULTI" : "EXCLUSIVE") );
	fprintf( printFile, "%sIsEmpty?     = %s\n", indentSpace(),
										cluster->isEmpty() ? "YES" : "NO" );
	fprintf( printFile, "%s# of members = %d\n", indentSpace(),
										cluster->numberOfMembers() );

	if (cluster->numberOfMembers() > 0)
	{
		fprintf( printFile, "%sMembers:\n", indentSpace() );
		setIndentation( INDENT );
		for (AlClusterMember *member = cluster->firstMember();
			 member != NULL;
			 member = member->nextClusterMember())
		{
			AlCurveCV	*curveCV;
			AlSurfaceCV	*surfaceCV;
			AlObject	*object = member->object();

			SDF_PrintObjectName( object );
			if (curveCV = object->asCurveCVPtr())
			{
				fprintf( printFile, "%s  %% effect = %g\n", indentSpace(),
											cluster->percentEffect(*curveCV));
			}
			else if (surfaceCV = object->asSurfaceCVPtr())
			{
				fprintf( printFile, "%s  %% effect = %g\n", indentSpace(),
											cluster->percentEffect(*surfaceCV));
			}

		}
		setIndentation( -INDENT );
	}

	setIndentation( -INDENT );
#endif
}

void printAlSetName( AlSet *set )
{
	if( NULL == set ) {
		return;
	}

    // Print each set name and its type - exclusive or multiple...
	//
    if( set->isExclusive() ) {
         fprintf(printFile,
               "Set named \"%s\" is EXCLUSIVE & has %d member(s):\n",
               set->name(), set->numberOfMembers());
    } else {
         fprintf(printFile,
               "Set named \"%s\" is MULTI & has %d member(s):\n",
               set->name(), set->numberOfMembers());
    }

	// Print each set member
	//
    AlSetMember *setMember = set->firstMember();
    if( setMember != NULL )
    {
        setIndentation(INDENT);
        do {
            AlObject *object = setMember->object();

            AL_ASSERT( object != NULL, "printAlSet: Internal logic error.");
         	SDF_PrintObjectName( object);
            delete object;
        } while( sSuccess == setMember->nextSetMemberD());
        setIndentation(-INDENT);
        delete setMember;
    }
}

void printAlSetNames()
{
	AlSet *set;

	// Print each set in the universe and its type (exclusive or multiple) 
	// and its set members
	//
    for( set = AlUniverse::firstSet();
         set != NULL;
         set = set->nextSet()) {
		printAlSetName( set );
	}
}

void printAllChannels()
{
	AlChannel	*channel = AlUniverse::firstChannel();

	while (channel != NULL)
	{
		printAlChannel(channel);
		channel = AlUniverse::nextChannel(channel);
	}
}

void printAlChannel(AlChannel *channel)
{
	AlAction	*action;

	if (!channel)
		return;

	const char	*className	= AlObjectClassName(channel);
	AlObject	*item		= channel->animatedItem();
	const char	*field_name	= channel->parameterName();
	int			field		= channel->parameter();

	fprintf(printFile, "%s%s( ", indentSpace(), className);

	fprintf(printFile, "%s [%s], ", item->name(), AlObjectClassName(item));
	fprintf(printFile, "%s [%d] )\n", field_name, field);
	for (int i = channel->numAppliedActions(); i >= 1; i--)
	{
		action = channel->appliedAction(i);
#if TAI_DEBUG
		printAlObject(action);
#endif
		if (action->type() == kMotionActionType)
		{
			fprintf(printFile, "  Action %d: %s [", i, action->name());
			print_component(channel->appliedActionComponent(i));
			fprintf(printFile, "]\n");
		}
		else
			fprintf(printFile, "  Action %d: %s\n", i, action->name());
	}
}

void printAllActions()
{
	AlAction	*action = AlUniverse::firstAction();

	while (action != NULL)
	{
		printAlAction(action);
		action = AlUniverse::nextAction(action);
	}
}

void printAlAction(AlAction *action)
{
	int		 	num;
	AlChannel	*channel;
	AlObject	*item;

	if (!action)
		return;

	const char *	className = AlObjectClassName(action);
	const char *	name = action->name();

	fprintf(printFile, "%s%s(%s)\n", indentSpace(), className, name);
	setIndentation(INDENT);

	fprintf(printFile, "%sComment: %s\n", indentSpace(), action->comment());
	fprintf(printFile, "%sPre-extrap type: ", indentSpace());
	printExtrapType(action->extrapTypePRE());
	fprintf(printFile, "\n%sPost-extrap type: ", indentSpace());
	printExtrapType(action->extrapTypePOST());
	fprintf(printFile, "\n");
	num = action->numChannelReferences();
	fprintf(printFile, "%sNum channels using action: %d\n", indentSpace(), num);

	setIndentation(INDENT);
	for (int i = num; i > 0; i--)
	{
		channel = action->channelReference(i);
		item = channel->animatedItem();
		fprintf(printFile, "%sChannel %d: %s:%s (%d)\n", indentSpace(),
			i, item->name(), channel->parameterName(), channel->parameter());
	}
	setIndentation(-INDENT);

	switch(action->type())
	{
		case kParamActionType:
			printAlParamAction(action->asParamActionPtr());
			break;
		case kMotionActionType:
			printAlMotionAction(action->asMotionActionPtr());
			break;
	}
	setIndentation(-INDENT);
}

void printAlParamAction(
	AlParamAction	*action)
{
	AlKeyframe	*keyframe;

	fprintf(printFile, "%sParam action keyframes:\n", indentSpace());
	setIndentation(INDENT);
	for (keyframe = action->firstKeyframe();
		 keyframe;
		 keyframe = keyframe->next())
	{
		printAlKeyframe(keyframe);
	}
	setIndentation(-INDENT);
}

void printAlMotionAction(
	AlMotionAction	*action)
{
	AlCurveNode	*curve;

	curve = action->motionCurve();
	fprintf(printFile, "%sMotion action curve:\n", indentSpace());
	setIndentation(INDENT);
	fprintf(printFile, "%s%s\n", indentSpace(), curve->name());
	setIndentation(-INDENT);
}

void printExtrapType(
	AlActionExtrapType	type)
{
	switch(type)
	{
		case kEXTRAP_CONSTANT:
			fprintf(printFile, "CONSTANT");
			break;
		case kEXTRAP_CYCLE:
			fprintf(printFile, "CYCLE");
			break;
		case kEXTRAP_OSCILLATE:
			fprintf(printFile, "OSCILLATE");
			break;
		case kEXTRAP_LINEAR:
			fprintf(printFile, "LINEAR");
			break;
		case kEXTRAP_IDENTITY:
			fprintf(printFile, "IDENTITY");
			break;
		default:
			fprintf(printFile, "??");
			break;
	}
}

void print_component(
	AlTripleComponent component)
{
	switch(component)
	{
		case kX_COMPONENT:
			fprintf(printFile, "X");
			break;
		case kY_COMPONENT:
			fprintf(printFile, "Y");
			break;
		case kZ_COMPONENT:
			fprintf(printFile, "Z");
			break;
		default:
			fprintf(printFile, "?");
			break;
	}
}

void printAlKeyframe(
	AlKeyframe	*keyframe)
{
	double	s1, s2;

	if (!keyframe)
		return;

	const char *	className = AlObjectClassName(keyframe);

	fprintf(printFile, "%s%s", indentSpace(), className);
	setIndentation(INDENT);

	s1 = keyframe->location();
	s2 = keyframe->value();
	fprintf(printFile, "(%.1f, %.1f) ", s1, s2);

	fprintf(printFile, "in-tan: %.1f, out-tan: %.1f, locked? %s\n",
			keyframe->inTangent(), keyframe->outTangent(),
			(keyframe->isLocked() ? "TRUE" : "FALSE"));

	setIndentation(-INDENT);
}

#if 0
void printAlStream(
	AlStream	*stream)
{
	float fx, fy, fz, fw;
	double dx, dy, dz, dw;

	if (!stream)
		return;

	switch(stream->streamType())
	{
		case kAA_DATA_FLOAT:
			fprintf(printFile, "%.1f", stream->floatValue());
			break;
		case kAA_DATA_DOUBLE:
			fprintf(printFile, "%.1f", stream->doubleValue());
			break;
		case kAA_DATA_INT:
			fprintf(printFile, "%.1d", stream->intValue());
			break;
		case kAA_DATA_FLOAT_VECTOR:
			stream->getFloatVector(&fx, &fy, &fz, &fw);
			fprintf(printFile, "(%.1f %.1f %.1f %.1f)", fx, fy, fz, fw);
			break;
		case kAA_DATA_DOUBLE_VECTOR:
			stream->getDoubleVector(&dx, &dy, &dz, &dw);
			fprintf(printFile, "(%.1f %.1f %.1f %.1f)", dx, dy, dz, dw);
			break;
		case kAA_DATA_INVALID:
		default:
			fprintf(printFile, "???");
			break;
	}
}
#endif

void printAlEnvironment(AlEnvironment *env)
{
	if (!env)
		return;

	SDF_PrintObjectName(env);

	setIndentation( INDENT );

	// Print the animation on this environment (if any)
	//
	AlChannel *channel = env->firstChannel();
	if (channel != NULL)
	{
		fprintf(printFile, "%sANIMATION on this environment:\n", indentSpace());
		setIndentation( INDENT );
		do {

			const char	*className	= AlObjectClassName(channel);
			AlObject	*item		= channel->animatedItem();
			const char	*field_name	= channel->parameterName();
			int			field		= channel->parameter();

			fprintf(printFile, "%schannel: (", indentSpace());
			fprintf(printFile, "%s, ", AlObjectClassName(item));
			fprintf(printFile, "%s [%d] )\n", field_name, field);

		} while (channel = env->nextChannel(channel));
		setIndentation( -INDENT );
	}

	// Print the environment's textures
	//
	AlTexture	*texture = env->firstTexture();
	while (texture != NULL)
	{
		texture = env->nextTexture(texture);
	}

	setIndentation( -INDENT );
}


/*
** Print SDF object material&texture info from the Alias shader
*/
int 
SDF_PrintAlMatData(AlShader *shader, int *indx )
{
	statusCode sc0, sc1, sc2, sc3;
	int mat_defined;
	M2_Material m;
	int	isdefined = -1;

	if (!shader) return -1;

	mat_defined = SDF_PrintObjectIndex( shader, indx , 1);

	if ( mat_defined == -1 ) 
	{
		// Print the shaders material
		/* setIndentation( INDENT ); */

		double r1, r2, r3, alpha;
		m.dflag = 0;
		m.sflag = 0;
		m.eflag = 0;
		sc0 = shader->parameter( kFLD_SHADING_COMMON_COLOR_R, &m.d_r );
		sc1 = shader->parameter( kFLD_SHADING_COMMON_COLOR_G, &m.d_g );
		sc2 = shader->parameter( kFLD_SHADING_COMMON_COLOR_B, &m.d_b );
		sc3 = shader->parameter( kFLD_SHADING_COMMON_TRANSPARENCY_R, &m.d_a );
		/*
		sc3 = shader->parameter( kFLD_SHADING_COMMON_TRANSPARENCY_G, &m.d_a );
		sc3 = shader->parameter( kFLD_SHADING_COMMON_TRANSPARENCY_B, &m.d_a );
		sc3 = shader->parameter( kFLD_SHADING_COMMON_TRANSPARENCY_SHADE, &m.d_a );
		*/
		m.d_r = m.d_r / 255.0;
		m.d_g = m.d_g / 255.0;
		m.d_b = m.d_b / 255.0;
		m.d_a = 1 - m.d_a / 255.0;
		/* r1 = r1 / 255.0; r2 = r2 / 255.0; r3 = r3 / 255.0; alpha = 1.0 - alpha; */
		if ((sc0 != sInvalidArgument) || (sc1 != sInvalidArgument) ||
			(sc2 != sInvalidArgument) || (sc3 != sInvalidArgument))
			m.dflag = 1;
		else
		{
			m.d_r = 0.0;
			m.d_g = 0.0;
			m.d_b = 0.0;
			m.d_a = 0.0;
		}
		sc0 = shader->parameter( kFLD_SHADING_PHONG_SPECULAR_R, &m.s_r );
		sc1 = shader->parameter( kFLD_SHADING_PHONG_SPECULAR_G, &m.s_g );
		sc2 = shader->parameter( kFLD_SHADING_PHONG_SPECULAR_B, &m.s_b );
		m.s_r = m.s_r / 255.0;
		m.s_g = m.s_g / 255.0;
		m.s_b = m.s_b / 255.0;
		m.s_a = 1.0;
		/* r1 = r1 / 255.0; r2 = r2 / 255.0; r3 = r3 / 255.0; alpha = 1.0; */
		if ((sc0 != sInvalidArgument) || (sc1 != sInvalidArgument) ||
			(sc2 != sInvalidArgument))
			m.sflag = 1;
		else
		{
			m.s_r = 0.0;
			m.s_g = 0.0;
			m.s_b = 0.0;
			m.s_a = 0.0;
		}
		sc0 = shader->parameter( kFLD_SHADING_COMMON_INCANDESCENCE_R, &m.e_r);
		sc1 = shader->parameter( kFLD_SHADING_COMMON_INCANDESCENCE_G, &m.e_g);
		sc2 = shader->parameter( kFLD_SHADING_COMMON_INCANDESCENCE_B, &m.e_b);
		m.e_r = m.e_r / 255.0;
		m.e_g = m.e_g / 255.0;
		m.e_b = m.e_b / 255.0;
		m.e_a = 1.0;
		/* r1 = r1 / 255.0; r2 = r2 / 255.0; r3 = r3 / 255.0; alpha = 1.0; */
		if ((sc0 != sInvalidArgument) || (sc1 != sInvalidArgument) ||
			(sc2 != sInvalidArgument))
			m.eflag = 1;
		else
		{
			m.e_r = 0.0;
			m.e_g = 0.0;
			m.e_b = 0.0;
			m.e_a = 0.0;
		}

		isdefined = Add_M2_Material(&m);
		if (isdefined != -1)
		{
			/* setIndentation( -INDENT ); */
			return ( mat_defined );
		}
		BEGIN_SDF( printFile, "{\n" );
		if ((m.dflag) && (isdefined == -1))
			fprintf(printFile, "%sdiffuse { %g %g %g %g }\n", 
						indentSpace(), m.d_r, m.d_g, m.d_b, m.d_a);
		if ((m.sflag) && (isdefined == -1))
			fprintf(printFile, "%sspecular { %g %g %g %g }\n", 
						indentSpace(), m.s_r, m.s_g, m.s_b, m.s_a);
		if ((m.eflag) && (isdefined == -1))
			fprintf(printFile, "%semission { %g %g %g %g }\n", 
						indentSpace(), m.e_r, m.e_g, m.e_b, m.e_a);
		END_SDF( printFile, "}\n" );
		/* setIndentation( -INDENT ); */
	}
	return ( mat_defined );
}

/*
** Print SDF object material&texture info from the Alias shader
*/
int
SDF_PrintAlTexData(AlShader *shader, int *indx )
{
	int mat_defined;

	if (!shader) return -1;

	// Write the texture references into the SDF file
	if( writeTexRefs )
	{
		// Print the shader's textures
		AlTexture	*texture = shader->firstTexture();
		while (texture != NULL)
		{
			SDF_PrintAlTexture(texture, indx);

			texture = shader->nextTexture(texture);
		}
	}
	return( mat_defined );
}

/*
** Print Alias texture data into SDF file
*/
void 
SDF_PrintAlTexture(AlTexture *texture, int *indx )
{
	char *temp;
	const char *tex_file;
	const char *nm;
	int obj_defined = 0;
 	char buf[80];
	double u_wrap, v_wrap;
	double u_repeat, v_repeat;
	double u_offset, v_offset;
	double uv_rotate;

	if (!texture) return;

	obj_defined = SDF_PrintObjectIndex( texture, indx , 0);
	if ( obj_defined == -1 ) 
	{
		setIndentation( INDENT );
		temp = (char *)texture->filename();

		// get texture tiling info
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UWRAP, &u_wrap );
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UWRAP, &v_wrap );
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UREPEAT, &u_repeat );
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_VREPEAT, &v_repeat );
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UOFFSET, &u_offset );
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_VOFFSET, &v_offset );
		texture->parameter( kFLD_SHADING_COMMON_TEXTURE_ROTATE, &uv_rotate );

#if 0
		printf( "Texture %s UV wrap = %f, %f\n", temp, u_wrap, v_wrap );
		printf( "              UV Repeat = %f, %f\n", u_repeat, v_repeat );
		printf( "              UV Offset = %f, %f\n", u_offset, v_offset );
		printf( "              UV Rotate =  %f\n", uv_rotate );
#endif

		if ( temp != NULL )
		{
			tex_file = SDF_ExtractFileName( temp );
			fprintf(printFile, "%sfileName %s.utf\n", indentSpace(), tex_file );
			// write the texture reference the script file
			if ( scriptFile != NULL ) 
				fprintf( scriptFile, "process_image_tex %s %s.utf\n", temp, tex_file );
			delete[] temp;
		} else {
			nm = texture->name();
			fprintf( stderr, "WARNING: No associted texture file for texture %s\n", 
					&nm[4] );

			tex_file = SDF_LegalName( &nm[4] );
			fprintf(printFile, "%sfileName %s.utf\n", indentSpace(), tex_file );
			if ( scriptFile != NULL ) 
				fprintf( scriptFile, "process_procedural_tex %s %s.utf\n", tex_file, tex_file );
		}
		
     	/*
		sprintf( buf, "txColorOut Texture\n" );
     	WRITE_SDF( printFile, buf );
     	sprintf( buf, "txAlphaOut Prim\n" );
     	WRITE_SDF( printFile, buf );
		*/
		sprintf( buf, "txFirstColor PrimColor\n" );
     	WRITE_SDF( printFile, buf );
		sprintf( buf, "txSecondColor TexColor\n" );
     	WRITE_SDF( printFile, buf );
		sprintf( buf, "txColorOut Blend\n" );
     	WRITE_SDF( printFile, buf );
		sprintf( buf, "txAlphaOut Prim\n" );
     	WRITE_SDF( printFile, buf );
		sprintf( buf, "txBlendOp Mult\n" );
     	WRITE_SDF( printFile, buf );

		if( u_repeat > 1.0 )  WRITE_SDF( printFile,"xWrap Tile\n" );
		if( v_repeat > 1.0 )  WRITE_SDF( printFile,"yWrap Tile\n" );

		END_SDF( printFile, "}\n" );
	}


}

/*
** Print texture coordinates with texture mapping data
*/
void
SDF_TransformTexCoords(
	double *u,
	double *v,	
	AlTexture *texture
	)
{
	double u_wrap, v_wrap;
	double u_repeat, v_repeat;
	double u_offset, v_offset;
	double uv_rotate;
	double center_u, center_v;
	double ou, ov;

	// get texture tiling info
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UREPEAT, &u_repeat );
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_VREPEAT, &v_repeat );
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UOFFSET, &u_offset );
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_VOFFSET, &v_offset );
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_ROTATE, &uv_rotate );
	uv_rotate = -uv_rotate * (22.0 / ( 7.0 * 180 ));

#if 0
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UWRAP, &u_wrap );
	texture->parameter( kFLD_SHADING_COMMON_TEXTURE_UWRAP, &v_wrap );
	printf( "Texture UV wrap = %f, %f\n", u_wrap, v_wrap );
	printf( "              UV Repeat = %f, %f\n", u_repeat, v_repeat );
	printf( "              UV Offset = %f, %f\n", u_offset, v_offset );
	printf( "              UV Rotate =  %f\n", uv_rotate );
#endif

	// Calculate the coordinates with tiling
	*u = u_repeat * (*u) - u_offset;
	*v = v_repeat * (*v) - v_offset;
	center_u = 0.5 * u_repeat - u_offset;
	center_v = 0.5 * v_repeat - v_offset;
	ou = ((*u) - center_u);
	ov = ((*v) - center_v);
	*u = ou * cos( uv_rotate ) - ov * sin( uv_rotate ) + center_u;
	*v = ou * sin( uv_rotate ) + ov * cos( uv_rotate ) + center_v;
	*v = 1.0 - (*v);
}


/*
** Print Alias polygon vertices with position, normal ,texture
** coordinates
*/
void 
SDF_PrintAlPolysetVertex( 
	AlPolysetVertex* vertex, 
	boolean flipNormal, 
	AlTexture *tex_exist,
	int has_color
	)
{
	if ( NULL == vertex ) return;

	double				ux, uy, uz;
	double				nx, ny, nz;
    double              s, t;
    double              r, g, b, a;

	vertex->unaffectedPosition( ux, uy, uz );
	vertex->normal( nx, ny, nz );
	vertex->st( s, t );
	vertex->color( r, g, b, a );

	// Unaffected Vertex position 
	fprintf( printFile, "%s{ %g %g %g ",
			indentSpace(), ux, uy, uz );

	if (has_color)
	{
		r = r / 255.0; g = g / 255.0; b = b / 255.0; 
		a = 1.0;
    	fprintf( printFile, "%g %g %g %g ", r, g, b , a);
	}
	else
	{
		// Vertex Normal
		if ( flipNormal ) fprintf( printFile, "%g %g %g ", -nx, -ny, -nz);
		else fprintf( printFile, "%g %g %g ", nx, ny, nz);
#if 1
		if ( flipNormal ) fprintf( stderr, "WARNING: Flip Normal\n" );
#endif
	}

	// Texture Coordinates
	if (( tex_exist ) && (writeTexRefs))
	{
		SDF_TransformTexCoords( &s, &t, tex_exist );
		fprintf( printFile, "%g %g }\n", s, t );
	} else fprintf( printFile, " }\n");

#ifdef DEBUG
	// Vertex Color - no alpha channel in SDF color
	r = r / 255.0; g = g / 255.0; b = b / 255.0;
    fprintf( printFile, "%g %g %g }\n", r, g, b );
 
	// Print the clusters that this vertex is in (if any)
	//
	AlCluster *cluster = vertex->firstCluster();
	if (cluster != NULL)
	{
		fprintf(printFile, "%sCLUSTERS this vertex is in:\n", indentSpace());
		setIndentation( INDENT );
		do {
			fprintf(printFile, "%scluster: %s\n",
						indentSpace(), cluster->clusterNode()->name());
		} while (cluster = vertex->nextCluster(cluster));
		setIndentation( -INDENT );
	}

	// Print the animation on this vertex (if any)
	//
	AlChannel *channel = vertex->firstChannel();
	if (channel != NULL)
	{
		fprintf(printFile, "%sANIMATION on this vertex:\n", indentSpace());
		setIndentation( INDENT );
		do {

			const char	*className	= AlObjectClassName(channel);
			AlObject	*item		= channel->animatedItem();
			const char	*field_name	= channel->parameterName();
			int			field		= channel->parameter();

			fprintf(printFile, "%schannel: (", indentSpace());
			fprintf(printFile, "%s, ", AlObjectClassName(item));
			fprintf(printFile, "%s [%d] )\n", field_name, field);

		} while (channel = vertex->nextChannel(channel));
		setIndentation( -INDENT );
	}
#endif
}

/*
** Print Alias polygon as a SDF indexed polygon
*/
void 
SDF_PrintAlPolygon( AlPolygon* polygon , boolean twosided)
{
	int ccw_poly = 0;

	if ( NULL == polygon ) return;

	int					index;
	int					numVertices = polygon->numberOfVertices();
	AlPolysetVertex*	vertex;

	// if the number of vertices less than 3 then do not write
	if ( numVertices < 3 ) return;

	// SDF_PrintObjectName( polygon );

#ifdef DEBUG
	double				nx, ny, nz;

	polygon->normal( nx, ny, nz );

	fprintf( printFile, "%sNumber of Vertices = %d\n",
		indentSpace(), numVertices );
	fprintf( printFile, "%sNormal: (%g %g %g)\n",
		indentSpace(),nx, ny, nz);

	fprintf( printFile, "%s", indentSpace() );
	for ( index = 0; index < numVertices; index++ )
	{
		vertex = polygon->vertex(index);
		fprintf( printFile, "%d ", vertex->index() );
	}
	fprintf( printFile, "\n");
#endif

	fprintf( printFile, "%s", indentSpace() );

	if( ccw_poly ) 
	{
		for ( index = 0; index < numVertices; index++ )
		{
			vertex = polygon->vertex(index);
			fprintf( printFile, "%d ", vertex->index() );
		}
		if (twosided)
		{
			for ( index = (numVertices-1); index >= 0; index-- )
			{
				vertex = polygon->vertex(index);
				fprintf( printFile, "%d ", vertex->index() );
			}
		}
	} else {
		for ( index = (numVertices-1); index >= 0; index-- )
		{
			vertex = polygon->vertex(index);
			fprintf( printFile, "%d ", vertex->index() );
		}
		if (twosided)
		{
			for ( index = 0; index < numVertices; index++ )
			{
				vertex = polygon->vertex(index);
				fprintf( printFile, "%d ", vertex->index() );
			}
		}
	}

	fprintf( printFile, "\n");
}

/*
** Output Alias PolySet object as SDF TriMesh object
*/
void 
SDF_PrintAlPolyset( AlPolyset* polyset )
{
	char buf[ 180 ], tmp[ 80 ];
	boolean flipNormal = FALSE;
	boolean twosided = FALSE;

	if ( NULL == polyset ) return;

	int					index, i;
	int					numVertices = polyset->numberOfVertices();
	int					numPolygons = polyset->numberOfPolygons();

	// Print out the render info on this surface
	AlRenderInfo		renderInfo;
	if( polyset->renderInfo( renderInfo ))
	{
	}
		flipNormal = renderInfo.opposite;
		twosided = renderInfo.doubleSided;
		twosided = FALSE;
#if 0
       	fprintf( printFile, "# PolySet %s normals = %d dsided = %d\n", 
							polyset->name(),
       	                     renderInfo.opposite ,
							 renderInfo.doubleSided );
#endif

	// Shader Information
	AlShader *shaderInfo;
	AlTexture *tex_exist = NULL;
	shaderInfo = polyset->firstShader();
	int tex_indx = 0;
	AlShader *sdrInfo;
	int sdr_list = 0;
	int sdr_count = 0;
	boolean texflag;

#if TAI_DEBUG 
	sdr_list = polyset->numberOfShaderLists();
	/* fprintf(printFile, "# %s numberOfShaderLists = %d\n",indentSpace(),sdr_list); */
	texflag = FALSE;
	for (i = 0; i < sdr_list; i++)
	{
		sdrInfo = polyset->firstShader(i);
		tex_exist = sdrInfo->firstTexture();
		if (tex_exist)
			texflag = TRUE;
		while (sdrInfo)
		{
			sdr_count++;
			sdrInfo = polyset->nextShader(i, sdrInfo);
			/* fprintf(printFile, "# %s shader list = %d, count = %d\n", indentSpace(), i, sdr_count); */
		}
	}
	/* fprintf(printFile, "# %s shader count = %d\n", indentSpace(), sdr_count); */
#endif

	tex_exist = SDF_TexturesExist( shaderInfo );

	// write the textures and material reference array
	if ( shaderInfo != NULL ) 
	{
		sprintf( buf, "Use MatArray %s_materials\n", topMdlName );
		WRITE_SDF( printFile, buf );

		/* if ( writeTexRefs && tex_exist ) */
		if ( writeTexRefs && texflag )
		{
				sprintf( buf, "Use TexArray %s_textures\n", topMdlName );
				WRITE_SDF( printFile, buf );

		}
	}

#if TAI_DEBUG
	if (sdr_count >= 2)
	{
		int k, ind, vertcount, num, sdri, j, icnt;
		/*
		int vcount;
		double *vlist;
		double *vptr;
		int *clist;
		*/
		boolean rtnflag;
		int *vflag;
		int *sdrlist;
		int *ilist;
		AlPolygon* polygon;
		int ccw_poly = flipNormal;
		int tex_index, mat_index;
		AlPolysetVertex*	vertex;

		/*
		if ( tex_exist ) 
			vcount = 8;
		else
			vcount = 6;
		vlist = (double *)malloc(sizeof(double) * vcount * numVertices);
		clist = (int *)malloc(sizeof(int) * numPolygons);
		*/
		vflag = (int *)malloc(sizeof(int) * numVertices);
		sdrlist = (int *)malloc(sizeof(int) * numPolygons);

		/*
		vptr = vlist;
		for ( index = 0; index < numVertices; index++ )
		{
			double				ux, uy, uz;
			double				nx, ny, nz;
		    double              s, t;
		    double              r, g, b, a;

			vertex->unaffectedPosition( ux, uy, uz );
			vertex->normal( nx, ny, nz );
			vertex->st( s, t );
			vertex->color( r, g, b, a );
			*vptr = ux;
			*(vptr+1) = uy;
			*(vptr+2) = uz;
			*(vptr+3) = nx;
			*(vptr+4) = ny;
			*(vptr+5) = nz;
			if (tex_exist)
			{
				*(vptr+6) = s;
				*(vptr+7) = t;
				vptr += 8;
			}
			else
			{
				vptr += 6;
			}
		}
		*/
		vertcount = 0;
		k = 0;
		for ( index = 0; index < numPolygons; )
		{
			for ( i = 0; ( (index < numPolygons) && (i < 20) ); i++, index++ )
			{
				polygon = polyset->polygon(index);
				int poly_sdr = polygon->shaderIndex();
				num = polygon->numberOfVertices();
				/*
				*(clist+k) = num;
				*/
				vertcount += num;
				*(sdrlist+k) = poly_sdr;
				k++;
			}
		}
		ilist = (int *)malloc(vertcount * sizeof(int));
		icnt = 0;
		for ( index = 0; index < numPolygons; index++ )
		{
			polygon = polyset->polygon(index);
			int	numVert = polygon->numberOfVertices();
			if( ccw_poly ) 
			{
				for ( ind = 0; ind < numVert; ind++ )
				{
					vertex = polygon->vertex(ind);
					*(ilist+icnt) = vertex->index();
					icnt++;
				}
			} else {
				for ( ind = (numVert-1); ind >= 0; ind-- )
				{
					vertex = polygon->vertex(ind);
					*(ilist+icnt) = vertex->index();
					icnt++;
				}
			}
		}
	
		BEGIN_SDF( printFile, "Surface {\n" );

		for (sdri = 0; sdri < sdr_list; sdri++)
		{
			tex_index = -2;
			mat_index = -2;
			sdrInfo = polyset->firstShader(sdri);
			if ( sdrInfo != NULL ) 
			{
				tex_exist = sdrInfo->firstTexture();
				if( writeTexRefs && tex_exist ) 
				{
					tex_index = SDF_PrintAlTexData( sdrInfo, &tex_indx );
				} 
				else 
				{
					WRITE_SDF( printFile, "TexIndex -1\n" );
				} 

				mat_index = SDF_PrintAlMatData( sdrInfo, &tex_indx );
			}
			/* mark the used vertices */
			for (i = 0; i < numVertices; i++)
				vflag[i] = 0;
			for ( index = 0; index < numPolygons; index++)
			{
				polygon = polyset->polygon(index);
				int	numVert = polygon->numberOfVertices();
				/* if ((sdrlist[index] == tex_index) || (sdrlist[index] == mat_index)) */
				if (sdrlist[index] == sdri)
				{
					for ( ind = 0; ind < numVert; ind++ )
					{
						vertex = polygon->vertex(ind);
						vflag[vertex->index()] = 1;
					}
				}
			}
			// TriMesh object
			BEGIN_SDF( printFile, "TriMesh {\n" );
			BEGIN_SDF( printFile, "vertexList {\n" );
			if (SDF_GetColorFlag())
			{
				if (( tex_exist ) && (writeTexRefs))
					WRITE_SDF( printFile, "format ( LOCATIONS | COLORS | TEXCOORDS )\n");
				else WRITE_SDF( printFile, "format ( LOCATIONS | COLORS )\n");
			}
			else
			{
				if ( tex_exist ) 
					WRITE_SDF( printFile, "format ( LOCATIONS | NORMALS | TEXCOORDS )\n");
				else WRITE_SDF( printFile, "format ( LOCATIONS | NORMALS )\n");
			}
			BEGIN_SDF( printFile, "vertices {\n" );
			for ( index = 0; index < numVertices; index++ )
			{
				if (vflag[index])
				{
					vertex = polyset->vertex(index);
					SDF_PrintAlPolysetVertex( vertex, flipNormal, tex_exist , SDF_GetColorFlag());
				}
			}
    		END_SDF( printFile, "}\n" );
			END_SDF( printFile, "}\n" );

			BEGIN_SDF( printFile, "vertexCount {\n" );
			for ( index = 0; index < numPolygons; )
			{
				buf[0] = '\0';
				rtnflag = 0;
				for ( i = 0; ( (index < numPolygons) && (i < 20) ); i++, index++ )
				{
					/* if ((sdrlist[index] == tex_index) || (sdrlist[index] == mat_index)) */
					if (sdrlist[index] == sdri)
					{
						polygon = polyset->polygon(index);
						num = polygon->numberOfVertices();

						// if the number of vertices less than 3 then do not write
						if ( num < 3 ) tmp[0] = '\0';
						else if ( num > 3 ) sprintf( tmp, "-%d ",num );
						else sprintf( tmp, "%d ", num );
						strcat( buf, tmp );
						if (twosided)
						{
							if ( num < 3 ) tmp[0] = '\0';
							else if ( num > 3 ) sprintf( tmp, "-%d ",num );
							else sprintf( tmp, "%d ", num );
							strcat( buf, tmp );
						}
						rtnflag = 1;
					}
				}
				if (rtnflag)
					strcat( buf, "\n" );
				if (buf[0] != 0)
					WRITE_SDF( printFile, buf );
			}
			END_SDF( printFile, "}\n" );

			k = 0;
			for (i = 0; i < numVertices; i++)
			{
				if (vflag[i])
				{
					vflag[i] = k;
					k++;
				}
			}
			BEGIN_SDF( printFile, "vertexIndices {\n" );
			for ( index = 0; index < numPolygons; index++ )
			{
				polygon = polyset->polygon(index);
				int	numVert = polygon->numberOfVertices();
				/* if ((sdrlist[index] == tex_index) || (sdrlist[index] == mat_index)) */
				if ((sdrlist[index] == sdri) && (numVert >= 3))
				{
					fprintf( printFile, "%s", indentSpace() );
					if( ccw_poly ) 
					{
						for ( ind = 0; ind < numVert; ind++ )
						{
							vertex = polygon->vertex(ind);
							fprintf( printFile, "%d ", vflag[vertex->index()]);
						}
						if (twosided)
						{
							for ( ind = (numVert-1); ind >= 0; ind-- )
							{
								vertex = polygon->vertex(ind);
								fprintf( printFile, "%d ", vflag[vertex->index()]);
							}
						}
					} else {
						for ( ind = (numVert-1); ind >= 0; ind-- )
						{
							vertex = polygon->vertex(ind);
							fprintf( printFile, "%d ", vflag[vertex->index()]);
						}
						if (twosided)
						{
							for ( ind = 0; ind < numVert; ind++ )
							{
								vertex = polygon->vertex(ind);
								fprintf( printFile, "%d ", vflag[vertex->index()]);
							}
						}
					}
					fprintf( printFile, "\n");
				}
			}
    		END_SDF( printFile, "}\n" );

    		END_SDF( printFile, "}\n" );
		}
    	END_SDF( printFile, "}\n" );
		/* 
		free(vlist);
		free(clist);
		*/
		free(sdrlist);
		free(ilist);
	}
	else
	{
#endif
	// Surface with material and texture properties
	BEGIN_SDF( printFile, "Surface {\n" );

	if ( shaderInfo != NULL ) 
	{
		if( writeTexRefs && tex_exist ) 
		{
			SDF_PrintAlTexData( shaderInfo, &tex_indx );
		} else WRITE_SDF( printFile, "TexIndex -1\n" );

		SDF_PrintAlMatData( shaderInfo, &tex_indx );
	}

	// TriMesh object
	BEGIN_SDF( printFile, "TriMesh {\n" );

	// Vertex List
	BEGIN_SDF( printFile, "vertexList {\n" );
	if (SDF_GetColorFlag())
	{
		if (( tex_exist ) && (writeTexRefs))
			WRITE_SDF( printFile, "format ( LOCATIONS | COLORS | TEXCOORDS )\n");
		else WRITE_SDF( printFile, "format ( LOCATIONS | COLORS )\n");
	}
	else
	{
		if ( tex_exist ) 
			WRITE_SDF( printFile, "format ( LOCATIONS | NORMALS | TEXCOORDS )\n");
		else WRITE_SDF( printFile, "format ( LOCATIONS | NORMALS )\n");
	}

	BEGIN_SDF( printFile, "vertices {\n" );
	AlPolysetVertex*	vertex;
	for ( index = 0; index < numVertices; index++ )
	{
		vertex = polyset->vertex(index);
		SDF_PrintAlPolysetVertex( vertex, flipNormal, tex_exist , SDF_GetColorFlag());
	}
    END_SDF( printFile, "}\n" );
    END_SDF( printFile, "}\n" );
	

	AlPolygon* polygon;
	int numVerts; 
	// Vertex count in a polygon
	BEGIN_SDF( printFile, "vertexCount {\n" );
	for ( index = 0; index < numPolygons; )
	{
		buf[0] = '\0';
		for ( i = 0; ( (index < numPolygons) && (i < 20) ); i++, index++ )
		{
			polygon = polyset->polygon(index);
#if TAI_DEBUG
			int poly_sdr = polygon->shaderIndex();
			/* fprintf(printFile, "%s shader index = %d\n", indentSpace(), poly_sdr); */
#endif
			numVerts = polygon->numberOfVertices();

			// if the number of vertices less than 3 then do not write
			if ( numVerts < 3 ) tmp[0] = '\0';
			else if ( numVerts > 3 ) sprintf( tmp, "-%d ",numVerts );
			else sprintf( tmp, "%d ", numVerts );
			strcat( buf, tmp );
			if (twosided)
			{
				if ( numVerts < 3 ) tmp[0] = '\0';
				else if ( numVerts > 3 ) sprintf( tmp, "-%d ",numVerts );
				else sprintf( tmp, "%d ", numVerts );
				strcat( buf, tmp );
			}
		}
		strcat( buf, "\n" );
		WRITE_SDF( printFile, buf );
	}
	END_SDF( printFile, "}\n" );

	// Vertex indices in a polygon
	BEGIN_SDF( printFile, "vertexIndices {\n" );
	for ( index = 0; index < numPolygons; index++ )
	{
		polygon = polyset->polygon(index);
		SDF_PrintAlPolygon(polygon, twosided);
	}
    END_SDF( printFile, "}\n" );

    END_SDF( printFile, "}\n" );
    END_SDF( printFile, "}\n" );
#if TAI_DEBUG
	}
#endif
}

/*
** Start the routines to handle keyframe animation data
*/
/*
** Start dumping out the Alias Universe Keyframe Animation Data
*/
void 
printAlUniverseAnim( char *fname )
{
	AlDagNode *child;
	char top_node_name[80], buf[120], filename[256];
	FILE *fp;

	strcpy( topMdlName, SDF_ExtractFileName( fname ) );
	strcpy( filename, SDF_ExtractFileName( fname ) );
	strcat( filename, ".anim");
	fp = fopen(filename, "w");
	if (fp == NULL)
		fprintf(stderr, "ERROR: Error opening output file %s.\n", filename);
	else
		setPrintFile(fp);

	fprintf( printFile, "SDFVersion 0.1\n" );

	BEGIN_SDF( printFile, "define class KF_Object from Engine {\n" );
	WRITE_SDF( printFile, "character   Target\n");
	WRITE_SDF( printFile, "point       ObjPivot\n");
	WRITE_SDF( printFile, "point       PrntPivot\n");
	WRITE_SDF( printFile, "floatarray  PosFrames\n");
	WRITE_SDF( printFile, "floatarray  PosData\n");
	WRITE_SDF( printFile, "floatarray  PosSplData\n");
	WRITE_SDF( printFile, "floatarray  RotFrames\n");
	WRITE_SDF( printFile, "floatarray  RotData\n");
	WRITE_SDF( printFile, "floatarray  RotSplData\n");
	WRITE_SDF( printFile, "floatarray  SclFrames\n");
	WRITE_SDF( printFile, "floatarray  SclData\n");
	WRITE_SDF( printFile, "floatarray  SclSplData\n");
	END_SDF( printFile, "}\n\n" );
	
	WRITE_SDF( printFile, "Define array kfobjarray of KF_Object\n");
	sprintf(buf, "Define kfobjarray \"%s_kfengines\" {\n", topMdlName);
	BEGIN_SDF( printFile, buf );
	
	animData *anim;
	double *XPos, *YPos, *ZPos, *XRot, *YRot, *ZRot, *XScl, *YScl, *ZScl;
	int numkey, i;
	double dot1, dot2, halfAng, s;
	double  pmq[4], pq[4], cq[4];

	anim = gAnimPtr;
	while (anim != NULL)
	{
		XPos = anim->XPos;	YPos = anim->YPos;	ZPos = anim->ZPos;
		XRot = anim->XRot;	YRot = anim->YRot;	ZRot = anim->ZRot;
		XScl = anim->XScl;	YScl = anim->YScl;	ZScl = anim->ZScl;
		BEGIN_SDF( printFile,  "KF_Object {\n" );
        fprintf(printFile, "%sControl cycle\n", indentSpace());
        fprintf(printFile, "%sDuration %g\n", indentSpace(), anim->duration);
		fprintf( printFile, "%sTarget Use %s\n", indentSpace(), anim->name);
		BEGIN_SDF( printFile,  "ObjPivot {\n" );
        fprintf(printFile, "%s %g %g %g\n", indentSpace(), anim->pvx,
									anim->pvy, anim->pvz);
		END_SDF( printFile,  "}\n" );
		BEGIN_SDF( printFile,  "PrntPivot {\n" );
        fprintf(printFile, "%s %g %g %g\n", indentSpace(), anim->pvinx,
									anim->pviny, anim->pvinz);
		/*
        fprintf(printFile, "%s %g %g %g\n", indentSpace(), anim->pvx,
									anim->pvy, anim->pvz);
		*/
		END_SDF( printFile,  "}\n" );
        if ((anim->XPos) && (anim->YPos) && (anim->ZPos))
        {       
            numkey = (int)*XPos;
			BEGIN_SDF( printFile,  "PosFrames {\n" );
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g\n", indentSpace(), *(XPos + i + 1));
            END_SDF( printFile,  "}\n" );
            BEGIN_SDF( printFile,  "PosData {\n" );
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g %g %g\n", indentSpace(),
                                    *(XPos + i + 1 + numkey),
                                    *(YPos + i + 1 + numkey),
                                    *(ZPos + i + 1 + numkey));
			END_SDF( printFile,  "}\n" );
        }
		else
		{
			if ((XRot) && (YRot) && (ZRot))
			{
            	numkey = (int)*XRot;
            	BEGIN_SDF( printFile,  "PosFrames {\n" );
            	for (i = 0; i < numkey; i++)
               		fprintf(printFile, "%s%g\n", indentSpace(), *(XRot+i+1));
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "PosData {\n" );
            	for (i = 0; i < numkey; i++)
            		fprintf(printFile, "%s 0 0 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
			}
			else if ((XScl) && (YScl) && (ZScl))
			{
            	numkey = (int)*XScl;
            	BEGIN_SDF( printFile,  "PosFrames {\n" );
            	for (i = 0; i < numkey; i++)
               		fprintf(printFile, "%s%g\n", indentSpace(), *(XScl+i+1));
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "PosData {\n" );
            	for (i = 0; i < numkey; i++)
            		fprintf(printFile, "%s 0 0 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
			}
			else
			{
				BEGIN_SDF( printFile,  "PosFrames {\n" );
           		fprintf(printFile, "%s 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "PosData {\n" );
            	fprintf(printFile, "%s 0 0 0\n", indentSpace());
				END_SDF( printFile,  "}\n" );
			}
		}
        if ((XRot) && (YRot) && (ZRot))
        {
			double mat[4][4], axis[3], angle, ang;
			AlTM imat;
			int j;

            numkey = (int)*XRot;
            BEGIN_SDF( printFile,  "RotFrames {\n" );
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g\n", indentSpace(), *(XRot + i + 1));
            END_SDF( printFile,  "}\n" );
            BEGIN_SDF( printFile,  "RotData {\n" );
			for (i = 0; i < numkey; i++)
			{
				/*
				imat = AlTM::rotateX(0.25 * PI);
            	for (j = 0; j < 4; j++)
               		fprintf(printFile, "# %s%g %g %g %g\n", indentSpace(), 
						imat[j][0], imat[j][1], imat[j][2], imat[j][3]);
				mx_Identity(mat);
				mx_Rotate(mat, 'x', 0.25 * PI);
            	for (j = 0; j < 4; j++)
               		fprintf(printFile, "# %s%g %g %g %g\n", indentSpace(), 
						mat[j][0], mat[j][1], mat[j][2], mat[j][3]);
				*/

				mx_Identity(mat);
				if (i == 0)
				{
					angle = *(XRot + i + 1 + numkey);
					mx_Rotate(mat, 'x', angle/180*PI);
					angle = *(YRot + i + 1 + numkey);
					mx_Rotate(mat, 'y', angle/180*PI);
					angle = *(ZRot + i + 1 + numkey);
					mx_Rotate(mat, 'z', angle/180*PI);
				}
				else
				{
					angle = *(XRot + i + 1 + numkey);/*- *(XRot + i + numkey);*/
					mx_Rotate(mat, 'x', angle/180*PI);
					angle = *(YRot + i + 1 + numkey);/*- *(YRot + i + numkey);*/
					mx_Rotate(mat, 'y', angle/180*PI);
					angle = *(ZRot + i + 1 + numkey);/*- *(ZRot + i + numkey);*/
					mx_Rotate(mat, 'z', angle/180*PI);
				}
				AxisRotation(mat, axis, &ang );
				if ((axis[0] == 0.0) && (axis[1] == 0.0) && (axis[2] == 0.0) && (ang == 0.0))
						axis[0] = 1.0;
				/* 
					make sure that the current rotation key is has the shortest
					rotational arc. This is done by testing magnitudes of 
					(pq-cq).(pq-cq) or (pq+cq).(pq+q). pq and cq are previous 
					and current quaternion.
				*/
				{
	
					cq[0]  = axis[0]*sin(ang/2.0); 
					cq[1]  = axis[1]*sin(ang/2.0); 
					cq[2]  = axis[2]*sin(ang/2.0);
					cq[3]  = cos(ang/2.0);
					
					if( i > 0 ) {
						pmq[0] = pq[0] - cq[0]; pmq[1] = pq[1] - cq[1]; 
						pmq[2] = pq[2] - cq[2]; pmq[3] = pq[3] - cq[3];
						dot1 = pmq[0] * pmq[0] + pmq[1] * pmq[1] + 
						       pmq[2] * pmq[2] + pmq[3] * pmq[3];
						pmq[0] = pq[0] + cq[0]; pmq[1] = pq[1] + cq[1]; 
						pmq[2] = pq[2] + cq[2]; pmq[3] = pq[3] + cq[3];
						dot2 = pmq[0] * pmq[0] + pmq[1] * pmq[1] + 
						       pmq[2] * pmq[2] + pmq[3] * pmq[3];
						if ( dot1 > dot2 ) {
							cq[0]  = -cq[0]; 
							cq[1]  = -cq[1]; 
							cq[2]  = -cq[2];
							cq[3]  = -cq[3];
							
						}
						halfAng = acos( cq[3] );
						ang = 2.0 * halfAng;
						// normalize the vector
						s = sqrt(
								cq[0] * cq[0] +
								cq[1] * cq[1] +
								cq[2] * cq[2]
								);
						if ( s != 0.0 )
						{
							axis[0] = cq[0] / s;
							axis[1] = cq[1] / s;
							axis[2] = cq[2] / s;
						}
					}	
					pq[0]  = cq[0]; 
					pq[1]  = cq[1]; 
					pq[2]  = cq[2];
					pq[3]  = cq[3];
				}
				/* if (ang >= PI)
					ang -= PI; */
				fprintf(printFile, "%s%g %g %g %g\n", indentSpace(),
									axis[0], axis[1], axis[2], ang);
			}
			/* 
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g %g %g\n", indentSpace(),
                                    *(XRot + i + 1 + numkey),
                                    *(YRot + i + 1 + numkey),
                                    *(ZRot + i + 1 + numkey));
			*/
            END_SDF( printFile,  "}\n" );
        }
		else
		{
			if ((XPos) && (YPos) && (ZPos))
			{
            	numkey = (int)*XPos;
            	BEGIN_SDF( printFile,  "RotFrames {\n" );
            	for (i = 0; i < numkey; i++)
               		fprintf(printFile, "%s%g\n", indentSpace(), *(XPos+i+1));
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "RotData {\n" );
            	for (int i = 0; i < numkey; i++)
            		fprintf(printFile, "%s 1 0 0 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
			}
			else if ((XScl) && (YScl) && (ZScl))
			{
            	numkey = (int)*XScl;
            	BEGIN_SDF( printFile,  "RotFrames {\n" );
            	for (i = 0; i < numkey; i++)
               		fprintf(printFile, "%s%g\n", indentSpace(), *(XScl+i+1));
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "RotData {\n" );
            	for (int i = 0; i < numkey; i++)
            		fprintf(printFile, "%s 1 0 0 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
			}
			else
			{
				BEGIN_SDF( printFile,  "RotFrames {\n" );
            	fprintf(printFile, "%s 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "RotData {\n" );
            	fprintf(printFile, "%s 1 0 0 0\n", indentSpace());
				END_SDF( printFile,  "}\n" );
			}
		}
        if ((XScl) && (YScl) && (ZScl))
        {
            numkey = (int)*XScl;
            BEGIN_SDF( printFile,  "SclFrames {\n" );
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g\n", indentSpace(), *(XScl + i + 1));
            END_SDF( printFile,  "}\n" );
            BEGIN_SDF( printFile,  "SclData {\n" );
            for (i = 0; i < numkey; i++)
                fprintf(printFile, "%s%g %g %g\n", indentSpace(),
                                    *(XScl + i + 1 + numkey),
                                    *(YScl + i + 1 + numkey),
                                    *(ZScl + i + 1 + numkey));
            END_SDF( printFile,  "}\n" );
        }
		else
		{
			if ((XPos) && (YPos) && (ZPos))
			{
            	numkey = (int)*XPos;
				BEGIN_SDF( printFile,  "SclFrames {\n" );
            	for (i = 0; i < numkey; i++)
               		fprintf(printFile, "%s%g\n", indentSpace(), *(XPos+i+1));
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "SclData {\n" );
            	for (i = 0; i < numkey; i++)
            		fprintf(printFile, "%s 1 1 1\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
			}
			else if ((XRot) && (YRot) && (ZRot))
			{
            	numkey = (int)*XRot;
				BEGIN_SDF( printFile,  "SclFrames {\n" );
            	for (i = 0; i < numkey; i++)
               		fprintf(printFile, "%s%g\n", indentSpace(), *(XRot+i+1));
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "SclData {\n" );
            	for (i = 0; i < numkey; i++)
            		fprintf(printFile, "%s 1 1 1\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
			}
			else
			{
				BEGIN_SDF( printFile,  "SclFrames {\n" );
            	fprintf(printFile, "%s 0\n", indentSpace());
            	END_SDF( printFile,  "}\n" );
            	BEGIN_SDF( printFile,  "SclData {\n" );
            	fprintf(printFile, "%s 1 1 1\n", indentSpace());
				END_SDF( printFile,  "}\n" );
			}
		}
		END_SDF( printFile,  "}\n" );
		anim = anim->next;
		if (XPos) free(XPos);
		if (YPos) free(YPos);
		if (ZPos) free(ZPos);
		if (XRot) free(XRot);
		if (YRot) free(YRot);
		if (ZRot) free(ZRot);
		if (XScl) free(XScl);
		if (YScl) free(YScl);
		if (ZScl) free(ZScl);
		free(gAnimPtr);
		gAnimPtr = anim;
	}
	/*
	child = AlUniverse::firstDagNode();
	while ( child != NULL ) 
	{
		printAlAnimObject( child );
		child = child->nextNode();
	}
	*/

	END_SDF( printFile, "}\n" );

	fclose(fp);
}

/*
** Print Alias object to SDF object
*/
void 
printAlAnimObject( AlObject *object )
{
	if ( NULL == object ) return;

	fprintf( printFile, "# printAlAnimObject : type = %d\n", object->type());
    switch ( object->type() )
	{

#if 0 
	  case kCameraType:
		printAlCamera( object->asCameraPtr() );
		break;

	  case kCameraEyeType:
	  case kCameraViewType:
	  case kCameraUpType:
		printAlCameraNode( object->asCameraNodePtr() );
		break;

	  case kClusterMemberType:
		AlClusterMember*	clusterMember = object->asClusterMemberPtr();
		break;

	  case kClusterType:
		AlCluster*	cluster = object->asClusterPtr();
		break;

	  case kClusterNodeType:
		printAlClusterNode( object->asClusterNodePtr() );
		break;

	  case kCurveType:
		printAlCurve( object->asCurvePtr() );
		break;
	  case kCurveNodeType:
		printAlCurveNode( object->asCurveNodePtr() );
		break;

	  case kCurveOnSurfaceType:
		printAlCurveOnSurface( object->asCurveOnSurfacePtr() );
		break;
#endif

	  case kFaceType:
		fprintf(stderr, "kFaceType OBJECT\n");
		break;

	  case kFaceNodeType:
		SDF_PrintAlFaceNode( object->asFaceNodePtr() );
		break;

	  case kDagNodeType:
		fprintf(printFile, "kDagNodeType\n");
		printAlDagNode( object->asDagNodePtr() );
		break;

	  case kGroupNodeType:
		SDF_PrintAlAnimGroupNode( object->asGroupNodePtr() );
		break;

#if TAI_DEBUG
	  case kLightLookAtNodeType:
	  case kLightNodeType:
	  case kLightUpNodeType:
		printAlLightNode( object->asLightNodePtr() );
		break;

	  case kLightType:
	  case kNonAmbientLightType:
		break;

	  case kAmbientLightType:
		printAlAmbientLight( object->asAmbientLightPtr() );
		break;

	  case kPointLightType:
		printAlPointLight( object->asPointLightPtr() );
		break;

	  case kDirectionLightType:
		printAlDirectionLight( object->asDirectionLightPtr() );
		break;

	  case kSpotLightType:
		printAlSpotLight( object->asSpotLightPtr() );
		break;

	  case kLinearLightType:
		printAlLinearLight( object->asLinearLightPtr() );
		break;

	  case kAreaLightType:
		printAlAreaLight( object->asAreaLightPtr() );
		break;

	  case kSetType:
		printAlSetName( object->asSetPtr() );
		break;

	  case kSetMemberType:
		break;

#endif
	  case kActionType:
	  case kParamActionType:
	  case kMotionActionType:
		printAlAnimAction(object->asActionPtr());
		break;

	  case kChannelType:
		printAlAnimChannel(object->asChannelPtr());
		break;


	  case kSurfaceType:
		{
		fprintf(stderr, "kSurfaceType OBJECT\n");
		AlSurface* surface = object->asSurfacePtr();
		printAlSurface( surface );
		if( surface->trimmed() )
			printAlSurfaceTrimRegions( surface );
		}
		break;

	  case kPolysetNodeType:
		SDF_PrintAlAnimPolysetNode( object->asPolysetNodePtr() );
		break;

	  case kSurfaceNodeType:
		SDF_PrintAlAnimSurfaceNode( object->asSurfaceNodePtr() );
		break;

	  default:
		break;
	}
}

/*
** Print Alias group node as a SDF character with bunch of
** children as characters
*/
void 
SDF_PrintAlAnimGroupNode( AlGroupNode *groupNode )
{
	int num_sdf_objects = 0;
	int obj_defined;

	fprintf( printFile, "# SDF_PrintAlAnimGroupNode\n");
	if ( NULL == groupNode ) return;

	// Recursively find if there are any SDF objects in
	// Alias hierarchy
	SDF_NumObjects( (AlObject *)groupNode, &num_sdf_objects );

#if 0
	fprintf( stderr, "Number of SDF objects in group %s = %d\n", 
			AlObjectClassName( groupNode ), num_sdf_objects );

#endif
	fprintf( printFile, "# SDF_PrintAlAnimGroupNode : num of obj = %d\n",
						num_sdf_objects);
	if ( num_sdf_objects == 0 ) return;

	// Print define object
	obj_defined = SDF_PrintAlAnimDagNodeInfo( groupNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	AlDagNode *child = groupNode->childNode();
	while ( child != NULL ) {
		printAlAnimObject( child );
		child = child->nextNode();
	}
	setIndentation( -INDENT );

	// End define object 
	fprintf( printFile, "%s}\n ", indentSpace() );

	// Recurse across to print siblings
    // printAlObject( groupNode->nextNode() );
}

/*
** Tesselate analytically described SurfaceNode into a PolysetNode
** then write this out as a SDF TriMesh
** NOTE : when a 'AlSurfaceNode' gets tesselated a new node of type
**       'AlPolysetNode' is created and added to the parent of
**       the surface node. A unique name is generated for this by
**       bumping up and concatenating the suffix number on this
**       surface node to avoid duplicate names in the Alias World
*/
void 
SDF_PrintAlAnimSurfaceNode( AlSurfaceNode *surfaceNode )
{
	fprintf( printFile, "SDF_PrintAlAnimSurfaceNode\n");
	if ( NULL == surfaceNode ) return;
	AlDagNode* tessData;
	int obj_defined;

	// Tesselate all the objects below this group node
	/*
	tessData = AlTesselate::adaptive( *surfaceNode->asDagNodePtr(),
                                      tessPolys,
                                      1, 4, tessQuality );
	if ( tessData == NULL ) return;
	*/
	if (sSuccess != AlTesselate::adaptive( tessData, 
					surfaceNode->asDagNodePtr(), tessPolys, 
					1, 4, tessQuality ))
		return;

#if 0
	AlDagNode* parent = surfaceNode->parentNode();
	fprintf( stderr, "Surface 0x%x , (parent 0x%x) =  %s ", 
						surfaceNode, parent,
						surfaceNode->name());
	if ( parent != NULL ) fprintf( stderr, "( %s ) - ", parent->name() );
	if ( tessData != NULL ) fprintf( stderr, "%s\n", tessData->name() );
	else fprintf( stderr, "\n" );
#endif

	AlPolysetNode *polysetNode = tessData->asPolysetNodePtr(); 
	if ( NULL == polysetNode ) return;
	if ( SDF_IsEmptyPolySet( polysetNode->polyset() ) ) return;

	// Print define object
	obj_defined = SDF_PrintAlDagNodeInfo( surfaceNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	SDF_PrintAlPolyset( polysetNode->polyset() );
	setIndentation( -INDENT );

	fprintf( printFile, "%s}\n ", indentSpace() );

	// Remove and free the 'tessData Node'
	// NOTE : for some reason deleteing "tessData" slows the conversion
	// delete tessData;
}

/*
** Print Alias PolysetNode as a SDF TriMesh object
*/
void 
SDF_PrintAlAnimPolysetNode( AlPolysetNode *polysetNode )
{
	int obj_defined;

	fprintf( printFile, "# SDF_PrintAlAnimPolysetNode\n");
	if ( NULL == polysetNode ) return;
	if ( SDF_IsEmptyPolySet( polysetNode->polyset() ) )  return;

	// Print define object
	obj_defined = SDF_PrintAlAnimDagNodeInfo( polysetNode->asDagNodePtr() );
	// if it is defined earlier then use it
	if ( obj_defined ) return;

	setIndentation( INDENT );
	SDF_PrintAlPolyset( polysetNode->polyset() );
	setIndentation( -INDENT );

	fprintf( printFile, "%s}\n ", indentSpace() );
}

/*
** Print Alias DAG node as a SDF group Character
** Print the local transformation matrix of this group 
** return value indicates whether it is defined earlier or not
*/
int 
SDF_PrintAlAnimDagNodeInfo( AlDagNode *dagNode )
{
	int obj_defined = 1;

	fprintf( printFile, "# SDF_PrintAlAnimDagNodeInfo\n");
	if ( NULL == dagNode ) return ( obj_defined );

	obj_defined = SDF_PrintAnimObjectName( dagNode );


#if 1
    //  Print the display state of the dag Node  
    //
    if( dagNode->isDisplayModeSet( kDisplayModeInvisible ) ) {
        fprintf( printFile, "%sDag Node \"%s\" is invisible\n",
												indentSpace(), dagNode->name());
    }
    if( dagNode->isDisplayModeSet( kDisplayModeBoundingBox ) ) {
        fprintf( printFile, "%sDag Node \"%s\" is bounding boxed\n",
												indentSpace(), dagNode->name());
    }
    if( dagNode->isDisplayModeSet( kDisplayModeTemplate ) ) {
        fprintf( printFile, "%sDag Node \"%s\" is templated\n",
											indentSpace(), dagNode->name());
    }

	if( dagNode->isDisplayModeSet( kDisplayModeQuickWire ) ) {
		fprintf( printFile, "%sDag Node  \"%s\" is quick wired\n",
											indentSpace(), dagNode->name());
	}

	// If the dag node is bounding boxed, print out the corners of the
	// bounding box
	//
    if( dagNode->isDisplayModeSet( kDisplayModeBoundingBox ) ) {
        double corners[8][4];
        dagNode->boundingBox( corners );
        fprintf(printFile, "%sBounding box is:\n", indentSpace() );

        for ( int i = 0; i < 8; i++ ) {
                    fprintf(printFile, "%s   %g, %g, %g\n", indentSpace(),  
											corners[i][0],
                                            corners[i][1],
                                            corners[i][2]);
        }
    }

	// Print the clusters that this dag node is in (if any)
	//
	AlCluster *cluster = dagNode->firstCluster();
	if (cluster != NULL)
	{
		fprintf(printFile, "%sCLUSTERS this dag node is in:\n", indentSpace());
		setIndentation( INDENT );
		do {
			fprintf(printFile, "%scluster: %s\n",
						indentSpace(), cluster->clusterNode()->name());
		} while (cluster = dagNode->nextCluster(cluster));
		setIndentation( -INDENT );
	}

	// Print the animation on this dag node (if any)
	//
	AlChannel *channel = dagNode->firstChannel();
	double **ptr;
	double *XPos = NULL, *YPos = NULL, *ZPos = NULL;
	double *XRot = NULL, *YRot = NULL, *ZRot = NULL;
	double *XScl = NULL, *YScl = NULL, *ZScl = NULL;
	double duration;
	int numkey;
	if (channel != NULL)
	{
		/*
		fprintf(printFile, "%sANIMATION on this dag node:\n", indentSpace());
		*/
		do {

			const char	*className	= AlObjectClassName(channel);
			AlObject	*item		= channel->animatedItem();
			const char	*field_name	= channel->parameterName();
			int			field		= channel->parameter();
			AlAction	*action;

			for (int i = channel->numAppliedActions(); i >= 1; i--)
			{
				action = channel->appliedAction(i);
				if (!strcmp(field_name, "X Translate"))
				{
#if 0
					fprintf(printFile, "# This channel is X Translate\n");
#endif
					ptr = &XPos;
				}
				else if (!strcmp(field_name, "Y Translate"))
				{
#if 0
					fprintf(printFile, "# This channel is Y Translate\n");
#endif
					ptr = &YPos;
				}
				else if (!strcmp(field_name, "Z Translate"))
				{
#if 0
					fprintf(printFile, "# This channel is Z Translate\n");
#endif
					ptr = &ZPos;
				}
				else if (!strcmp(field_name, "X Rotate"))
				{
#if 0
					fprintf(printFile, "# This channel is X Rotate\n");
#endif
					ptr = &XRot;
				}
				else if (!strcmp(field_name, "Y Rotate"))
				{
#if 0
					fprintf(printFile, "# This channel is Y Rotate\n");
#endif
					ptr = &YRot;
				}
				else if (!strcmp(field_name, "Z Rotate"))
				{
#if 0
					fprintf(printFile, "# This channel is Z Rotate\n");
#endif
					ptr = &ZRot;
				}
				else
				{
#if 0
					fprintf(printFile, "# other channel: %s\n", field_name);
#endif
				}
				if (action->type() == kMotionActionType)
					printAlAnimMotionAction(action->asMotionActionPtr());
				else
					printAlAnimParamAction(ptr, action->asParamActionPtr());
			}
			/*
			printAlAnimChannel( channel );
			fprintf(printFile, "%schannel: (", indentSpace());
			fprintf(printFile, "%s [%s], ",
									item->name(), AlObjectClassName(item));
			fprintf(printFile, "%s [%d] )\n", field_name, field);
			*/
			/* printAlObject( item ); */

		} while (channel = dagNode->nextChannel(channel));
		duration = 0;
		get_duration(&duration, XPos);
		get_duration(&duration, YPos);
		get_duration(&duration, ZPos);
		get_duration(&duration, XRot);
		get_duration(&duration, YRot);
		get_duration(&duration, ZRot);
		get_duration(&duration, XScl);
		get_duration(&duration, YScl);
		get_duration(&duration, ZScl);
		fprintf(printFile, "%sDuration %g\n", indentSpace(), duration);
		if ((XPos) && (YPos) && (ZPos))
		{
			numkey = (int)*XPos;
			BEGIN_SDF( printFile,  "PosFrames {\n" );
			for (int i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g\n", indentSpace(), *(XPos + i + 1));
			END_SDF( printFile,  "}\n" );
			BEGIN_SDF( printFile,  "PosData {\n" );
			for (i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g %g %g\n", indentSpace(), 
									*(XPos + i + 1 + numkey),
									*(YPos + i + 1 + numkey),
									*(ZPos + i + 1 + numkey));
			END_SDF( printFile,  "}\n" );
		}
		if ((XRot) && (YRot) && (ZRot))
		{
			numkey = (int)*XRot;
			BEGIN_SDF( printFile,  "RotFrames {\n" );
			for (int i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g\n", indentSpace(), *(XRot + i + 1));
			END_SDF( printFile,  "}\n" );
			BEGIN_SDF( printFile,  "RotData {\n" );
			for (i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g %g %g\n", indentSpace(), 
									*(XRot + i + 1 + numkey),
									*(YRot + i + 1 + numkey),
									*(ZRot + i + 1 + numkey));
			END_SDF( printFile,  "}\n" );
		}
		if ((XScl) && (YScl) && (ZScl))
		{
			numkey = (int)*XScl;
			BEGIN_SDF( printFile,  "SclFrames {\n" );
			for (int i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g\n", indentSpace(), *(XScl + i + 1));
			END_SDF( printFile,  "}\n" );
			BEGIN_SDF( printFile,  "SclData {\n" );
			for (i = 0; i < numkey; i++)
				fprintf(printFile, "%s%g %g %g\n", indentSpace(), 
									*(XScl + i + 1 + numkey),
									*(YScl + i + 1 + numkey),
									*(ZScl + i + 1 + numkey));
			END_SDF( printFile,  "}\n" );
		}
	}
	if (XPos)	free(XPos);
	if (YPos)	free(YPos);
	if (ZPos)	free(ZPos);
	if (XRot)	free(XRot);
	if (YRot)	free(YRot);
	if (ZRot)	free(ZRot);
	if (XScl)	free(XScl);
	if (YScl)	free(YScl);
	if (ZScl)	free(ZScl);
#endif

	// Print the local transformation matrices 
  	if ( !obj_defined )
	{ 
    	double matrix[4][4];
    	int    i;
    	dagNode->localTransformationMatrix( matrix );
	
		BEGIN_SDF( printFile,  "Transform {\n" );
    	for( i  = 0; i < 4; i ++ ) {
       	 fprintf(printFile, "%s%g %g %g %g\n", indentSpace(),
                        matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3]);
    	}
		END_SDF( printFile, "}\n" );
	}

#if 0
    dagNode->globalTransformationMatrix( matrix );
    fprintf(printFile, "%sGlobal Transform is: \n", indentSpace());
    for( i  = 0; i < 4; i ++ ) {
        fprintf(printFile, "%s%g, %g, %g, %g\n", indentSpace(),
                        matrix[i][0], matrix[i][1], matrix[i][2], matrix[i][3]);
    }
	AlJointNode*	jointNode = dagNode->jointNode();
	if ( jointNode == NULL)
		fprintf(printFile, "%sDag node has no jointNode.\n", indentSpace());
	else
		printAlJointNode(jointNode);

#endif

	return ( obj_defined );
}

void printAlAnimChannel(AlChannel *channel)
{
	AlAction	*action;

	fprintf(printFile, "printAlAnimChannel\n");
	if (!channel)
		return;

	const char	*className	= AlObjectClassName(channel);
	AlObject	*item		= channel->animatedItem();
	const char	*field_name	= channel->parameterName();
	int			field		= channel->parameter();

	
#if 0
	fprintf(printFile, "%s%s( ", indentSpace(), className);

	fprintf(printFile, "%s [%s], ", item->name(), AlObjectClassName(item));
	fprintf(printFile, "%s [%d] )\n", field_name, field);
#endif
	for (int i = channel->numAppliedActions(); i >= 1; i--)
	{
		if (!strcmp(field_name, "X Translate"))
		{
			fprintf(printFile, "This channel is X Translate\n");
		}
		else if (!strcmp(field_name, "Y Translate"))
		{
			fprintf(printFile, "This channel is Y Translate\n");
		}
		else if (!strcmp(field_name, "Z Translate"))
		{
			fprintf(printFile, "This channel is Z Translate\n");
		}
		else
		{
			fprintf(printFile, "other channel: %s\n", field_name);
		}
		action = channel->appliedAction(i);
#if 0
		printAlAnimObject(action);
		if (action->type() == kMotionActionType)
			printAlAnimMotionAction(action->asMotionActionPtr());
		else
			printAlAnimParamAction(action->asParamActionPtr());
#endif
	}
}

void printAlAnimAction(AlAction *action)
{
	int		 	num;
	AlChannel	*channel;
	AlObject	*item;

#if 0
	fprintf(printFile, "printAlAnimAction\n");
#endif
	if (!action)
		return;

	const char *	className = AlObjectClassName(action);
	const char *	name = action->name();

	fprintf(printFile, "%s%s(%s)\n", indentSpace(), className, name);
	setIndentation(INDENT);

	fprintf(printFile, "%sComment: %s\n", indentSpace(), action->comment());
	fprintf(printFile, "%sPre-extrap type: ", indentSpace());
	printExtrapType(action->extrapTypePRE());
	fprintf(printFile, "\n%sPost-extrap type: ", indentSpace());
	printExtrapType(action->extrapTypePOST());
	fprintf(printFile, "\n");
	num = action->numChannelReferences();
	fprintf(printFile, "%sNum channels using action: %d\n", indentSpace(), num);

	setIndentation(INDENT);
	for (int i = num; i > 0; i--)
	{
		channel = action->channelReference(i);
		item = channel->animatedItem();
		fprintf(printFile, "%sChannel %d: %s:%s (%d)\n", indentSpace(),
			i, item->name(), channel->parameterName(), channel->parameter());
	}
	setIndentation(-INDENT);

#if 0
	switch(action->type())
	{
		case kParamActionType:
			printAlAnimParamAction(action->asParamActionPtr());
			break;
		case kMotionActionType:
			printAlAnimMotionAction(action->asMotionActionPtr());
			break;
	}
#endif
	setIndentation(-INDENT);
}

void printAlAnimParamAction(
	double			**ptr,
	AlParamAction	*action)
{
	AlKeyframe	*keyframe;
	double	s1, s2, base, *data;
	int numkey;

#if 0
	fprintf(printFile, "# printAlAnimParamAction\n");
#endif
	/* fprintf(printFile, "%sParam action keyframes:\n", indentSpace()); */
	setIndentation(INDENT);
	numkey = action->numberOfKeyframes();
	data = (double *)malloc(sizeof(double) * numkey * 2 + 1);
	*ptr = data;
	*data++ = numkey;
	keyframe = action->firstKeyframe();
	base = keyframe->location();
	base = base / FRAMERATE;
	for (keyframe = action->firstKeyframe();
		 keyframe;
		 keyframe = keyframe->next())
	{
		s1 = keyframe->inTangent();
		/* fprintf(printFile, "# %s%g\n", indentSpace(), s1); */
		s1 = keyframe->outTangent();
		/* fprintf(printFile, "# %s%g\n", indentSpace(), s1); */
		s1 = keyframe->location();
		*data++ = s1/FRAMERATE;
		/* fprintf(printFile, "# %s%g\n", indentSpace(), s1/FRAMERATE); */ 
		/* fprintf(printFile, "%s%g\n", indentSpace(), s1); */
	}
	for (keyframe = action->firstKeyframe();
		 keyframe;
		 keyframe = keyframe->next())
	{
		s2 = keyframe->inTangent();
		/* fprintf(printFile, "# %s%g\n", indentSpace(), s1); */
		s2 = keyframe->outTangent();
		/* fprintf(printFile, "# %s%g\n", indentSpace(), s1); */
		s2 = keyframe->value();
		*data++ = s2;
		/* fprintf(printFile, "%s%g\n", indentSpace(), s2); */
	}
#if 0
	for (keyframe = action->firstKeyframe();
		 keyframe;
		 keyframe = keyframe->next())
	{
		printAlAnimKeyframe(keyframe);
	}
#endif
	setIndentation(-INDENT);
}

void printAlAnimMotionAction(
	AlMotionAction	*action)
{
	AlCurveNode	*curve;

#if 0
	fprintf(printFile, "printAlAnimMotionAction\n");
#endif
	curve = action->motionCurve();
	fprintf(printFile, "%sMotion action curve:\n", indentSpace());
	setIndentation(INDENT);
	fprintf(printFile, "%s%s\n", indentSpace(), curve->name());
	setIndentation(-INDENT);
}

void printAlAnimKeyframe(
	AlKeyframe	*keyframe)
{
	double	s1, s2;

#if 0
	fprintf(printFile, "printAlAnimKeyframe\n");
#endif
	if (!keyframe)
		return;

	const char *	className = AlObjectClassName(keyframe);

	fprintf(printFile, "%s%s", indentSpace(), className);
	setIndentation(INDENT);

	s1 = keyframe->location();
	s2 = keyframe->value();
	fprintf(printFile, "(%.1f, %.1f) ", s1, s2);

	fprintf(printFile, "in-tan: %.1f, out-tan: %.1f, locked? %s\n",
			keyframe->inTangent(), keyframe->outTangent(),
			(keyframe->isLocked() ? "TRUE" : "FALSE"));

	setIndentation(-INDENT);
}

/*
** Print SDF name&type from Alias object 
** If the object is already defined then the object name
** will be tagged with a special character 'use'. 
** The return value out of this says whether it is
** defined before ( = 1 ) or this is the first time ( = 0 )
*/
int
SDF_PrintAnimObjectName( AlObject *object )
{
	static int nodeNum = 0;
	int obj_defined = 0;
	int is_char = 0;

#if 0
	fprintf(printFile, "# printAlAnimObjectName\n");
#endif
	if ( NULL == object ) return ( obj_defined );

	const char*	className;
	if ( NULL != (className = AlObjectClassName( object )) )
	{
		const char* name = object->name();
		char temp_name[ 80 ], mod_name[ 80 ];
		const char *new_name;

		if ( NULL != name ) sprintf( temp_name, "%s", name );
		else 
		{
			sprintf( temp_name, "object_%d", nodeNum );
			nodeNum++;
		}

#if 0
		if( (object->type()==20) || (object->type()==32) )  
		fprintf(stdout, "......... %s (0x%x) = 0x%x, %s\n", name, object->name(),
								object, className );
#endif
		obj_defined = SDF_ObjDefined( object, &is_char, temp_name );

		if ( obj_defined ) // object is already defined
		{
			fprintf( printFile, "%sTarget Use %s\n", indentSpace(), 
								SDF_LegalName( &temp_name[4] ) );
		} else {                  // object is not defined yet
#if 0
			if( !is_char )
			{
				sprintf( mod_name, "use_%s", temp_name );	
				object->setName( mod_name );
				fprintf( printFile, "%sDefine %s %s {\n", indentSpace(), 
								className, 
								SDF_LegalName ( temp_name ) );
			} else
			fprintf( printFile, "%s %s {\n", indentSpace(), 
								className );
#else
			sprintf( mod_name, "use_%s", temp_name );	
			object->setName( mod_name );
			// for some reason setName sometimes sets a different name !!
			new_name = object->name();
			fprintf( printFile, "%sDefine %s %s%s {\n", indentSpace(), 
								className, 
								prefixStr,
								SDF_LegalName ( &new_name[4] ) );
#endif
		}
	}

	return ( obj_defined );
}


