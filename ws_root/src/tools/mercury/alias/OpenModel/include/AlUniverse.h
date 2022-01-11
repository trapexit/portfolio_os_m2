/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	rotected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
*/

/*
//
//	.NAME AlUniverse - Encapsulates the retrieval, data access and storage of Alias Wire Files.
//
//	.SECTION Description
//		This is a static class in that all of its member functions are
//		static.  It provides the foundation on which to interface with
//		Alias Wire Files (OpenModel) or with Alias directly (via LiveData).
//
//		In Alias, there is the concept of a 'stage', an independant entity
//		which encapsulates all the top-level information about a modeling
//		environment.  The stage effectively represents the running version
//		of an individual Wire file.  In the interactive package, stages can
//		be controlled through the Stage Editor.  The identical control also
//		exists in the AlUniverse class, via the Stage functions.  These let
//		you switch between stages, create and destroy stages, merge stages,
//		and alter their features.
//
//		LiveData and OpenModel use object data which defines, among other
//		things, transformations, curve and surface geometry, cameras and lights.
//		Transformations are kept in objects called "dag nodes".
//		Dag nodes can be connected to other dag nodes to form a list of
//		dag nodes.  There are particular types of dag nodes which refer
//		specifically to geometry, cameras, lights or refer to another
//		list of dag nodes to form a hierarchical structure.  Together,
//		these different types of dag nodes can be used to build a list
//		of hierarchical structures which is collectively called a
//		directed acyclic graph or "DAG". 
//
//		The AlUniverse class gives access to the first dag node in the DAG,
//		and from that dag node, the list of all dag nodes in the universe
//		can be traversed.
//
//		The object data may also contain animation information on various
//		animatable items, such as a dag node or a piece of geometry.
//		The animation information can be accessed through the item that
//		is animated.  The AlUniverse class also gives access to animation
//		data through global lists.
//
//		Geometry will also contain shading information.  Each surface
//		or face (a particular kind of curve) will contain a list of
//		shaders.  The AlUniverse class also gives access to a global list of
//		all shaders in the universe, as well as the single environment class.
//
//		The AlUniverse class also gives access to the list of sets in the
//		universe via the firstSet() method.  Similarly, the firstCluster()
//		method will give access to all the clusters in the universe.
//		However, clusters are also accessible through methods in the
//		classes which can belong to clusters (i.e. AlDagNode, AlCurveCV and
//		AlSurfaceCV).  A cluster is an object in the DAG which has a collection
//		of dag nodes and/or CVs.  The transformations in the dag nodes
//		above the cluster are applied to the CVs in the cluster after the
//		latter's own transformations have been applied.
//
//		AlUniverse provides base access to the ImagePlane list and
//		the Window list through firstImagePlane, firstWindow and related
//		functions.
//
//		Whenever a new Alias Object of any kind is created, it is automatically
//		added to the appropriate place in the universe.  For example, if
//		a dag node is created, it will be traversable through the
//		AlUniverse::firstDagNode() method.  If a new animation action is
//		created, it will be accessable by walking the list of actions via
//		AlUniverse::firstAction()/nextAction().
//
//		However, there are a few exceptions.
//		The AlCurve, AlFace, AlSurface and AlKeyframe classes can all be
//		created and will not be part of the universe initially.  They will
//		become part of the universe when they are added to other objects.
//		For example, an AlCurve will become part of the universe when its
//		AlCurveNode is created.  An AlKeyframe will become part of the
//		universe when its AlParamAction is created.  If a class is not
//		part of the universe, this means that if AlUniverse::store() is
//		called, that class will not be stored.  If a LiveData application
//		exits without making an object part of the universe, then the data
//		will be lost and wasted memory will result.
//
//		The initialize method must be called before any other AlUniverse
//		method is called.  An Alias AlObject cannot be created until the
//		initialize method is called.
//
*/

#ifndef _AlUniverse
#define _AlUniverse

/*
 * window refresh flags for LiveData
 */ 

#define kRedrawInactive 001
#define kRedrawActive   002
#define kRedrawTemplate 004
#define kRedrawWindows	010
#define kRedrawListers	020
#define kRedrawNone     0
#define kRedrawAll  (kRedrawInactive | kRedrawActive | kRedrawTemplate | kRedrawWindows | kRedrawListers )

typedef enum {
    kFromPreviewWindow,
    kMinMax,
    kGlobal
} AlFrameRangeType;

