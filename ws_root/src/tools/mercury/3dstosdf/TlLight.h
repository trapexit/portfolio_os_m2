/*
**	File:		TlLight.h
**
**	Contains:	Contains TlLight class  
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.
**
**	Change History (most recent first):
**
**
**	To Do:
*/

#include "TlCharacter.h"
#include "TlSurface.h"

#ifndef LIGHT
#define LIGHT

typedef struct TlSpotLight
{
	Point3D target;
	float  coneAngle;
	float  fallOff;
} TlSpotLight;

class TlLight : public TlCharacter
{
  public:
	TlLight( const char *name );
	TlLight( const TlLight& lgt );
	~TlLight();
	TlCharacter* Instance();
		
	int SetRefID( int id );
	inline int GetRefID()
		{ return ( m->refID ); }
	inline int GetRefCount()
		{ return ( m->refCount ); }
		
  	int GetType() { return LIGHT_TYPE; }

  	ostream& WriteSDF1( ostream& os );
  	ostream& WriteSDF( ostream& os );
  
  public:
  	struct LightEntry {
  		char *lgtName;
  		int refCount;	// Share the light data
  		int refID;      // Shared light ID
  		Point3D pos;
  		TlColor lgtColor;
  		Boolean lgtOff;
		TlSpotLight* spotLght;
		LightEntry() { refCount = 1; }
	};
	
	LightEntry *m;
};

#endif // LIGHT

