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
//
//+

//
//  .NAME AlVolumeLight - Encapsulates the functionality common to all volume lights.
//
//  .SECTION Description
//
//		There are two ways that this class is used.  If the user
//		instantiates and calls the create method on an AlVolumeLight,
//		the resulting object is a box volume light. This light has
//		all the special effects parameters in a cubical volume.
//
//		Alternatively, the  user could instantiate and create an
//		AlSphereLight, AlCylinderLight, AlTorusLight, AlConeLight or AlBoxLight
//		(which are all considered "volume" and which are all derived from
//		this light class).  
//
//		Note that volume lights differ in thir shape, the way that their
//		direction components are interpreted with respect to that shape,
//		and the way that dropoff and decay are applied.
//		


#ifndef _AlVolumeLight
#define _AlVolumeLight

#include <AlNonAmbientLight.h>

class AlLightNode;
class AR_VolumeLightInfo;

class AlVolumeLight: public AlLight {

public:
	virtual					~AlVolumeLight();

	virtual	AlObject*		copyWrapper() const;

	virtual	AlObjectType	type() const;
	AlVolumeLight*			asVolumeLightPtr();	

	boolean					shadows() const;
	boolean					specular() const;
	boolean					turbulenceAnimated() const;
	boolean					turbulenceDirectional() const;
	int						turbulenceSpaceRes() const;
	int						turbulenceTimeRes() const;
	double					intensity() const;
	double					decay() const;
	double					decayStart() const;
	double					directionality() const;
	double					concentric() const;
	double					directional() const;
	double					radial() const;
	double					dropoff() const;
	double					dropoffStart() const;
	double					turbulenceIntensity() const;
	double					turbulenceSpread() const;
	double					turbulencePersistance() const;
	double					turbulenceGranularity() const;
	double					turbulenceRoughness() const;
	double					turbulenceVariability() const;

	statusCode				setShadows(boolean);
	statusCode				setSpecular(boolean);
	statusCode				setTurbulenceAnimated(boolean);
	statusCode				setTurbulenceDirectional(boolean);

	statusCode				setTurbulenceSpaceRes(int);
	statusCode				setTurbulenceTimeRes(int);

	statusCode				setIntensity( double );
	statusCode				setDecay(double);
	statusCode				setDecayStart(double);
	statusCode				setDirectionality(double);
	statusCode				setConcentric(double);
	statusCode				setDirectional(double);
	statusCode				setRadial(double);
	statusCode				setDropoff(double);
	statusCode				setDropoffStart(double);
	statusCode				setTurbulenceIntensity(double);
	statusCode				setTurbulenceSpread(double);
	statusCode				setTurbulencePersistance(double);
	statusCode				setTurbulenceGranularity(double);
	statusCode				setTurbulenceRoughness(double);
	statusCode				setTurbulenceVariability(double);

protected:

	// This class is a pure virtual class.  It cannot
	// be instantiated.
	//
    						AlVolumeLight();
};
#endif
