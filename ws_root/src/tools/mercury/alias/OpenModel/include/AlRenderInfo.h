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
//	.NAME AlRenderInfo - A structure used to transfer render information 
//	of an object.
//
//	.SECTION Description
//		An AlRenderInfo structure is used to store render information 
//		for an object. Before setting the AlRenderInfo for an object it
//		is necessary to retrieve the current values from the object.
//
// .br
//		For example:
//
// .nf
//		// No error checking is done here for brevity in the example.
//		AlRenderInfo	renderInfo;
//		newSurface->renderInfo( renderInfo );
//		renderInfo.castsShadow = FALSE;
//		newSurface->setRenderInfo( renderInfo );
//
// .fi
//		See Render->globals in the Alias manuals for a more detailed
//		description of these fields.
//
//	%cdoubleSided
//		This is used to set whether both sides of an
//		object (inside and outside) or only the outside
//		is to be rendered.  For example, a closed object,
//		such as a sphere doesn't have to be double sided;
//		only one side of the object is oging to be seen 
//		when it is rendered. The default is TRUE.
//
//	%copposite
//		This is used to determine which side of a surface 
//		will be used for the render; the side that the 
//		normals point out of, or the opposite. The default
//		is FALSE, meaning the side the normals point out of
//		will be used.
//
//	%ccastsShadow
//		This indicates whether or not the object will cast
//		shadows in the RayCaster and RayTracer. The
//		default is TRUE.
//
//	%cadaptive
//		This indicates whether adaptive or uniform subdivisions
//		will be used. The default is TRUE, meaning adaptive
//		subdivisions.
//
//	%cadaptive_min
//		This indicates the minimum level of adaptive
//		subdivision on the object. The level must be
//		a power of 2 between 0 and 7, any other values
//		will cause the next higher power of 2 to be used.
//		The default it 2.
//							
//	%cadaptive_max
//		This indicates the maximum level of adaptive
//		subdivision on the object. The level must be
//		a power of 2 between 0 and 7, any other values 
//		will cause the next higher power of 2 to be used.
//		The default it 4.
//
//	%ccurvature_threshold
//		This controls the threshold for subdivision
//		of the surface. Values must be between 0
//		and 1, with 0.96 being the default.
//
//	%cuniform_u
//		When "adaptive" is FALSE this indicates the number
//		of subdivisions in the U direction. Values must be
//		between 1 and 256 with 4 being the default.
//
//	%cuniform_v
//		When "adaptive" is FALSE this indicates the number
//		of subdivisions in the V direction. Values must be
//		between 1 and 256 with 4 being the default.
//
//	%csmooth_shading
//		This indicates whether smooth or flat shading
//		should be used. The default is TRUE meaning
//		that smooth shading will be used.
//
//	%cmotion_blur
//		This indicates whether the object should be
//		motion blurred. The default is TRUE.
//
//	%creflection_only
//		This object is a reflection only object.
//
//	%cmotion_blur_texture_sample_level
//	%cmotion_blur_shading_samples
//		Parameters for motion blur.
//
*/

#ifndef _AlRenderInfo
#define _AlRenderInfo

typedef struct AlRenderInfo {

	boolean	doubleSided;			/* == TRUE if this object is double-sided */
									/* (default is TRUE) */
	boolean opposite;				/* == TRUE if this object is opposite */
									/* (default is FALSE) */
	boolean castsShadow;			/* == TRUE if this object casts a shadow */
									/* (default is TRUE) */
    boolean adaptive;               /* adaptive or uniform subdivisions */
                                    /* (default is TRUE) */
    int     adaptive_min;           /* Minimum number of subdiv levels  */
                                    /* (default is 2)    */
    int     adaptive_max;           /* Maximum number of subdiv levels  */
                                    /* (default is 4)    */
    double  curvature_threshold;    /* Curvature threshold for adaptive */
                                    /* (default is 0.96  */
    int     uniform_u;              /* Uniform subdivision level in u   */
                                    /* (default is 4)    */
    int     uniform_v;              /* Uniform subdivision level in v   */
                                    /* (default is 4)    */
    int     smooth_shading;         /* smooth or flat shading           */
                                    /* (default is smooth) */
    int     motion_blur;            /* should object be motion blurred  */
                                    /* (default is TRUE)  */
	boolean	reflection_only;		/* new flag for reflection only objects */

	int		motion_blur_texture_sample_level; /* 0..5 */

	int		motion_blur_shading_samples; /* >= 1  */
} AlRenderInfo;

#endif
