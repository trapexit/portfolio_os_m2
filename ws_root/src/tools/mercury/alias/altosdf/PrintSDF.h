
/*
**	File:		PrintSDF.h
**
**	Contains:	Function prototypes	
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
**
**	To Do:
*/

#include	<AlAnim.h>
#include	<AlTesselate.h>

#ifdef __cplusplus

extern "C" {

#endif

void 	SDF_SetColorFlag(int flag);
int 	SDF_GetColorFlag(void);
void	BEGIN_SDF( FILE* os, char *sdump );
void	WRITE_SDF( FILE* os, char *sdump );
void	END_SDF( FILE* os, char *sdump );
int 	SDF_PrintAlMatData(AlShader *shader, int *count);
int 	SDF_PrintAlTexData(AlShader *shader, int *count);
void	SDF_SetPrefix( const char *pfx );
void	SDF_SetTexRefOutput( int texRef );
void	SDF_SetLightInfo( int lightinfo );
void	SDF_SeparateLightMaterialInfo( int lightmaterialinfo );
void	SDF_HasAnim( int animinfo );
void	SDF_PrintAlPolysetNode( AlPolysetNode* );
void	SDF_PrintAlFaceNode ( AlFaceNode *faceNode );
void	SDF_PrintAlGroupNode ( AlGroupNode *groupNode );
void	SDF_PrintAlSurfaceNode ( AlSurfaceNode *surfaceNode );
int		SDF_PrintObjectName ( AlObject *object );
void	SDF_PrintAlPolyset( AlPolyset* );
void	SDF_PrintAlPolygon( AlPolygon* , boolean twosided);
void	SDF_PrintAlTexture(AlTexture *texture, int *count );

void	printAlUniverse( char *fname );
void	setPrintFile( FILE* );
void	setScriptFile( FILE* );
FILE*	getPrintFile( void );
int		getIndentation ( void );
char*	indentSpace ( void );
void	setIndentation ( int space );
const char* AlObjectClassName ( AlObject *object );
void	printAlObject ( AlObject *object );
void	printAllAlObjects ( AlObject *object );
void	printAlDagNode ( AlDagNode *dagNode );
void	printAlSurface ( AlSurface *surface );
void	printAlCurvesOnSurface( AlSurface *surface );
void	printAlCurveOnSurface( AlCurveOnSurface *cos );
void	printAlSurfaceCV( AlSurfaceCV * );
void    printAlSurfaceTrimRegions ( AlSurface *surface );
/* void    printAlTrimRegion ( AlNewTrimRegion *region ); */
/* void    printAlTrimBoundary ( AlNewTrimBoundary *boundary ); */
/* void    printAlTrimCurve ( AlNewTrimCurve *curve ); */
void    printAlTrimRegion ( AlTrimRegion *region );
void    printAlTrimBoundary ( AlTrimBoundary *boundary );
void    printAlTrimCurve ( AlTrimCurve *curve );
void	printAlCamera ( AlCamera *camera );
void	printAlCameraNode ( AlCameraNode *cameraNode );
void	printAlLight( AlLight *light );
void	printAlAmbientLight ( AlAmbientLight *light );
void	printAlPointLight ( AlPointLight *light );
void	printAlDirectionLight ( AlDirectionLight *light );
void	printAlSpotLight ( AlSpotLight *light );
void	printAlLinearLight ( AlLinearLight *light );
void	printAlAreaLight ( AlAreaLight *light );
void	printAlLightNode ( AlLightNode *lightNode );
void	printAlClusterNode ( AlClusterNode *clusterNode );
void	printAlCluster ( AlCluster *cluster );

void	printAlSetName ( AlSet *set );
void	printAlSetNames ();

void	printAllChannels(void);
void	printAlChannel(AlChannel *channel);
void	printAllActions(void);
void	printAlAction(AlAction *action);
void	printAlParamAction(AlParamAction *action);
void	printAlMotionAction(AlMotionAction *action);
void	printExtrapType(AlActionExtrapType);
void	print_component(AlTripleComponent);
void	printAlKeyframe(AlKeyframe *keyframe);
void	printAlStream(AlStream *stream);

void	printAllShading(void);
void	printAlEnvironment(AlEnvironment *env);

void	printAlPolysetVertex( AlPolysetVertex* );

void    setTessPolyType( AlTesselateTypes tp );
void    setTessQuality( double tq );

/* prototype for keyframe animation */
void	printAlUniverseAnim( char *fname );
void	printAlAnimObject ( AlObject *object );
void	SDF_PrintAlAnimGroupNode ( AlGroupNode *groupNode );
void	SDF_PrintAlAnimSurfaceNode( AlSurfaceNode *surfaceNode );
void	SDF_PrintAlAnimPolysetNode( AlPolysetNode *polysetNode );
int		SDF_PrintAlAnimDagNodeInfo( AlDagNode *dagNode );
void	printAlAnimChannel(AlChannel *channel);
void	printAlAnimAction(AlAction *action);
void	printAlAnimParamAction(double **ptr, AlParamAction   *action);
void	printAlAnimMotionAction(AlMotionAction  *action);
void	printAlAnimKeyframe(AlKeyframe  *keyframe);
int		SDF_PrintAnimObjectName( AlObject *object );


#ifdef __cplusplus

}

#endif