#ifndef __cplusplus 

    typedef enum {
		AlUniverse_kUnknown,  

		AlUniverse_kWire,	 
		AlUniverse_kIges,	
		AlUniverse_kVdais,
		AlUniverse_kC4x,
		AlUniverse_kJamais,   
		AlUniverse_kVdafs,	  
		AlUniverse_kDxf,	  

	    AlUniverse_kDes,	  
		AlUniverse_kTri,	  
		AlUniverse_kQuad,     
		AlUniverse_kProRender,
		AlUniverse_kInventor,
	    AlUniverse_kStl,
	    AlUniverse_kObj,
	    AlUniverse_kGeo,
		AlUniverse_kStyle,
		AlUniverse_kEpsf,
		AlUniverse_kIllustrator,
		AlUniverse_kSlc	
	} AlFileType;

	typedef enum {
		AlUniverse_kAnimData,
		AlUniverse_kCanvasData,
		AlUniverse_kDepthData,
		AlUniverse_kEnvironmentData,
		AlUniverse_kLightData,
		AlUniverse_kMaskData,
		AlUniverse_kMiscData,
		AlUniverse_kOptionData,
		AlUniverse_kPixData,
		AlUniverse_kPlotData,
		AlUniverse_kRrfrData,
		AlUniverse_kRibData,
		AlUniverse_kSlaData,
		AlUniverse_kSdlData,
		AlUniverse_kShaderData,
		AlUniverse_kStageData,
		AlUniverse_kTextureData,
		AlUniverse_kWireData
	} AlDirectoryType;

#else
#include <AlCoordinateSystem.h>
#include <AlStyle.h>
#include <AlList.h>
#include <AlMessage.h>

class AlDagNode;
class AlSet;
class AlChannel;
class AlAction;
class AlStream;
class AlShader;
class AlCluster;
class AlEnvironment;
class AlSurface;
class AlObject;
class AlWindow;
class AlImagePlane;

class AlPtrTable;

class AlIterator;
class AlUpdate;

struct Aa_Channel_s;
struct Aa_Action_s;
struct AlRetrieveOptions;

class AlUniverse : public AlMessage
{
	friend					class AlFriend;
	friend					class AlUpdate;

public:
    enum AlFileType {
		kUnknown,  // File type is unknown and will be ignored by ::retrieve()

		kWire,	   // Alias/Wavefront Wire File 
		kIges,	   // Initial Graphics Exchange Standard 
		kVdais,	   // Verband Der Automobilindustrie IGES Subset
		kC4x,	   // EDS/GM format.
		kJamais,   // Japan Automobile Manufacturers Association IGES Subset.
		kVdafs,	   // Verband Der Automobilindustrie Freeform Surface Interface.
		kDxf,	   // Drawing Exchange Format

			// The file types below are available only in OpenAlias.

	    kDes,	   		// EDS/GM Data Exchange Standard
		kTri,	   		// Alias/Wavefront Object Separated Triangle Format
		kQuad,      	// Alias/Wavefront Object Separated Quad Format
		kProRender, 	// Pro/Engineer Render File Format.
		kInventor,  	// OpenInventor Metafile Subset V1.0
	    kStl,			// 3D Systems SLA Format
	    kObj,			// Alias/Wavefront TAV format.
	    kGeo,			// Alias/Wavefront Explore format.
		kStyle,			// Alias/Wavefront Sketch Format.
		kEpsf,			// Adobe Encapsulated Postscript v2.x
		kIllustrator,	// Illustrator Format.
		kSlc			// 3D Systems Slice Format.
	};

	// Project subdirectory types.
	// These indicate the subdirectories where different types of files are
	// located.

	enum AlDirectoryType {
		kAnimData,			// Animation data (Save Anim / Retrieve Anim)
		kCanvasData,		// Paint canvas files
		kDepthData,			// Depth files
		kEnvironmentData,	// Environment files (saved/retrieved via 
							//					  multi-lister)
		kLightData,			// Light files (saved/retrieved via multi-lister)
		kMaskData,			// Mask files
		kMiscData,			// Miscellaneous data files: color, window etc.
		kOptionData,		// UI option files
		kPixData,			// Image files for file textures, image planes etc.
		kPlotData,			// Plot files
		kRrfrData,			// Really Really Fast Render files.
		kRibData,			// Pixar RIB files
		kSlaData,			// Stereo Lithography files (.stl, .slc, .tri)
		kSdlData,			// Scene Description Files
		kShaderData,		// Shader files (saved/retrieved via multi-lister)
		kStageData,			// Wire files representing stages.
		kTextureData,		// Texture files (saved/retrieved via multi-lister)
		kWireData			// Wire files an CAD files (e.g. IGES, DXF, etc.)
	};

public:
	static statusCode		initialize( AlCoordinateSystem = kZUp, 
									    boolean initProjectEnv = FALSE  );
	static boolean			isInitialized();

