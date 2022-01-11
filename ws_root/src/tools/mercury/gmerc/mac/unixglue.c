#include <types.h>
#include <packages.h>
#include <TextUtils.h>
#include <math.h>

/*
	strcasecmp
*/
int strcasecmp (const char *s1, const char *s2) {
	return iuequalstring(s1,s2);
}

/* bstring(3C) -- byte string operations */


/*
	bzero
*/
void	bzero(void *d, int count) {
	
	char 	*d1;
	
	d1 = (char *)d;		/*  <Reddy 3-9-95> */
	
	while( 	count -- )
		*(d1)++ = 0;
}

/*
	bzero
*/
void	bcopy(const void *s , void *d, int count) {

	char 	*s1, *d1;

	s1 = (char *)s;
	d1 = (char *)d;
	
	while( 	count -- )
		*(d1)++ = *(s1)++;
}


#ifndef __MWERKS__

/*
	sqrtf
*/

float	sqrtf(float v) {
	double	nv;
	
	nv = v;
	nv = sqrt(nv);
	
	return (float)nv;

}

/*
	sinf
*/

float	sinf(float v) {
	double	nv;
	
	nv = v;
	nv = sin(nv);
	
	return (float)nv;

}
/*
	cosf
*/

float	cosf(float v) {
	double	nv;
	
	nv = v;
	nv = cos(nv);
	
	return (float)nv;

}

/*
	atan2f
*/
float	atan2f(float v) {
	double	nv;
	
	nv = v;
	nv = atan(nv);
	
	return (float)nv;

}
#endif
