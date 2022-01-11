/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//+
*/

/*
//
//	.NAME AlRetrieveOptions - A structure used to transfer options that control
//        the AlUniverse::retrieve() method.
//
//	.SECTION Description
//		An AlRetrieveOptions structure is used set or determine the options
//      that the AlUniverse::retrieve() method uses to tailor the import
//		of files.
//		To avoid setting all of the fields of AlRetrieveOptions using
//		AlUniverse::setRetrieveOptions(), it is recommended that the 
//		current values be acquired first using AlUniverse::retrieveOptions().
//
// .br
//		For example:
//
// .nf
//		// No error checking is done here for brevity in the example.
//		AlRetrieveOptions	options;
//		AlUniverse::retrieveOptions( options );
//		options.dxf.units = k_millimeters;
//		options.dxf.in_create_shaders = True;
//		AlUniverse::setRetrieveOptions( options );
//
// .fi
//		See File->Retrieve in the Alias manuals for a more detailed
//		description of these options.
//
//  General Options:
//
//	%cqk_wire
//		This is only relevent to OpenAlias plugins, and determines whether or
//		not all retrieved geometry is displayed in quick-wire mode.
//		Only relevent for OpenAlias plugins.
//
//	%cnew_stage
//		Determines if all retrieved geometry is put in the current stage,
//		or is put in a new stage named after the filename. If set to TRUE, 
//		then the retrieved geometry will be put in a new stage called
//		<filename><ext> (e.g. model.iges -> modeliges) and this stage will
//		be made current.
//
//  Wire File Options:
//
//	    All wire file options default to TRUE.
//
//	%ckeep_windows
//		If set to TRUE, the modelling window layout contained in wire files 
//		will be retrieved, and if set to FALSE, the window layout will not be
//		retrieved.
//		
//	%ckeep_cameras
//		If set to TRUE, the cameras contained in wire files will be retrieved, 
//		and if set to FALSE, the cameras will not be retrieved.
//
//	%ckeep_animation
//		If set to TRUE, both the animation and the model contained in wire 
//		files will be retrieved, and if set to FALSE, only the model will be
//		retrieved.
//
//	Note: If a window contains an animated camera and keep_animation is TRUE
//		  then this window will be retrieved also, even if keep_windows is
//		  FALSE.
//
//	%ckeep_backgrounds
//		If set to TRUE, the background contained in wire files will be 
//		retrieved, and if set to FALSE, the background will not be retrieved.
//		  
//	%ckeep_unitsTolerances
//		If set to TRUE, linear units, angular units, and construction
//		toleranced contained in the wire will be retrieved and set.
//		If set to FALSE, the units and tolerances in the wire file are 
//		ignored. Only relevent for OpenAlias plugins.
//
//	%ckeep_renderGlobals
//		If set to TRUE, the render global information stored in a wire file will
//		be retrieved and set. If set to FALSE, this information will be 
//		ignored. Only relevent for OpenAlias plugins.
//
//  IGES/VDAIS/C4X/JAMA-IS and VDAFS File Options:
//
//	%cgroup
//		If set to TRUE, all geometry retrieved from the file will be
//		grouped under a node named after the translator. For example, IGES
//		geometry will be grouped under a node called "IGES_FILE". and DXF 
//		geometry will be grouped under a node called "DXF_FILE".
//		If set to FALSE, this group node node will not be created.
//		The default is FALSE.
//
//	%ccoalesce
//		If set to TRUE, multiple knots will be removed based on continutity
//		in IGES Parametric Curve and Surface geometry, and all VDAFS curve and
//		surface geometry. The default is FALSE.
//	
//	%cannotation
//		If set to TRUE, supported geometric entities that have been flagged 
//		for use as annotation will be retrieved. If set to FALSE, these entities
//		will not be retrieved. This option does not apply to VDAFS.
//
//	%ctrim_to_surf
//		If set to TRUE, retrieved Trimmed or Bounded surfaces whose boundaries 
//		are the same as, or iso-parametric to, the natural boundaries of the 
//		untrimmed surface, will be converted to untrimmed surfaces by shrinking
//		the	surface to the trim boundaries. If set to FALSE, trimmed surfaces 
//		will be represented as such even if the trim boundary information is 
//		unnecessary. The default is TRUE.  This option does not apply to VDAFS.
//
//	%ctrim_to_face
//		If set to TRUE, retrieved Trimmed or Bounded surfaces that are planar 
//      are converted to FACE geometry from the model-space (3D) boundary 
//		curves of the trimmed/bounded surfaces. If set to FALSE, this processing
//		does not occur, and trimmed surfaces are created. This option does not 
//		apply to VDAFS.
//
//	%cscale
//		All retrieved geometry will be scaled by this factor.
//		The default is 1.0.
//
//
//	DXF File Options.
//
//	%cgroup
// 		See description above.
//
//	%cload_shaders
//		Determines whether or not shaders are to be loaded from the "shader"
//		sub-directory of the current project. If set to TRUE, if there is a
//		shader on disk whose name matches a DXF layer name, then this shader
//		will be loaded (if not already in memory) and assigned to all objects
//		on that layer. If set to FALSE, this process is skipped, and at best, 
//		only simple colour shaders will be created from the DXF colour
//		information. The default is TRUE.
//
//	%canonymous_blocks
//	    Determines whether "anonymous" DXF BLOCK entities are processed (TRUE)
//		or ignored (FALSE). The default is FALSE, since mostly, these BLOCK
//		entities contain pattern hatching lines.
//	
//
//	%c3dface_type
//		This indicates the type of geometry resulting from conversion of
//		DXF 3DFACE, SOLID and TRACE entities. These entities can be 
//		represented as polysets (kPolysetType) or surfaces (kSurfaceType).
//		The default is kPolysetType.
//
//	%cpolyline_type
//		This indicates the type of geometry resulting from the conversion of
//		DXF POLYLINE and LINE entities that have area. These entities can be 
//		represented as polysets (kPolysetType) or surfaces (kSurfaceType).
//		The default is kPolysetType.
//
//	%cunits
//		If the units of the DXF coordinate data is known, it can be set using
//		this option so that the data is properly scaled. This option is
//		necessary since the units of the DXF coordinate data is not stored 
//		in the file. The default is inches (kInches), but it can be set to 
//		any of the following values: kMiles, kFeet, kInches, kMils, 
//		kMicroInches, kKilometers, kMeters, kCentimeters, kMillimeters, or
//		kMicrons.
//
//	%cscale
//		 See above for description of this field.
//
*/