	static AlCoordinateSystem	coordinateSystem ();

	static statusCode		expandFileName( char[], const char *,
										    AlDirectoryType directoryType );
	static boolean			isWireFile( const char *, char * );

	static statusCode		wireFileWindowSize( int &sizeX, int &sizeY );
	static statusCode		setWireFileWindowSize( int sizeX, int sizeY );

	static AlFileType		fileType( const char * );
	static statusCode       retrieveOptions( AlRetrieveOptions& );
	static statusCode		setRetrieveOptions( const AlRetrieveOptions& );
	static statusCode		retrieve( const char * );
	static statusCode		retrieveStdin();
	static statusCode		store( const char *, AlDagNode* = NULL );
	static statusCode		storeStdout();

	static AlDagNode*		firstDagNode();
	static AlSet*			firstSet();
	static AlCluster*		firstCluster();

	static const char*		currentStage( void );
	static statusCode		setCurrentStage( const char* );

	static statusCode		mergeStage( const char* );
	static statusCode		mergeAllStages( void );

	static statusCode		deleteStage( const char* );
	static statusCode		deleteAllStages( void );

	static statusCode		visibility( const char *, boolean& );
	static statusCode		setVisibility( const char *, boolean );

	static const char*		windowSource( void );
	static statusCode		setWindowSource( const char * );

	static const char*		backgroundSource( void );
	static statusCode		setBackgroundSource( const char * );

	static statusCode		renameStage( const char *, const char * );
	static AlList*			stageNames( void );

	static statusCode		createNewStage( const char * );

	static AlWindow*		firstWindow();
	static AlWindow*		currentWindow();
	static AlWindow*		sbdWindow();

	static AlImagePlane*	firstImagePlane();

	static statusCode		blindData( int, long&, const char*& );
	static statusCode		setBlindData( int, long, const char* );
	static statusCode		removeBlindData( int );

	static AlChannel*		firstChannel();
	static AlChannel*		nextChannel(AlChannel *);
	static statusCode		nextChannelD( AlChannel *);

	static AlAction*		firstAction();
	static AlAction*		nextAction(AlAction *);

	static AlShader*		firstShader();
	static AlShader*		nextShader( AlShader* );
	static statusCode		nextShaderD( AlShader* );
	static AlEnvironment*	firstEnvironment();

	static statusCode		deleteAll();

	static statusCode		redrawScreen( unsigned flags = kRedrawInactive | kRedrawActive );

	static statusCode		applyIteratorToImagePlanes( AlIterator*, int& );
	static statusCode		applyIteratorToWindows( AlIterator *, int& );
	static statusCode		applyIteratorToDagNodes( AlIterator *, int& );
	static statusCode		applyIteratorToActions( AlIterator *, int& );
	static statusCode		applyIteratorToChannels( AlIterator *, int& );
	static statusCode		applyIteratorToSets( AlIterator *, int& );
	static statusCode		applyIteratorToClusters( AlIterator *, int& );
	static statusCode		applyIteratorToShaders( AlIterator *, int& );

	static statusCode		writeSDLNoAnimation( const char *, boolean );
	static statusCode		writeSDL( const char *, boolean, double, double, double );

	static statusCode		frameRange(AlFrameRangeType, double&, double&, double&);
	static double			currentTime();

	static statusCode		doUpdates( boolean = TRUE );

protected:
	static void				setCurrentChannelToNull();
	static void				setCurrentActionToNull();

private:
	static void				initMessages();
	static void				finiMessages();

	static statusCode		retrieveMain( const char * );
	static int				alDeleteObject( AlObject* );

	// Animation private data
	static void				*lastChannel;	// list element for last Aa_Channel
	static void				*lastAction;	// list element for last Aa_Action
	static void				*lastExprChan;	// list element for last expression Aa_Channel
	static boolean			updateOn;
	static boolean			updateNeeded;

	// curve private data
	static AlPtrTable		trimCurveTable;
};

typedef AlUniverse AlStage;

#endif /* __cplusplus */

#endif /* _AlUniverse */
