//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions, statements and computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties or copied or duplicated, in whole or
//	in part, without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
//
//	.NAME AlVertexDataList - Interface to Alias polyset ST's and Normals.
//
//	.SECTION Description
//		
//      AlVertexDataList is an optional additional interface to Alias's
//      per-vertex-per-poly ST and Normal data. It provides an output
//      only format for this data similar to that used by Wavefront's
//      .obj file format.
//		.br
//      The user can use this class to obtain a list of ST's and Normals
//      used by the polyset, and also may find out which normals or
//      ST's are used by each polygon in the polyset as an index into
//      this shared table.
//		.br
//      Note that this data is read-only, and is NOT automatically updated
//      if the source polyset is changed.
//

#ifndef _AlVertexDataList
#define _AlVertexDataList

#include <AlPolyset.h>

class PS_Polyset;
class AlVertexDataList
{
	friend		class AlPolyset;

public:
				AlVertexDataList( AlPolyset *pset );
				~AlVertexDataList();

	int			STListSize();
	int			normalListSize();

	boolean		STByIndex( int index, float &s, float &t );
	boolean		normalByIndex( int index, float &x, float &y, float &z );

	int			vertexNormalIndex( int polygon, int vertex  );
	int			vertexSTIndex( int polygon, int vertex );

private:
	PS_Polyset*	fPolyset;
};
#endif
