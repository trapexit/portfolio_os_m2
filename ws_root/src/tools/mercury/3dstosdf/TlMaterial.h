#include "TlBasicTypes.h"

#ifndef MATERIAL
#define MATERIAL

class TlMaterial
{
  public:
  	TlMaterial( const char *name );
  	TlMaterial( const TlMaterial& mat );
  	~TlMaterial();
  	
  	// Data setting functions
  	void SetName( char * );
  	void SetAmbient( const TlColor& );
  	void SetDiffuse( const TlColor& );
  	void SetSpecular( const TlColor& );
  	void SetEmission( const TlColor& );
  	void SetShininess( float );
  	void SetShineStrength( float );
  	void SetTransparency( float );
  	void SetShading( int );
  	void SetTwoSided( Boolean flag );
  	void SetNextMaterial( TlMaterial *mat );
  	
  	// Data getting fuctions
  	inline const char* GetName() const
  		{ return m->matName; };
  	inline const TlColor& GetAmbient() const
  		{ return *m->ambient; }
  	inline const TlColor& GetDiffuse() const
  		{ return *m->diffuse; }
  	inline const TlColor& GetSpecular() const
  		{ return *m->specular; }
  	inline const TlColor& GetEmission() const
  		{ return *m->emission; }
  	inline float GetShininess() const
  		{ return m->shininess; }
  	inline float GetShineStrength() const
  		{ return m->shineStrength; }
  	inline float GetTransparency() const
  		{ return m->transparency; }
  	inline int GetShading() const
  		{ return m->shading; }
  	inline Boolean GetTwoSided() const
  		{ return m->twoSided; }
  	inline TlMaterial* GetNextMaterial() const
  		{ return nextMaterial; }
  	Boolean HasNullData() const;
  	
	TlMaterial& operator=( const TlMaterial& mat );
	Boolean operator==( const TlMaterial& mat ) const;
  	ostream& WriteSDF( ostream& );
	ostream& WriteSDF1( ostream& os );  
	int SetRefID( int id );
	inline int GetRefID()
		{ return ( m->refID ); }
			
  private:
  	struct MaterialEntry {
  		char *matName;
  		int   refCount;		// reference count
  		int refID;      	// Shared material ID
		TlColor *ambient;		// TlMaterial data
		TlColor *diffuse;
		TlColor *specular;
		TlColor *emission;
		float shininess;
	        float shineStrength;
		float transparency;
		int   shading;     // 1:flat, 2:smooth
		Boolean   twoSided;     // FALSE:one side, TRUE : two sided material
		MaterialEntry() { refCount = 1; }
	};
	
	MaterialEntry *m;
	TlMaterial *nextMaterial;
};

# endif // MATERIAL