#ifndef _AlRetrieveOptions
#define _AlRetrieveOptions

    enum AlUnitsType {
		kUnknown,  // Unknown unit (should not be used).
		kMicrons,
		kMillimeters,	 
		kCentimeters,	 
		kMeters, 
		kKilometers, 
		kMicroInches,	
		kMils,	
		kInches,	
		kFeet,	
		kMiles
	};

typedef struct AlRetrieveOptions {

/* Options that apply to all file types */

	boolean	qk_wire;					/* == TRUE all retrieved geometry is  */
										/*    displayed in quick-wire mode    */
	                                    /* Note: only applicable to OpenAlias */
										/*       plugins                      */
	                                    /*    (default is FALSE)              */

	boolean	new_stage;					/* == TRUE all retrieved geometry will*/
	                                    /*    placed in a new stage whose name*/
	                                    /*    is based on the filename        */
	                                    /*    (default is FALSE)              */

/* Wire File options */

	struct _wire {
		boolean	keep_windows;  			/* == TRUE windows will be retrieved  */
		                                /*    (default is TRUE) */

		boolean	keep_cameras; 			/* == TRUE cameras will be retrieved  */
		                                /*    (default is TRUE) */

		boolean	keep_animation;			/* == TRUE animation will be retrieved*/
									    /*     (default is TRUE)              */

		boolean	keep_backgrounds;		/* == TRUE background will be         */
										/*    retrieved. (default is TRUE)    */

		boolean	keep_unitsTolerances;	/* == TRUE linear/angular units and   */
		                                /*    construction tolerances will    */
		                                /*    be retrieved and set.           */
		                                /*    (default is TRUE)               */

		boolean	keep_renderGlobals;     /* == TRUE render global settings     */
		                                /*    will be retrieved and set       */
									    /*    (default is TRUE)               */
	} wire;

/* IGES File options */

 	struct _iges {
		boolean	group;			    	/* == TRUE retrieved geometry will be */
										/*    grouped under a node            */
		                                /*    (default is FALSE)			  */

		boolean	coalesce;				/* == TRUE multiple knot removal base */
		                                /*    continutity will occur for      */
		                                /*    retrieved parametric spline     */
										/*    geometry( Entities 112 and 114) */
                                        /*    (default is FALSE)			  */

		boolean	annotation;  			/* == TRUE IGES geometry entities     */
		                                /*    flagged as annotation will be   */
		                                /*    retrieved.                      */
		                                /*    (default is FALSE)              */

        boolean	trim_to_surf;      	    /* == TRUE IGES Trimmed and Bounded   */
		                                /*    surfaces (entities 144 and 143) */
		                                /*    will be represented as untrimmed*/
		                                /*    surfaces if possible            */
		                                /*    (default is TRUE)               */

		boolean	trim_to_face;      	    /* == TRUE Planar IGES Trimmed and    */
		                                /*    Bounded surfaces will be        */
		                                /*    represented as FACE geometry    */
		                                /*    (default is FALSE)              */

		float	scale;				    /*    All retrieved geometry will be  */
		                                /*    scaled by this factor.          */
		                                /*    (default is 1.0)                */
	} iges;

/* VDA-IS File options  - see IGES options for field descriptions */
	struct _vdais {
		boolean	group;
		boolean	coalesce;
		boolean	annotation;
        boolean	trim_to_surf;
		boolean	trim_to_face;
		float	scale;
	} vdais;

/* C4X File options  - see IGES options for field descriptions */
    struct _c4 {
		boolean	group;
		boolean	coalesce;
		boolean	annotation;
        boolean	trim_to_surf;
		boolean	trim_to_face;
		float	scale;
    } c4;

/* JAMA-IS File options  - see IGES options for field descriptions */
    struct _jamais {
		boolean	group;
		boolean	coalesce;
		boolean	annotation;
        boolean	trim_to_surf;
		boolean	trim_to_face;
		float	scale;
    } jamais;

/* VDAFS File options  - see IGES options for field descriptions */

	struct _vdafs {
		boolean	group;
		boolean	coalesce;
		float	scale;
	} vdafs;

/* DXF File options */
	struct _dxf {
		boolean	group;			    	/* See IGES description of this field */

		boolean	load_shaders;           /* == TRUE attempt to load shaders    */
		                                /*    corresponding to layers         */
		                                /*    (default is TRUE)               */

		boolean	anonymous_blocks;       /* == TRUE retrieve DXF Anonymous     */
		                                /*    blocks (default is FALSE)       */

		int	 	solid_trace_3dface_type;/* == kPolysetType create polyset     */
		                                /*    geometry from 3DFACE, SOLID,    */
		                                /*    and TRACE entities.             */
		                                /* == kSurfaceType create surface     */
		                                /*    geometry for 3DFACE/SOLID/TRACE */
		                                /*    (default is kPolysetType)       */

		int	 	line_polyline_type;     /* == kPolysetType create polyset     */
		                                /*    geometry from POLYLINE or LINE  */
		                                /*    entities that describe surfaces */
		                                /* == kSurfaceType create surface     */
		                                /*    geometry from POLYLINE or LINE  */
		                                /*    entities that describe surfaces */
		                                /*    (default is kPolysetType)       */

		int		units;					/* == units of the DXF coordinate data*/
		                                /*    one of kMiles, kFeet, kInches,  */
		                                /*    kMils, kMicroInches,            */
		                                /*    kKilometers, kMeters,           */
		                                /*    kCentimeters, kMillimeters, or  */
		                                /*    kMicrons.						  */
		                                /*    (default is kInches)            */

		float	scale;					/* See IGES description of this field */
	} dxf;
		
} AlRetrieveOptions;

#endif
