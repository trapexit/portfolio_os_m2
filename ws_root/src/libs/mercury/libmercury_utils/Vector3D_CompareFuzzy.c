#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

bool Vector3D_CompareFuzzy( Vector3D *v, Vector3D *a, float fuzzy ) {

	bool	equal;

	equal =	(
    	( (v)->x > (a)->x - fuzzy ) && ( (v)->x < (a)->x + fuzzy )
		&&
		( (v)->y > (a)->y - fuzzy ) && ( (v)->y < (a)->y + fuzzy )
		&&
		( (v)->z > (a)->z - fuzzy ) && ( (v)->z < (a)->z + fuzzy )
	);

	return equal;
}
