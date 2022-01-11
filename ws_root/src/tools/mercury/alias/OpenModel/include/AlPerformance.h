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
//	.NAME AlPerformance - Interface to Alias performance options
//
//	.SECTION Description
//		This class encapsulates all access to the Alias performance
//		options.  Every value in the window can be retrieved and set.
//
*/

#ifndef _AlPerformance_h
#define _AlPerformance_h

#include <AlStyle.h>

#ifndef __cplusplus
	typedef enum {
		AlPerformance_kActual,
	   	AlPerformance_kScreenSize,
  	 	AlPerformance_kMedium,
	   	AlPerformance_kLow,
  	 	AlPerformance_kConnectedPoints,
	   	AlPerformance_kBoundary,
  	    AlPerformance_kBoundingBox
	} AlPrecisionType;

#else 

class AlPerformance
{
public:

	enum AlPrecisionType {
		kActual,
		kScreenSize,
		kMedium,
		kLow,
		kConnectedPoints,
		kBoundary,
		kBoundingBox
	};

public:
	static AlPrecisionType	redrawPrecision( void );
	static void				setRedrawPrecision( AlPrecisionType );

	static AlPrecisionType	motionPrecision( void );
	static void				setMotionPrecision( AlPrecisionType );


	static boolean 		drawTrimBoundaries( void );
	static void			setDrawTrimBoundaries( boolean );

	static boolean 		trimBoundariesDuringPlayback( void );
	static void			setTrimBoundariesDuringPlayback( boolean );


	static boolean		expressionsAfterModification( void );
	static void			setExpressionsAfterModification( boolean );

	static boolean		expressionsDuringXform( void );
	static void			setExpressionsDuringXform( boolean );

	static boolean		expressionsDuringPlayback( void );
	static void			setExpressionsDuringPlayback( boolean );


	static boolean		constraintsAfterModification( void );
	static void			setConstraintsAfterModification( boolean );

	static boolean		constraintsDuringXform( void );
	static void			setConstraintsDuringXform( boolean );

	static boolean		constraintsDuringPlayback( void );
	static void			setConstraintsDuringPlayback( boolean );


	static boolean		constructionHistoryAfterModification( void );
	static void			setConstructionHistoryAfterModification( boolean );

	static boolean		constructionHistoryDuringXform( void );
	static void			setConstructionHistoryDuringXform( boolean );

	static boolean		constructionHistoryDuringPlayback( void );
	static void			setConstructionHistoryDuringPlayback( boolean );

	static boolean		blendDuringPlayback( void );
	static void			setBlendDuringPlayback( boolean );

	static boolean		constructionHistoryRound( void );
	static void			setConstructionHistoryRound( boolean );


	static boolean		actionWindowAfterModification( void );
	static void			setActionWindowAfterModification( boolean );

	static boolean		actionWindowDuringXform( void );
	static void			setActionWindowDuringXform( boolean );
};
#endif

#endif
