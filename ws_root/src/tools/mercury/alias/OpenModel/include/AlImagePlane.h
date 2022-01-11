/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//+
*/


/*
//
//  .NAME AlImagePlane - image planes.
//
//  .SECTION Description
//		Image planes are full colour images that are attached to a camera.
//		If the view from that camera is rendered, they get rendered into the
//		background of the resulting image.  This class deals with setting the
//		various parameters of image planes and their associations to cameras.
//		
//		To create an image plane, you must use the addImagePlane function
//		of AlCamera.  That function will return to you a new AlImagePlane
//		object that you can use to set the image plane's parameters.  Given
//		a pointer to an AlImagePlane, you can delete it from Alias using the
//		deleteObject method.
//
//		To walk the list of all the image planes in Alias, use 
//		AlUniverse::firstImagePlane.  You can move from image plane to image
//		plane in the current stage, or in a camera, by using the appropriate
//		next method.
//
//		For more information about image planes in general, see the 
//		online documentation for the camera editor.
//
*/

#ifndef _AlImagePlane
#define _AlImagePlane

#include <AlStyle.h>

#ifndef __cplusplus
	/* C version of the structures */
    typedef enum {
		AlImagePlane_kDisplayOff,
		AlImagePlane_kDisplayUnknown,
		AlImagePlane_kRGB,
		AlImagePlane_kColourMap,
		AlImagePlane_kOutline,
		AlImagePlane_kFilled
	} AlDisplayMode;

    typedef enum {
		AlImagePlane_kFrameOff,
		AlImagePlane_kRegular,
		AlImagePlane_kKeyframe
	} AlFrameType;

    typedef union {
		struct {
			int start;
			int end;
			int by;
			int offset;
		} regular;
		struct { 
			int frame;
			double offset;
		} keyframe;
	} AlFrameTypeParams;

    typedef enum {
		AlImagePlane_kDepthOff,
		AlImagePlane_kDepthUnknown,
		AlImagePlane_kPriority
	} AlDepthType;

    typedef enum {
		AlImagePlane_kMaskOff,
		AlImagePlane_kMaskUnknown,
		AlImagePlane_kImage,
		AlImagePlane_kFile,
		AlImagePlane_kChromaKey
	} AlMaskType;

    typedef union {
		struct {
			const char 			*name;
			AlFrameType 		frameType;
			AlFrameTypeParams	*params;
		} file;
		struct {
			double 			r;
			double 			g;
			double 			b;
			double 			hue_range;
			double 			sat_range;
			double 			val_range;
			double 			threshold;
		} chroma_key;
	} AlMaskTypeParams;

    typedef enum {
		AlImagePlane_kScreen,
		AlImagePlane_kFields,
		AlImagePlane_k01,
		AlImagePlane_kWorld
	} AlImageSpaceType;

    typedef union {
		struct {
			int				size;
		} fields;
		struct {
			int 			origin_x;
			int				origin_y;
			int				size_x;
			int				size_y;
		} screen;
		struct {
			double			tran_x;
			double			tran_y;
			double			size_x;
			double 			size_y;
		} zero_one;
		struct {
			double			tran_x;
			double			tran_y;
			double			size_x;
			double			size_y;
			double			pivot_x;
			double			pivot_y;
		} world;
	} AlImageSpaceTypeParams;

    typedef struct {
		boolean				wrap_horiz;
		boolean				wrap_vert;
		int 				offset_x;
		int					offset_y;
		int					coverage_x;
		int					coverage_y;
	} AlImageSpaceParams;
#endif

#ifdef __cplusplus
#include <AlObject.h>
#include <AlAnimatable.h>
#include <AlPickable.h>

