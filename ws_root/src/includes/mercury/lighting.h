#ifndef __LIGHTING_H
#define __LIGHTING_H

/* Declares structures used for the light sources */
typedef struct Color3
{
	float	r;
	float	g;
	float	b;
} Color3;

#if 0
typedef struct Color4
{
	float	r;
	float	g;
	float	b;
	float	a;
} Color4;
#else
#include <graphics/pipe/col.h>
#endif

typedef struct Material 
{
    Color4 base;
    Color3 diffuse;
    float shine;
    Color3 specular;
    uint32 flags;
    float specdata[10];
} Material;

typedef struct LightFog 
{
	float dist1, dist2;
} LightFog;

typedef struct LightDir 
{
	float nx, ny, nz;
	Color3 lightcolor;
} LightDir;

typedef struct LightPoint 
{
	float x, y, z;
	float maxdist;
	float intensity;
	Color3 lightcolor;
} LightPoint;

typedef struct LightSoftSpot {
	float x, y, z;
	float nx, ny, nz;
	float maxdist;
	float intensity;
	float cos, invcos;
	Color3 lightcolor;
} LightSoftSpot;

typedef struct LightFogTrans 
{
	float dist1, dist2;
} LightFogTrans;

typedef struct LightDirSpec
{
	float nx, ny, nz;
	Color3 lightcolor;
} LightDirSpec;

#define specdatadefinedFLAG	1


#endif /* __LIGHTING_H*/
