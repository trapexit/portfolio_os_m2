/*
**	File:		TlModel.h
**
**	Contains:	Contains TlModel class - TlModel is a single entity
**          	containing any number of surfaces shared among
**              other models.  
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

#ifndef MODEL
#define MODEL

class TlModel : public TlCharacter
{
  public:
	TlModel( const char *name );
	TlModel( const TlModel& mdl );
	~TlModel();
	TlCharacter* Instance();

	inline TlSurface*	GetSurface()
		{ return m->surface; }
	inline void SetSurface( TlSurface *surf )
		{ m->surface = surf; }
		
	int SetRefID( int id );
	inline int GetRefID()
		{ return ( m->refID ); }
	inline int GetRefCount()
		{ return ( m->refCount ); }
		
	void AppendSurface( TlSurface *surf );
  	int GetType() { return MODEL_TYPE; }

  	ostream& WriteSDF1( ostream& os );
  	ostream& WriteSDF( ostream& os );
  
  private:
  	struct ModelEntry {
  		char *mdlName;
  		int refCount;	// Share the model data
  		int refID;      // Shared model ID
		TlSurface* surface;
		ModelEntry() { refCount = 1; }
	};
	
	ModelEntry *m;
};

#endif // MODEL