class AlImagePlane 	: public AlObject
					, public AlAnimatable
					, public AlPickable
{
	friend					class AlFriend;

public:
    enum AlDisplayMode {
		kDisplayOff,
		kDisplayUnknown,
		kRGB,
		kColourMap,
		kOutline,
		kFilled
	};

    enum AlFrameType {
		kFrameOff,
		kRegular,
		kKeyframe
	};

    union AlFrameTypeParams {
		struct {
			int start;
			int end;
			int by;
			int offset;
		} regular;
		struct { 
			int frame;
			double offset;
		} keyframe;
	};

    enum AlDepthType {
		kDepthOff,
		kDepthUnknown,
		kPriority
	};

    enum AlMaskType {
		kMaskOff,
		kMaskUnknown,
		kImage,
		kFile,
		kChromaKey
	};

    union AlMaskTypeParams {
		struct {
			const char 			*name;
			AlFrameType 		frameType;
			AlFrameTypeParams	*params;
		} file;
		struct {
			double 			r;
			double 			g;
			double 			b;
			double 			hue_range;
			double 			sat_range;
			double 			val_range;
			double 			threshold;
		} chroma_key;
	};

    enum AlImageSpaceType {
		kScreen,
		kFields,
		k01,
		kWorld
	};

    union AlImageSpaceTypeParams {
		struct {
			int				size;
		} fields;
		struct {
			int 			origin_x;
			int				origin_y;
			int				size_x;
			int				size_y;
		} screen;
		struct {
			double			tran_x;
			double			tran_y;
			double			size_x;
			double 			size_y;
		} zero_one;
		struct {
			double			tran_x;
			double			tran_y;
			double			size_x;
			double			size_y;
			double			pivot_x;
			double			pivot_y;
		} world;
	};

    struct AlImageSpaceParams {
		boolean				wrap_horiz;
		boolean				wrap_vert;
		int 				offset_x;
		int					offset_y;
		int					coverage_x;
		int					coverage_y;
	};

public:

							AlImagePlane();
							~AlImagePlane();

	virtual AlObject*		copyWrapper() const;
	virtual statusCode		deleteObject();

	virtual AlObjectType	type() const;
	virtual AlImagePlane*	asImagePlanePtr();
	virtual AlAnimatable*	asAnimatablePtr();
	virtual AlPickable*		asPickablePtr();

	virtual const char*		name() const;
	virtual statusCode		setName( const char * );

	AlImagePlane*			next() const;
	AlImagePlane*			nextInCamera() const;
	AlImagePlane*			prev() const;
	AlImagePlane*			prevInCamera() const;

	statusCode				nextD();
	statusCode				nextInCameraD();
	statusCode				prevD();
	statusCode				prevInCameraD();

	const char*				imageFile();
	statusCode				setImageFile( const char * );

	statusCode				displayMode( AlDisplayMode & );
	statusCode				setDisplayMode( AlDisplayMode );

	statusCode				RGBMult( double&, double&, double&, double& );
	statusCode				setRGBMult( double, double, double, double );

	statusCode				RGBOffset( double&, double&, double&, double& );
	statusCode				setRGBOffset( double, double, double, double );

	statusCode				frameType( AlFrameType&, AlFrameTypeParams* = NULL );
	statusCode				setFrameType( AlFrameType, AlFrameTypeParams* = NULL );

	// The next four functions are not fully supported.
	statusCode				maskType( AlMaskType&, AlMaskTypeParams* = NULL );
	statusCode				setMaskType( AlMaskType, AlMaskTypeParams* = NULL );
	statusCode				maskInvert( boolean& );
	statusCode				setMaskInvert( boolean );

	statusCode				depthType( AlDepthType&, double* = NULL );
	statusCode				setDepthType( AlDepthType, double* = NULL );

	// The next two functions are not fully supported.
	statusCode				imageSpace( AlImageSpaceType, AlImageSpaceTypeParams* = NULL, AlImageSpaceParams* = NULL );
	statusCode				setImageSpace( AlImageSpaceType, AlImageSpaceTypeParams* = NULL, AlImageSpaceParams* = NULL );

	//
	//	use kScreen X/Y origin to set the position of the image in pixels
	//	Use these to set the pivot (in pixels)
	//
	statusCode				pivot( double&x, double &y);
	statusCode				setPivot( double x, double y);


private:
	virtual	boolean extractType( int&, void*&, void*& ) const;

	static void				initMessages( void );
	static void				finiMessages( void );
};
#endif /* __cplusplus */

#endif /* _AlImagePlane */
