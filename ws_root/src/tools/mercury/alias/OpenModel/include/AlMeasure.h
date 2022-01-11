//-
//  Copyright (C) 1995, Alias|Wavefront
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
//  .NAME AlMeasure - Support for measurement of distances between objects
//
//  .SECTION Description
//		The following are a series of overloaded functions named
//		minDist().  These can be used to find the closest points
//		between any two surfaces, curves, and/or points.
//
//		The minDist() functions will return a negative value if and
//		only if the objects it is given do not exist.
//
//		For the sake of continuity, the order of the two objects to
//		be measured is always highest dimentionality to lowest.
//


class AlMeasure
{
public:
	static double minDist( AlSurface* surface1, AlSurface* surface2,
				  double* u1=NULL, double* v1=NULL, double P1[]=NULL,
				  double* u2=NULL, double* v2=NULL, double P2[]=NULL,
				  int interval_u=20, int interval_v=20);

	static double minDist( AlSurface* surface1, AlCurve* curve2,
                  double* u1=NULL, double* v1=NULL, double P1[]=NULL,
                  double* t2=NULL, double P2[]=NULL,
                  int interval=100);

	static double minDist( AlSurface* surface1, double P2[3],
				  double* u1=NULL, double* v1=NULL, double P1[]=NULL);

	static double minDist( AlCurve* curve1, AlCurve* curve2,
				  double* t1=NULL, double P1[]=NULL,
				  double* t2=NULL, double P2[]=NULL, int inter=100);

	static double minDist( AlCurve* curve1, double P2[3],
				  double* t1=NULL, double P1[]=NULL);
};
