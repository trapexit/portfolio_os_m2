/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  rotected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
//
//  .NAME AlUnits - interface to units
//
//  .SECTION Description
//		This class provides access to linear and angular units in the Alias api.
//		Note that all of the internal geometry routines work in centimeters
//		and degrees.  If you wish to use other units for distance tolerances
//		and lengths, they must be explicity to centimeters.  Similarly
//		all angles will have to be converted to degrees.
//	
*/

#ifndef	_al_units
#define _al_units

#ifdef __cplusplus

#include <statusCodes.h>

class AlUnits
{
public:
	enum Type
	{
		kUnit, kSubunit, kPosition
	};

	enum LinearUnit
	{
		kMiles, kYards, kFeet, kInches,
		kCentimeters, kMeters, kKilometers, kMillimeters
	};
	enum AngularUnit
	{
		kRadians, kDegrees, kMinutes, kSeconds
	};

	static AngularUnit	angularUnits( Type type );
	static double		angularScale( Type type );

	static LinearUnit	linearUnits( Type type );
	static double		linearScale( Type type );

	static double		linearInCM( LinearUnit unit );
	static double		angularInDegrees( AngularUnit unit );

	static statusCode	parseStringAngular( const char* string, char* &ret, const char* format=0 );
	static statusCode	parseStringLinear( const char* string, char* &ret, const char* format=0 );
	static statusCode	parseStringNoUnits( const char* string, char* &ret, const char* format=0 );
};
#endif

#endif
