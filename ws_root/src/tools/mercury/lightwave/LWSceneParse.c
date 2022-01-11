/*
	File:		LWSceneParse.c

	Contains:	 

	Written by:	 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<6+>	12/15/95	TMA		Added -t option. Update to match 1.2 anim player.  Stripping off
									of pathname from input file (for base names).
		<4+>	11/19/95	TMA		Changed anim output to match padding requirement.
		 <4>	10/16/95	TMA		Support for lights, extensions, and preserving case.
		 <3>	10/13/95	TMA		Fixed duration problem for 1.1 (seconds versus frames). Added
									gimble-lock prevention.

	To Do:
*/

#include "ifflib.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "M2TXTypes.h"
#include "M2Err.h"
#include <string.h>
#include "qstream.h"
#include "qmem.h"
#include "os.h"
#include "vec.h"
#include "LWSURF.h"
#include "lws.i"
#include "lws.h"

LWS *cur_lws;

#define PI 3.14159265359

int HPB= 0;

FILE *OutScript;
int vertexOrder = 0;
int setScale = 0;
bool testConvex = TRUE;
bool NoFlat = FALSE;
bool TexModulate = TRUE;
bool DiffForColor = TRUE;
bool DoEnvironmentMap = TRUE;
bool EnvMult = FALSE;
bool ToLower = TRUE;
bool RemoveExtensions = TRUE;
bool DoLights = FALSE;
bool NoTextures = FALSE;
bool SepMatTex = FALSE;

bool ValidTexScript = FALSE;
bool FirstTexScript = TRUE;
char TexFileName[256];
bool ValidMatScript = FALSE;
bool FirstMatScript = TRUE;
char MatFileName[256];

int TotalPolys = 0;
int TotalFPolys = 0;
int TotalSPolys = 0;
int TotalDPolys = 0;
long TotalVertices = 0;

#define MAXNAMES  200

typedef char Name[100];

Name ObjNames[MAXNAMES];
int NumObjNames = 0;


typedef struct
{
  int    Kind;
  gfloat Color[3];
  gfloat Intensity;
  gfloat ConeAngle;
  gfloat EdgeAngle;
  gfloat Falloff;
} LW_Light;

typedef struct
{
  int  Parent;
  bool IsLight;
  int  LookAt;
  LW_Light  Light;
  int  EndBehavior;
  bool SplineData;
  char *Model;
  double Transform[4][4];
  gfloat Pivot[3];
  bool   IsIdent;
}  HNode;

typedef struct 
{
  float  U;
  float  V;
  uint16 Geometry;
} UVCoord;


UVCoord  *UVs;
uint16  UVCount;
uint16  UVSize;
uint16  UVIncrement=900;

#define C_EPS 1.0e-15

#define FP_EQUAL(s, t) (fabs(s - t) <= C_EPS)


/* definition for the components of vectors and plane equations */
#define X   0
#define Y   1
#define Z   2
#define D   3

/* type definitions for vectors and plane equations */
typedef float Vector[3];
typedef Vector Point;
typedef Vector Normal;
typedef float Plane[4];

void HNode_Init(HNode *node)
{
  if (node == NULL)
    return;

  node->EndBehavior = 1;
  node->IsLight = FALSE;
  node->SplineData = FALSE;
  node->Pivot[0] = 0.0;
  node->Pivot[1] = 0.0;
  node->Pivot[2] = 0.0;
  node->Parent = -1;
  node->Model = NULL;
  IDENTMAT4(node->Transform);
  node->IsIdent = TRUE;
}

void Light_Init(LW_Light *light)
{
  if (light == NULL)
    return;

  light->Color[0] = light->Color[1] = light->Color[2] = 1.0;
  light->Intensity = 1.0;
  light->Falloff = 0.0;
  light->ConeAngle = 45.0;
  light->EdgeAngle = 45.0;
}


#define BadFile fprintf(stderr,"ERROR:Line %d\n", cur_lws->Lines); return(M2E_BadFile)


M2Err Envelope_Parse(gfloat *value)
{

  gfloat first;
  float  tempFloat;
  long   numKeys, key, numValues, j;


  while (lws_get_token() != T_NUMBER)
    ;
  
  if ((numValues = strtol(token, NULL, 10))!= 1)
    {
      fprintf(stderr,"Expected value of 1.  Got \"%d\" at line %d\n", numValues);
    }
  else
    {
      if (lws_get_token() != T_NUMBER)
	{
	  BadFile;
	}
      numKeys = strtol(token, NULL, 10);
      for (key=0; key<numKeys; key++)
	{
	  for (j=0; j<numValues; j++)
	    {
	      lws_read_float(&tempFloat);
	      if (j== 0)
		first = tempFloat;
	    }
	  for (j=0; j<5; j++)
	    {
	      lws_read_float(&tempFloat);
	    }
	}
    }
  *value = first;
  return(M2E_NoErr);
}

typedef double Matrix4x4[4][4];

FindArbitraryAxis(double yAngle, double xAngle, double zAngle, 
		  double arbAxis[3],double *arbAngle)
{
  double rotXMat[4][4];
  double rotYMat[4][4];
  double rotZMat[4][4];
  double rotXYMat[4][4];
  double rotMat[4][4];
  double len=1.0;
  Matrix4x4  *rX, *rY, *rZ, *rXY;

  double angle; double c; double s;
  IDENTMAT4(rotXMat);
  IDENTMAT4(rotYMat);
  IDENTMAT4(rotZMat);
    
  s = sin(xAngle*PI/180.0);
  c = cos(xAngle*PI/180.0);
  rotXMat[1][1] = c;
  rotXMat[2][2] = c;
  rotXMat[1][2] = -s;
  rotXMat[2][1] = s;    /* X Rotation */

  s = sin(yAngle*PI/180.0);
  c = cos(yAngle*PI/180.0);
  rotYMat[0][0] = c;
  rotYMat[2][2] = c;
  rotYMat[0][2] = s;
  rotYMat[2][0] = -s;    /* Y Rotation */

  s = sin(zAngle*PI/180.0);
  c = cos(zAngle*PI/180.0);
  rotZMat[0][0] = c;
  rotZMat[1][1] = c;
  rotZMat[0][1] = -s;
  rotZMat[1][0] = s;    /* Z Rotation */

  /*  MXM4(rotXYMat, rotZMat, rotYMat);  */
#if 0 
  switch (HPB)
    {
    case 0:
      MXM4(rotXYMat, rotXMat, rotZMat);
      MXM4(rotMat, rotYMat, rotXYMat);
      break;
    case 1:
      MXM4(rotXYMat, rotYMat, rotZMat);
      MXM4(rotMat, rotXYMat, rotXMat);
      break;
    case 2:
      MXM4(rotXYMat, rotYMat, rotXMat);
      MXM4(rotMat, rotXYMat, rotZMat);
      break;
    case 3:
      MXM4(rotXYMat, rotZMat, rotXMat);
      MXM4(rotMat, rotYMat, rotXYMat);
      break;
    case 4:
      MXM4(rotXYMat, rotXMat, rotYMat);
      MXM4(rotMat, rotXYMat, rotZMat);
      break;
    case 5:
      MXM4(rotXYMat, rotXMat, rotYMat);
      MXM4(rotMat, rotZMat, rotXYMat);
      break;
    case 6:
      MXM4(rotXYMat, rotYMat, rotXMat);
      MXM4(rotMat, rotZMat, rotXYMat);
      break;
    case 7:
      MXM4(rotXYMat, rotXMat, rotZMat);
      MXM4(rotMat, rotXYMat, rotYMat);
      break;
    case 8:
      MXM4(rotXYMat, rotZMat, rotXMat);
      MXM4(rotMat, rotXYMat, rotYMat);
      break;
    case 9:
      MXM4(rotXYMat, rotZMat, rotYMat);
      MXM4(rotMat, rotXYMat, rotXMat);
      break;
    case 10:
      MXM4(rotXYMat, rotZMat, rotYMat);
      MXM4(rotMat, rotXMat, rotXYMat);
      break;
    case 11:
      MXM4(rotXYMat, rotYMat, rotZMat);
      MXM4(rotMat, rotXMat, rotXYMat);
      break;
    default:
      MXM4(rotXYMat, rotXMat, rotZMat);
      MXM4(rotMat, rotYMat, rotXYMat);
      break;
    }
#else
  switch (HPB)
    {
    case 0:
    case 7:
      rX = &rotXMat; 
      rY = &rotZMat;
      break;
    case 1:
    case 11:
      rX = &rotYMat; 
      rY = &rotZMat;
      break;
    case 2:
    case 6:
      rX = &rotYMat; 
      rY = &rotXMat;
      break;
    case 3:
    case 8:
      rX = &rotZMat; 
      rY = &rotXMat;
      break;
    case 4:
    case 5:
      rX = &rotXMat; 
      rY = &rotYMat;
      break;
    case 9:
    case 10:
      rX = &rotZMat; 
      rY = &rotYMat;
      break;
    default:
      rX = &rotXMat; 
      rY = &rotZMat;
      break;
    }
  MXM4(rotXYMat, *rX, *rY);

  switch (HPB)
    {
    case 0:
    case 3:
      rXY = &rotYMat; 
      rZ = &rotXYMat;
      break;
    case 1:
    case 9:
      rXY = &rotXYMat; 
      rZ = &rotXMat;
      break;
    case 2:
    case 4:
      rXY = &rotXYMat; 
      rZ = &rotZMat;
      break;
    case 5:
    case 6:
      rXY = &rotZMat; 
      rZ = &rotXYMat;
      break;
    case 7:
    case 8:
      rXY = &rotXYMat; 
      rZ = &rotYMat;
      break;
    case 10:
    case 11:
      rXY = &rotXMat; 
      rZ = &rotXYMat;
      break;
    default:
      rXY = &rotYMat; 
      rZ = &rotXYMat;
      break;
    }
  MXM4(rotMat, *rXY, *rZ);
#endif

  c = (rotMat[0][0] + rotMat[1][1] + rotMat[2][2] -1)*0.5;
  
  angle = acos(c);

  arbAxis[0] = (rotMat[1][2] - rotMat[2][1]);
  arbAxis[1] = (rotMat[2][0] - rotMat[0][2]);
  arbAxis[2] = (rotMat[0][1] - rotMat[1][0]);

  if (angle != 0.0)
    {
      len = NORMSQRD3(arbAxis);
      len = sqrt(len);
      if (len!=0.0)
	{
	  if (setScale == 0)
	    arbAxis[0] = arbAxis[0]/len;
	  else
	    arbAxis[0] = -arbAxis[0]/len;
	  arbAxis[1] = arbAxis[1]/len;
	  arbAxis[2] = -arbAxis[2]/len;
	}
      else 
	{
	  fprintf(stderr,"WARNING:Arbitrary axis length = 0.0\n");
	  arbAxis[0] = arbAxis[1] = arbAxis[2] = 0.0;
	}
    }
  *arbAngle = angle;
}


typedef struct
{
  gfloat Motion[9];
  gfloat After[5];
} LW_Key;

static void Matrix_Compute(LW_Key key, HNode *node)
{
  double rotXMat[4][4];
  double rotYMat[4][4];
  double rotZMat[4][4];
  double matA[4][4];
  double matB[4][4];
  double rotMat[4][4];
  double trnslMat[4][4];
  double scaleMat[4][4];
  double pivotMat[4][4];
  double len=1.0;
  double c, s, t;
  double x, y, z;
  double arbAxis[3], arbAngle;

  IDENTMAT4(rotXMat);
  IDENTMAT4(rotMat);
  IDENTMAT4(rotYMat);
  IDENTMAT4(rotZMat);  
  IDENTMAT4(trnslMat);
  IDENTMAT4(scaleMat);
  IDENTMAT4(pivotMat);
    
  trnslMat[3][0] = key.Motion[0];
  trnslMat[3][1] = key.Motion[1];
  if (setScale != 0)
    trnslMat[3][2] = key.Motion[2];
  else
  trnslMat[3][2] = -key.Motion[2];

  pivotMat[3][0] = -node->Pivot[0];
  pivotMat[3][1] = -node->Pivot[1];
  if (setScale != 0)
    pivotMat[3][2] = -node->Pivot[2];
  else
    pivotMat[3][2] = node->Pivot[2];

  scaleMat[0][0] = key.Motion[6];
  scaleMat[1][1] = key.Motion[7];
  scaleMat[2][2] = key.Motion[8];

  
  FindArbitraryAxis(key.Motion[3], key.Motion[4], key.Motion[5], 
		    arbAxis, &arbAngle);
  
  x = arbAxis[0]; y = arbAxis[1]; z = arbAxis[2];
  c = cos(arbAngle);
  s = sin(arbAngle);
  t = 1.0 - c;
  
  rotMat[0][0]=t*x*x+c; rotMat[0][1]=t*x*y+(s*z); rotMat[0][2] = t*x*z-(s*y); 
  rotMat[1][0]=t*x*y-(s*z); rotMat[1][1]=t*y*y+c; rotMat[1][2] = t*y*z+(s*x); 
  rotMat[2][0]=t*x*z+(s*y); rotMat[2][1]=t*y*z-(s*x); rotMat[2][2] = t*z*z+c; 

  /* Order of transforms is scale, pivot, rotate, translate */

/*  close, spots off 
  MXM4(matA, rotMat, trnslMat);
  MXM4(matB, pivotMat, matA);
  MXM4(matA, scaleMat, matB);
*/

/* totally off
  MXM4(matA, pivotMat, scaleMat);
  MXM4(matB, rotMat, matA);
  MXM4(matA, trnslMat, matB);
*/

/*  About the same maybe better as first
  MXM4(matA, rotMat, trnslMat);
  MXM4(matB, scaleMat, matA);
  MXM4(matA, pivotMat, matB);
*/

  MXM4(matA, rotMat, trnslMat);
  MXM4(matB, scaleMat, matA);
  MXM4(matA, pivotMat, matB);


  SETMAT4(node->Transform, matA);

}

Name_Process(char *inName, char **procName)
{
  char *myName;
  char *lastSlash;
  char *lastColon;
  char *lastBack;
  char *temp;
  int i, slashLen, backLen, colonLen, tempLen;
  
  /* Find the file name minus the path name */
  
  lastBack = (char *)strrchr(inName, '\\');
  lastColon = (char *)strrchr(inName, ':');
  lastSlash = (char *)strrchr(inName, '/');
  tempLen = strlen(inName);

  if (lastColon != NULL)
    {
      lastColon++;
      colonLen = strlen(lastColon);
    }
  else
    {
      lastColon = inName;
      colonLen = tempLen;
    }
  if (lastSlash != NULL)
  {
    lastSlash++;
    slashLen = strlen(lastSlash);
  }  
  else
    {
      lastSlash = inName;
      slashLen = tempLen;
    }
  if (lastBack != NULL)
    { 
      lastBack++;
      backLen = strlen(lastBack);
    }
  else
    {
      lastBack = inName;
      backLen = tempLen;
    }

  if (colonLen < slashLen)
    {
      tempLen = colonLen;
      temp = lastColon;
    }
  else
    {
      tempLen = slashLen;
      temp = lastSlash;
    }
  
  if (backLen < tempLen)
    {
      tempLen = backLen;
      temp = lastBack;
    }

  /* Copy just the file Name */
  myName = (char *)calloc(tempLen+4, 1);
  strcpy(myName, temp);
  
  if (ToLower)
    {
      for (i=0; i<strlen(myName); i++)
	myName[i] = tolower(myName[i]);
    }
  
  *procName = myName;

}

static M2Err TexName_Process(char *inName, char **texName)
{

  char *lastExtension;
  char *myName;
  int i;

  Name_Process(inName, &myName);
  lastExtension = (char *)strrchr(myName, '.');
  
  /* Replace any extension */
  if ((lastExtension != NULL) && (RemoveExtensions))
    {
      for( i=1; lastExtension[i] != '\0'; i++)
	{
	  if (!((lastExtension[i] <= '9') && (lastExtension[i] >= '0')))
	    {  /* If it's not an image number like .120, replace it */
	      *lastExtension = '\0';
	      break;
	    }
	}
    }
  strcat(myName, ".utf");
  *texName = myName;
  return(M2E_NoErr);
}

static M2Err ObjName_Process(char *inName, char **objName)
{
  char *lastExtension;
  char *myName;
  int i;

  Name_Process(inName, &myName);

  lastExtension = (char *)strrchr(myName, '.');

  /* Replace any extension */
  if ((lastExtension != NULL) && (RemoveExtensions))
    {
      for( i=1; lastExtension[i] != '\0'; i++)
	{
	  if (!((lastExtension[i] <= '9') && (lastExtension[i] >= '0')))
	    {  /* If it's not an image number like .120, replace it */
	      *lastExtension = '\0';
	      break;
	    }
	}
    }

  strcat(myName, ".sdf");

  *objName = myName;
  return(M2E_NoErr);
}


bool Script_AddName(FILE *fPtr, char *newName)
{
  int i;
  char *procName;
  
  for (i=0; i<NumObjNames; i++)
    {
      if (!strcmp(ObjNames[i], newName))
	return(FALSE);
    }

  if (NumObjNames< MAXNAMES)
    {
      strcpy(ObjNames[NumObjNames], newName);
      NumObjNames++;
   }
  else
    fprintf(stderr,"WARNING:Name buffer overflow, can't store name!\n");
  
  Name_Process(newName, &procName);
  /* Replace any extension */
  fprintf(fPtr, "lwtosdf \"%s\" ",procName);
  /* Put options here */
  free(procName);
  ObjName_Process(newName, &procName);
  if (!ToLower)
    fprintf(fPtr, "-case ");
  if (NoFlat)
    fprintf(fPtr, "-smooth ");
  if (!DoEnvironmentMap)
    fprintf(fPtr, "-envmap ");
  if (EnvMult)
    fprintf(fPtr, "-envmult ");
  if (!RemoveExtensions)
    fprintf(fPtr, "-e ");
  if (SepMatTex)
    fprintf(fPtr, "-m ");
  if (ValidTexScript)
    {
      if (FirstTexScript)
	{
	  fprintf(fPtr, "-tn \"%s\" ",TexFileName);
	  FirstTexScript = FALSE;
	}
      else
	  fprintf(fPtr, "-ta \"%s\" ",TexFileName);	
    }
  if (ValidMatScript)
    {
      if (FirstMatScript)
	{
	  fprintf(fPtr, "-mn \"%s\" ",MatFileName);
	  FirstMatScript = FALSE;
	}
      else
	  fprintf(fPtr, "-ma \"%s\" ",MatFileName);	
    }
  if (NoTextures)
    fprintf(fPtr, "-t ",procName);    
  fprintf(fPtr, "\"%s\"\n",procName);
  free(procName);
  return(TRUE);
}


/***
 *
 * Token parsing data
 *
 ***/
		int		tokenType = 0;		/* type of input token */
static	bool	gotToken = FALSE;	/* TRUE if token has been read ahead */
char	token[256];			/* input token buffer */

static char *token_types[] =	/* token names for error messages */
{
	"end",
	"keyword",
	"number",
	"{",
	"}",
	"(",
	")",
	"|",
	",",
	"[",
	"]",
	"bitfield",
};
static int	max_token_types = sizeof(token_types) / sizeof(char *);

/****
 *
 * Read a floating point number from the current LWS file and save
 * its value in the given location. Print an error message if
 * a number cannot be parsed.
 *
 * returns:
 *	TRUE if successfully read, otherwise FALSE
 *
 ****/
bool
lws_read_float(gfloat* f)
{
	assert(f);
	if (lws_get_token() != T_NUMBER)
	   {
		lws_bad_token(T_NUMBER);
		return FALSE;
	   }
	else
	   {
		sscanf(token, "%f", f);
		return TRUE;
	   }
}

/****
 *
 * Read an integer from the current LWS file and save
 * its value in the given location. Print an error message if
 * a number cannot be parsed.
 *
 * returns:
 *	TRUE if successfully read, otherwise FALSE
 *
 ****/
bool
lws_read_int(int32* i)
{
	if (lws_get_token() != T_NUMBER)
	   {
		lws_bad_token(T_NUMBER);
		return FALSE;
	   }
	else
	   {
		sscanf(token, "%d", i);
		return TRUE;
	   }
}

/****
 *
 * Read a point from the current LWS file and copy
 * its value in the given point. Print an error message if
 * a point cannot be parsed.
 *
 * returns:
 *	TRUE if successfully read, otherwise FALSE
 *
 ****/
bool
lws_read_point(Point3* p)
{
	return (lws_check_token(T_LBRACE) &&
			lws_read_float(&p->x) &&
			lws_read_float(&p->y) &&
			lws_read_float(&p->z) &&
			lws_check_token(T_RBRACE));
}

/****
 *
 * Read a name from the input stream. A name can be either a
 * string of alphanumeric characters or a quoted string.
 * Names are converted to lower case. An error message is
 * printed if a name does not follow in the input stream.
 *
 * returns:
 *	-> first character in name (null-terminated string)
 *	NULL if no name can be read
 *
 ****/
char* lws_read_name(char* s)
{
	if (!s)									/* token already parsed? */
	   {
		if (!lws_check_token(T_KEYWORD))	/* parse the name/string */
			return NULL;
		s = token;
	   }
	while (*s)								/* convert to lower case */
		*s++ = tolower(*s);
	return token;
}

/****
 *
 * Read a string from the input stream. A name can be either a
 * string of alphanumeric characters or a quoted string. We do
 * not do any translation of the characters. An error message is
 * printed if a name does not follow in the input stream.
 * String are not converted to lower case.
 *
 * returns:
 *	-> first character in name (null-terminated string)
 *	NULL if no name can be read
 *
 ****/
char*
lws_read_string()
{
	if (lws_get_token() != T_KEYWORD)
	   {
		lws_bad_token(T_KEYWORD);
		return NULL;
	   }
	return token;
}

/****
 *
 * Read a character from the input stream and update the LWS
 * line number if a newline is encountered. Because we run on
 * Mac, PC and Unix, an end of line character can have different
 * meanings:
 *	Unix	newline			\n
 *	Mac		carriage return	\r
 *	PC		carriage return and newline \r\n
 *
 * returns:
 *   character read
 *
 ****/
static int
GetChar(void)
{
	int32 c, d;

	c = K9_GetChar(cur_lws->Stream);
	if (c == '\n')					/* Unix end of line? */
	   {
		cur_lws->Lines++;			/* bump line number and exit */
		return c;
	   }
	if (c != '\r')					/* not Mac end of line? */
		return c;					/* just return the char */
	c = '\n';						/* always convert to newline */
	cur_lws->Lines++;				/* bump line number */
	d = K9_GetChar(cur_lws->Stream);
	if (d != '\n')					/* PC end of line? */
		K9_UngetChar(cur_lws->Stream, d);
	return c;
}

/****
 *
 * Put back the given input character. If it is a newline,
 * update the LWS line number
 *
 ****/
static void
UngetChar(int c)
{
	if (c == '\n')
		cur_lws->Lines--;
	K9_UngetChar(cur_lws->Stream, c);
}


/****
 *
 * Get the next input token and return its type.
 *
 * returns:
 *	token	  -> first non-space character of input token
 *	tokenType = type of token
 *		T_END		end of file
 *		T_KEYWORD	name or quoted string
 *		T_NUMBER	integer or floating point number
 *		T_OR		|
 *		T_COMMA		,
 *		T_LBRACK	[
 *		T_RBRACK	]
 *		T_LPAREN	(
 *		T_RPAREN	)
 *
 ****/
int32
lws_get_token(void)
{
	int i;
	int c;

	if (gotToken)					/* did we already get one? */
	   {
		gotToken = FALSE;			/* indicate it has been used */
		return tokenType;			/* just return it */
	   }
	for (;;)						/* find next token */
	   {
		while ((c = GetChar()) != EOF && isspace(c))
			{ ; }					/* consume space characters */

		if (c == EOF)				/* stop if end of file */
			return (tokenType = T_END);

/*		if (c == '#' || c == '/') */	/* consume comments */
		if (c == '#')	                /* consume comments */
		   {
			while ((c = GetChar()) != EOF && (c != '\n'))
				{ ; }
			continue;
		   }
		if (c == '"')				/* consume quoted strings */
		   {
			i = 0;
			while ((c = GetChar()) != EOF && c != '"')
			   {
				token[i] = c;
				i++;
			   }
			token[i] = '\0';
			return (tokenType = T_KEYWORD);
		   }
		if (c == '|')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_OR);
		   }
		if (c == ',')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_COMMA);
		   }
		if (c == '[')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_LBRACK);
		   }
		if (c == ']')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_RBRACK);
		   }
		if (c == '(')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_LPAREN);
		   }
		if (c == ')')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_RPAREN);
		   }
		if (c == '{')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_LBRACE);
		   }
		if (c == '}')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_RBRACE);
		   }
		/* extract keyword token */
		/*		if (c != EOF && (isalpha(c) || c == '_')) */
		if (c != EOF && (isalpha(c) || c == '_' || c == '/' || c == '\\' || c == ':'))
		  {
		    /*			for (i = 0; c != EOF && (isalnum(c) || c == '_' || c == '.'); i++) */
		    for (i = 0; c != EOF && (isalnum(c) || c == '_' || c == '/' || c == '\\' || c == ':' || c == '.' || c =='-'); i++)
			   {
				token[i] = c;
				c = GetChar();
			   }
			token[i] = '\0';
			UngetChar(c);
			return (tokenType = T_KEYWORD);
		   }
		else if (c != EOF && isdigit(c) ||
				 c == '.' ||
				 c == '+' ||
				 c == '-' ||
				 c == 'e' ||
				 c == 'E')
		   {
			for (i = 0;
				 c != EOF &&
				 	(isdigit(c) ||
					 c == '.' ||
					 c == '+' ||
					 c == '-' ||
					 c == 'e' ||
					 c == 'E');
				 i++)
			   {
				token[i] = c;
				c = GetChar();
			   }
			token[i] = '\0';
			UngetChar(c);
			return (tokenType = T_NUMBER);
		   }
		else
		   {
/*			GLIB_WARNING(("<< %c >>\n", c)); */
		     fprintf(stderr,"WARNING:<< %c >> \n",c);
			lws_bad_token(T_UNKNOWN);
			tokenType = T_END;
		   }
	   }

	/* can't get here */
	return (tokenType = T_END);
}



/****
 *
 * Print a diagnostic message indicating a bad token was parsed
 *
 ****/
void
lws_bad_token(int32 t)
{
  char *t_name;
  
  if ((t > 0) && (t < max_token_types))
    {
      t_name = token_types[t];
      fprintf(stderr, "WARNING:unexpected token '%s', was expecting %s\n", token, t_name);
    }
  else
    fprintf(stderr,"WARNING:unexpected token '%s'", token);
}


/****
 *
 * Put the current token back
 *
 ****/
void
lws_unget_token(void)
{
	assert(!gotToken);
	gotToken = TRUE;
}

/****
 *
 * Get the next token and check to see if it is of the given type.
 * Print an error message if not.
 *
 * returns:
 *   TRUE if next token is of the given type, else FALSE
 *
 ****/
bool
lws_check_token(int32 t)
{
  if (lws_get_token() == t)
    return TRUE;
  lws_bad_token(t);
  return FALSE;
}

#define NUM_NODES 1000

Tab_Print(FILE *fPtr, int tabLevel)
{
  int i;

  for(i=0; i<tabLevel; i++)
    fprintf(fPtr,"\t");
}


bool LightOn(LW_Light light)
{
  float inten, value;

  inten = light.Intensity/255.0;
  value = light.Color[0]*inten;
  if (value > 0.001)
    return(TRUE);
  value = light.Color[1]*inten;
  if (value > 0.001)
    return(TRUE);
  value = light.Color[2]*inten;
  if (value > 0.001)
    return(TRUE);
  return(FALSE);
}

HNode_ChildPrint(FILE *fPtr, HNode *nodes, int numNodes, int parent, int tab)
{
  int i;
  double inten;

  for (i=0; i<numNodes; i++)
    {
      if (nodes[i].Parent == parent)
	{
	  Tab_Print(fPtr,tab);
	  fprintf(fPtr,"Define Group Grp.%d {\n",i);
	  Tab_Print(fPtr,tab+1);
	  fprintf(fPtr, "Transform {\n");
	  Tab_Print(fPtr,tab+2);
	  fprintf(fPtr, "%g %g %g %g\n", nodes[i].Transform[0][0], 
		  nodes[i].Transform[0][1], nodes[i].Transform[0][2],
		  nodes[i].Transform[0][3]);
	  Tab_Print(fPtr,tab+2);
	 	  fprintf(fPtr, "%g %g %g %g\n", nodes[i].Transform[1][0], 
		  nodes[i].Transform[1][1], nodes[i].Transform[1][2],
		  nodes[i].Transform[1][3]);
	  Tab_Print(fPtr,tab+2);
	  fprintf(fPtr, "%g %g %g %g\n", nodes[i].Transform[2][0], 
		  nodes[i].Transform[2][1], nodes[i].Transform[2][2],
		  nodes[i].Transform[2][3]);
	  Tab_Print(fPtr,tab+2);
	  fprintf(fPtr, "%g %g %g %g\n", nodes[i].Transform[3][0], 
		  nodes[i].Transform[3][1], nodes[i].Transform[3][2],
		  nodes[i].Transform[3][3]);
	  Tab_Print(fPtr,tab+1);
	  fprintf(fPtr, "}\n");

	  HNode_ChildPrint(fPtr, nodes, numNodes, i, tab+1);
	  if (nodes[i].Model != NULL)
	    {
	      Tab_Print(fPtr,tab+1);
	      if (setScale != 0)
		{
		  fprintf(fPtr,"Define Group Grp.%d.1 {\n",i);
		  Tab_Print(fPtr,tab+2);
		  fprintf(fPtr,"Scale { 1 1 -1} \n");
		  Tab_Print(fPtr,tab+2);
		  fprintf(fPtr,"Use Model \"%s\"\n",nodes[i].Model);
		  Tab_Print(fPtr,tab+1);
		  fprintf(fPtr,"}\n");
		}
	      else
		{
		  fprintf(fPtr,"Use Model \"%s\"\n",nodes[i].Model);
		}
	    }
	  if (DoLights && nodes[i].IsLight)
	    {
	      if (LightOn(nodes[i].Light))
		{
		  Tab_Print(fPtr,tab+1);
		  fprintf(fPtr,"Light {\n");
		  Tab_Print(fPtr,tab+2);
		  fprintf(fPtr,"Kind ");
		  switch(nodes[i].Light.Kind)
		    {
		    case 0:
		      fprintf(fPtr,"directional\n");
		      break;
		    case 1:
		      fprintf(fPtr,"point\n");
		      break;
		    case 2:
		      if (FP_EQUAL(nodes[i].Light.EdgeAngle, 0)) 
			{
			  fprintf(fPtr,"spot\n");
			  Tab_Print(fPtr,tab+2);
			  fprintf(fPtr,"angle %g\n",  nodes[i].Light.ConeAngle); 
			}
		      else
			{
			  fprintf(fPtr,"softspot\n");
			  Tab_Print(fPtr,tab+2);
			  fprintf(fPtr,"angle %g\n", nodes[i].Light.ConeAngle+nodes[i].Light.EdgeAngle); 
			  Tab_Print(fPtr,tab+2);
			  fprintf(fPtr,"falloff %g\n", 1-(nodes[i].Light.ConeAngle/(nodes[i].Light.ConeAngle+nodes[i].Light.EdgeAngle)));
			}
		      break;
		    }
		  Tab_Print(fPtr,tab+2);
		  inten = nodes[i].Light.Intensity/255.0;
		  fprintf(fPtr,"color {%g %g %g}\n", nodes[i].Light.Color[0]*inten,
			  nodes[i].Light.Color[1]*inten, 
			  nodes[i].Light.Color[2]*inten);
		  Tab_Print(fPtr,tab+1);
		  fprintf(fPtr,"}\n");
		}
	    }
	  Tab_Print(fPtr,tab);
	  fprintf(fPtr,"}\n");
	}
    }
}

static double FirstFrame=1;
static double LastFrame=30;
static double FramesPerSecond = 30;

Key_Print(FILE *fPtr, LW_Key *keys, int numKeys, int curObject, int tab,
	  HNode curNode, HNode *hier)
{
  int key;
  double arbAxis[3];
  double arbAngle;
  double dot1, dot2, halfAng, s;
  double x, y, z;
  double pmq[4], pq[4], cq[4];
  HNode pNode;

  Tab_Print(fPtr,tab);
  fprintf(fPtr,"KF_Object {\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"Target use Grp.%d\n", curObject-1);
  Tab_Print(fPtr,tab+1);
  if (setScale != 0)
    fprintf(fPtr,"ObjPivot { %g %g %g }\n", curNode.Pivot[0], curNode.Pivot[1], curNode.Pivot[2]);
  else
    fprintf(fPtr,"ObjPivot { %g %g %g }\n", curNode.Pivot[0], curNode.Pivot[1], -curNode.Pivot[2]);

  Tab_Print(fPtr,tab+1);
  if (curNode.Parent != -1)
    {
      pNode = hier[curNode.Parent];
     if (setScale != 0)
       fprintf(fPtr,"PrntPivot { %g %g %g }\n",  pNode.Pivot[0], pNode.Pivot[1], pNode.Pivot[2]);
     else
       fprintf(fPtr,"PrntPivot { %g %g %g }\n", pNode.Pivot[0], pNode.Pivot[1], -pNode.Pivot[2]);
      Tab_Print(fPtr,tab+1);
    }

  fprintf(fPtr,"PosFrames { \n");
  Tab_Print(fPtr,tab+2);
  for (key=0; key<numKeys; key++)
    fprintf(fPtr,"%g ", (double)keys[key].After[0]);
  fprintf(fPtr,"\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"}\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"PosData { \n");
  for (key=0; key<numKeys; key++)
    {
      Tab_Print(fPtr,tab+2);
      x = keys[key].Motion[0];
      y = keys[key].Motion[1];
      z = keys[key].Motion[2];
      
      if (curNode.Parent != -1)
	{
	  x -= pNode.Pivot[0];
	  y -= pNode.Pivot[1];
	  z -= pNode.Pivot[2];
	}
      if (setScale != 0)
	{
	  fprintf(fPtr,"%g %g %g\n", x, y, z);
	}
      else
	{
	  fprintf(fPtr,"%g %g %g\n", x, y, -z);	  
	}
    }
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"}\n");

  if (curNode.SplineData)
    {
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"PosSplData { \n");
      for (key=0; key<numKeys; key++)
	{
	  Tab_Print(fPtr,tab+2);
	  fprintf(fPtr,"%g %g %g 0 0\n", (double)keys[key].After[2], 
		  (double)keys[key].After[4], (double)keys[key].After[3]); 
	}
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"}\n");
    }
  
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"RotFrames { \n");
  Tab_Print(fPtr,tab+2);
  for (key=0; key<numKeys; key++)
    fprintf(fPtr,"%g ", (double)keys[key].After[0]);
  fprintf(fPtr,"\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"}\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"RotData { \n");
  for (key=0; key<numKeys; key++)
    {
      FindArbitraryAxis(keys[key].Motion[3], keys[key].Motion[4], keys[key].Motion[5], 
			arbAxis, &arbAngle);
      
      s = sin(arbAngle/2.0);
      cq[0] = arbAxis[0]*s;
      cq[1] = arbAxis[1]*s;
      cq[2] = arbAxis[2]*s;
      cq[3] = cos(arbAngle/2.0);

      if (key > 0)
	{
	  pmq[0] = pq[0] - cq[0]; pmq[1] = pq[1] - cq[1];
	  pmq[2] = pq[2] - cq[2]; pmq[3] = pq[3] - cq[3];
	  dot1 = pmq[0]*pmq[0] + pmq[1]*pmq[1] + pmq[2]*pmq[2]
	    + pmq[3]*pmq[3];

	  pmq[0] = pq[0] + cq[0]; pmq[1] = pq[1] + cq[1];
	  pmq[2] = pq[2] + cq[2]; pmq[3] = pq[3] + cq[3];
	  dot2 = pmq[0]*pmq[0] + pmq[1]*pmq[1] + pmq[2]*pmq[2]
	    + pmq[3]*pmq[3];

	  if (dot1 > dot2)
	    {
	      cq[0] = -cq[0];
	      cq[1] = -cq[1];
	      cq[2] = -cq[2];
	      cq[3] = -cq[3];
	    }
	  halfAng = acos(cq[3] );
	  arbAngle = 2.0 * halfAng;
	  /* normalize the vector */
	  s = sqrt( cq[0]*cq[0] + cq[1]*cq[1] + cq[2]*cq[2]);
	  if (s != 0.0)
	    {
	      arbAxis[0] = cq[0]/s;
	      arbAxis[1] = cq[1]/s;
	      arbAxis[2] = cq[2]/s;
	    }
	}
      pq[0] = cq[0];
      pq[1] = cq[1];
      pq[2] = cq[2];
      pq[3] = cq[3];

      Tab_Print(fPtr,tab+2);
      fprintf(fPtr,"%g %g %g %g\n", arbAxis[0], arbAxis[1], arbAxis[2],
	      arbAngle);
    }
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"}\n");

  if (curNode.SplineData)
    {
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"RotSplData { \n");
      for (key=0; key<numKeys; key++)
	{
	  Tab_Print(fPtr,tab+2);
	  fprintf(fPtr,"%g %g %g 0 0\n", (double)keys[key].After[2], 
		  (double)keys[key].After[4], (double)keys[key].After[3]); 
	}
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"}\n");
    }

  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"SclFrames { \n");
  Tab_Print(fPtr,tab+2);
  for (key=0; key<numKeys; key++)
    fprintf(fPtr,"%g ", (double)keys[key].After[0]);
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"}\n");
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"SclData { \n");
  for (key=0; key<numKeys; key++)
    {
      Tab_Print(fPtr,tab+2);
      fprintf(fPtr,"%g %g %g\n", keys[key].Motion[6], keys[key].Motion[7], 
	      keys[key].Motion[8]);
    }
  Tab_Print(fPtr,tab+1);
  fprintf(fPtr,"}\n");

  if (curNode.SplineData)
    {
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"SclSplData { \n");
      for (key=0; key<numKeys; key++)
	{
	  Tab_Print(fPtr,tab+2);
	  fprintf(fPtr,"%g %g %g 0 0\n", (double)keys[key].After[2], 
		  (double)keys[key].After[4], (double)keys[key].After[3]); 
	}
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"}\n");
    }

  if (numKeys > 1)
    {
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"duration %f\n", (keys[numKeys-1].After[0]-keys[0].After[0])/FramesPerSecond);
/*
   fprintf(fPtr,"StartTime %d\n", FirstFrame);
   Tab_Print(fPtr,tab+1);
   fprintf(fPtr,"duration %d\n", LastFrame - FirstFrame);
*/
    }
/*  if (curNode.EndBehavior == 2) */
  if (1)
    {
      Tab_Print(fPtr,tab+1);
      fprintf(fPtr,"control cycle\n");
    }
  Tab_Print(fPtr,tab);
  fprintf(fPtr, "}\n\n");
}

#define NUM_KEYS 300

M2Err LWS_Process(char *fileIn, char *fileOut, FILE *OutScript)
{
  LWS lws;
  ByteStream*		stream = NULL;
  FILE                  *outPtr;
  int  tokenType, numValues, numKeys, key, i, j;
  M2Err err;
  char *objName, *scriptName, *lastExtension;
  LW_Key *keys;
  gfloat tempFloat;
  HNode *hier;
  HNode *curNode;
  bool pastObjects;
  
  uint32 nodeCount, curObject;

  Name_Process(fileOut, &scriptName);
  lastExtension = (char *)strrchr(scriptName, '.');
	    
  /* Replace any extension */
  if ((lastExtension != NULL) && (RemoveExtensions))
    {
      for( i=1; lastExtension[i] != '\0'; i++)
	{
	  if (!((lastExtension[i] <= '9') && (lastExtension[i] >= '0')))
	    {  /* If it's not an image number like .120, replace it */
	      *lastExtension = '\0';
	      break;
	    }
	}
    }

  hier = (HNode *)malloc(NUM_NODES*sizeof(HNode));
  if (hier == NULL)
  {
  	fprintf(stderr,"Out of memory during node allocation in LWS_Process\n");
  	return(M2E_NoMem);  
  }
  keys = (LW_Key *)malloc(NUM_KEYS*sizeof(LW_Key));
  if (keys == NULL)
  {
  	fprintf(stderr,"Out of memory during key allocation in LWS_Process\n");
  	return(M2E_NoMem);  
  }

  for (i=0; i<NUM_NODES; i++)
    {
      HNode_Init(&(hier[i]));
    }

  strcpy(lws.FileName, fileIn);
  stream = K9_OpenByteStream(lws.FileName, Stream_Read, 0);
  outPtr = fopen(fileOut, "w");
  if (outPtr == NULL)
  	{
  		fprintf(stderr,"Bad File \"%s\"\n", fileOut);
  		return(M2E_BadFile);
  	}

  if (stream == NULL)
    {
      fprintf(stderr,"Could not open file \"%s\"", lws.FileName);
      return (M2E_BadFile);
    }
  lws.Stream = stream;
  lws.Parent = cur_lws;
  cur_lws = &lws;

  if (lws_get_token()==T_KEYWORD)
    {
      if (!strcmp(token, "LWSC"))
	{
	  if (lws_get_token() != T_NUMBER)
	   {
	     BadFile;
	   }
	  if (strcmp(token,"1"))
	   {
	     BadFile;
	   }
	}
    }
  else
    { 
      BadFile;
    }

  fprintf(outPtr,"SDFVersion 1.0\n\n");

  nodeCount = 0;
  curNode = &(hier[0]);
  pastObjects = FALSE;
  while ((tokenType = lws_get_token()) != T_END)
    {
      switch (tokenType)
	{
	case T_KEYWORD:
	  if (!strcmp(token, "LoadObject"))     
	    {
	      if (!pastObjects)
		{
		  if (lws_get_token() != T_KEYWORD)
		    {
		      fprintf(stderr,"Object expected. Got \"%s\" at line %d\n", token,
			      cur_lws->Lines);
		    }
		  Script_AddName(OutScript,token);
		  Name_Process(token, &(hier[nodeCount].Model));
		  curNode = &(hier[nodeCount]);
		  nodeCount++;
		}
	    }
	  else if(!strcmp(token, "PivotPoint"))
	    {
	      if (!pastObjects)
		{
		  if (!lws_read_float(&(curNode->Pivot[0])))
		      fprintf(stderr,"Warning! Pivot Point read expected float\n");
		  if (!lws_read_float(&(curNode->Pivot[1])))
		      fprintf(stderr,"Warning! Pivot Point read expected float\n");
		  if (!lws_read_float(&(curNode->Pivot[2])))
		      fprintf(stderr,"Warning! Pivot Point read expected float\n");
		}
	    }
	  else if(!strcmp(token, "AddNullObject"))
	    {
	      if (!pastObjects)
		{
		  hier[nodeCount].Model = NULL;
		  curNode = &(hier[nodeCount]);
		  nodeCount++;
		}
	    }
	  else if(!strcmp(token, "ShowCamera"))
	    {
	      pastObjects = TRUE;
	    }
	  else if(!strcmp(token, "AddLight"))
	    {
	      if (DoLights)
	    {
	      if (!pastObjects)
		{
		      hier[nodeCount].Model = NULL;
		      curNode = &(hier[nodeCount]);
		      hier[nodeCount].IsLight = TRUE;
		      Light_Init(&hier[nodeCount].Light);
		      nodeCount++;
		    }
		}
	      else
		{
		  pastObjects = TRUE;
		}
	    }
	  else if (!strcmp(token, "ParentObject"))
	    {
	      if (!pastObjects)
		{
		  lws_get_token();
		  curNode->Parent =  strtol(token, NULL, 10)-1;
		}
	    }
	  else if (DoLights)
	  {
	    if (!strcmp(token, "LightType"))
	      {
		if (!pastObjects)
		  {
		    lws_get_token();
		    curNode->Light.Kind = strtol(token, NULL, 10);
		  }
	      }
	    if (!strcmp(token, "LgtIntensity"))
	      {
		if (!pastObjects)
		  {
		    lws_get_token();
		    if (!strcmp(token, "(envelope)"))
		     {
		       err = Envelope_Parse(&(curNode->Light.Intensity));
		       if (err != M2E_NoErr)
			 return(err);
		     }
		    else
		      curNode->Light.Intensity = atof(token);
		  }
	      }
	    else if (!strcmp(token, "Falloff"))
	      {
		if (!pastObjects)
		  {
		    lws_get_token();
		    if (!strcmp(token, "(envelope)"))
		      {
			err = Envelope_Parse(&(curNode->Light.Falloff));
			if (err != M2E_NoErr)
			  return(err);
		      }
		    else
		      curNode->Light.Falloff = atof(token);
		  }
	      }
	    else if (!strcmp(token, "ConeAngle"))
	      {
		if (!pastObjects)
		  {
		    lws_get_token();
		    if (!strcmp(token, "(envelope)"))
		    {
		      err = Envelope_Parse(&(curNode->Light.ConeAngle));
		      if (err != M2E_NoErr)
			return(err);
		    }
		    else
		      curNode->Light.ConeAngle = atof(token);
		  }
	      }
	    else if (!strcmp(token, "EdgeAngle"))
	      {
		if (!pastObjects)
		  {
		    lws_get_token();
		    if (!strcmp(token, "(envelope)"))
		      {
			err = Envelope_Parse(&(curNode->Light.EdgeAngle));
			if (err != M2E_NoErr)
			  return(err);
		      }
		    else
		      curNode->Light.EdgeAngle = atof(token);
		  }
	      }
	    else if (!strcmp(token, "LightColor"))
	      {
		if (!pastObjects)
		  {
		    lws_read_float(&tempFloat);
		    curNode->Light.Color[0] = tempFloat;
		    lws_read_float(&tempFloat);
		    curNode->Light.Color[1] = tempFloat;
		    lws_read_float(&tempFloat);
		    curNode->Light.Color[2] = tempFloat;
		  }
	      }
	  }
	  else if (!strcmp(token, "FirstFrame"))
	    {
	      lws_read_float(&tempFloat);
	      FirstFrame =  tempFloat;
	    }
	  else if (!strcmp(token, "LastFrame"))
	    {
	      lws_read_float(&tempFloat);
	      LastFrame =  tempFloat;
	    }
	  else if (!strcmp(token, "FramesPerSecond"))
	    {
	      lws_read_float(&tempFloat);
	      if (!FP_EQUAL(tempFloat, 0.0))
		FramesPerSecond =  tempFloat;
	    }
	  else if (!strcmp(token, "EndBehavior"))
	    {
	      if (!pastObjects)
		{
		  lws_get_token();
		  curNode->EndBehavior =  strtol(token, NULL, 10);
		}
	    }
	  break;
	default:
	  break;
	}
    }
  K9_CloseByteStream(stream);



  stream = K9_OpenByteStream(lws.FileName, Stream_Read, 0);
  if (stream == NULL)
    {
      fprintf(stderr,"Could not open file \"%s\"", lws.FileName);
      return (M2E_BadFile);
    }

  lws.Stream = stream;
  lws.Parent = cur_lws;
  cur_lws = &lws;

  curObject = 0;
  while (TRUE)
    {
      do {
	do {
	  tokenType = lws_get_token();
	} while ((tokenType != T_KEYWORD) && (tokenType != T_END)  );    
	if (tokenType == T_END)
	  {
	    goto third;
	  }
	if (!(strcmp(token, "AddNullObject") && 
	      strcmp(token, "LoadObject") && 
	      (!DoLights || (strcmp(token, "AddLight")))))
	  curObject++;
      } while( (strcmp(token, "ObjectMotion")) && 
	      (!DoLights || (strcmp(token, "LightMotion"))));
      
      while (lws_get_token() != T_NUMBER)
	;
      
      if ((numValues = strtol(token, NULL, 10))!= 9)
	{
	  fprintf(stderr,"Expected value of 9.  Got \"%d\" at line %d\n", numValues);
	}
      else
	{
	  if (lws_get_token() != T_NUMBER)
	    {
	      BadFile;
	    }
	  numKeys = strtol(token, NULL, 10);
	  for (key=0; key<numKeys; key++)
	    {
	      for (j=0; j<numValues; j++)
		{
		  lws_read_float(&tempFloat);
		  keys[key].Motion[j] = tempFloat;
		}
	      for (j=0; j<5; j++)
		{
		  lws_read_float(&tempFloat);
		  keys[key].After[j] = tempFloat;
		}
	      if (!FP_EQUAL(keys[key].After[2], 0.0))
		hier[curObject-1].SplineData = TRUE;
	      if (!FP_EQUAL(keys[key].After[3], 0.0))
		hier[curObject-1].SplineData = TRUE;
	      if (!FP_EQUAL(keys[key].After[4], 0.0))
		hier[curObject-1].SplineData = TRUE;
	    }
	}
      Matrix_Compute(keys[0], &hier[curObject-1]);
    }
  
 third:
  K9_CloseByteStream(stream);
  

  fprintf(outPtr,"Define Group %s_world {\n", scriptName);
  if (setScale != 0)
    fprintf(outPtr,"\tScale { 1 1 -1}\n"); 
  HNode_ChildPrint(outPtr, hier, nodeCount, -1, 1);
  fprintf(outPtr,"}\n\n");


  stream = K9_OpenByteStream(lws.FileName, Stream_Read, 0);
  if (stream == NULL)
    {
      fprintf(stderr,"Could not open file \"%s\"", lws.FileName);
      return (M2E_BadFile);
    }

  lws.Stream = stream;
  lws.Parent = cur_lws;
  cur_lws = &lws;

  fprintf(outPtr,"\n\n");
  fprintf(outPtr,"define class KF_Object from Engine {\n");
  fprintf(outPtr,"\tcharacter \t Target\n");
  fprintf(outPtr,"\tpoint \t\t ObjPivot\n");
  fprintf(outPtr,"\tpoint \t\t PrntPivot\n");
  fprintf(outPtr,"\n");
  fprintf(outPtr,"\tfloatarray \t PosFrames\n");
  fprintf(outPtr,"\tfloatarray \t PosData\n");
  fprintf(outPtr,"\tfloatarray \t PosSplData\n");
  fprintf(outPtr,"\n");
  fprintf(outPtr,"\tfloatarray \t RotFrames\n");
  fprintf(outPtr,"\tfloatarray \t RotData\n");
  fprintf(outPtr,"\tfloatarray \t RotSplData\n");
  fprintf(outPtr,"\n");
  fprintf(outPtr,"\tfloatarray \t SclFrames\n");
  fprintf(outPtr,"\tfloatarray \t SclData\n");
  fprintf(outPtr,"\tfloatarray \t SclSplData\n");
  fprintf(outPtr,"\tpad \t 5\n");
  fprintf(outPtr,"}\n\n\n");


  fprintf(outPtr,"Define array kfobjarray of KF_Object\n");
  fprintf(outPtr,"Define kfobjarray %s_kfengines {\n",scriptName);
  curObject = 0;
  while (TRUE)
    {
      do {
	do {
	  tokenType = lws_get_token();
	} while ((tokenType != T_KEYWORD) && (tokenType != T_END)  );    
	if (tokenType == T_END)
	  {
	    fprintf(outPtr,"}\n");
	    if (outPtr != stderr)
	      fclose(outPtr);


	    fprintf(OutScript,"gcomp -b %s.csf ", scriptName);
	    if (ValidTexScript)
	      fprintf(OutScript, "\"%s\" ",TexFileName);
	    if (ValidMatScript)
	      fprintf(OutScript, "\"%s\" ",MatFileName);
	    for (i=0; i<NumObjNames; i++)
	      {
		ObjName_Process(ObjNames[i], &objName);
		fprintf(OutScript, "\"%s\" ",objName);
	      }
	    fprintf(OutScript,"\"%s\"\n", fileOut);
	    return(M2E_NoErr);
	  }
	if (!(strcmp(token, "AddNullObject") && 
	      strcmp(token, "LoadObject")))
	  curObject++;
      } while( strcmp(token, "ObjectMotion"));
      
      while (lws_get_token() != T_NUMBER)
	;
      
      if ((numValues = strtol(token, NULL, 10))!= 9)
	{
	  fprintf(stderr,"Expected value of 9.  Got \"%d\" at line %d\n", numValues);
	}
      else
	{
	  if (lws_get_token() != T_NUMBER)
	    {
	      BadFile;
	    }
	  numKeys = strtol(token, NULL, 10);
	  if (numKeys > NUM_KEYS)
	    {
	      fprintf(stderr,"WARNING:%d keys but only %d keys will be read\n",
		      numKeys, NUM_KEYS);

	    }
	  for (key=0; key<numKeys; key++)
	    {
	      if (numKeys<NUM_KEYS)
	    {
	      for (j=0; j<numValues; j++)
		{
		  lws_read_float(&tempFloat);
		  keys[key].Motion[j] = tempFloat;
		}
	      for (j=0; j<5; j++)
		{
		  lws_read_float(&tempFloat);
		  keys[key].After[j] = tempFloat;
		}
	      if (!FP_EQUAL(keys[key].After[2], 0.0))
		hier[curObject-1].SplineData = TRUE;
	      if (!FP_EQUAL(keys[key].After[3], 0.0))
		hier[curObject-1].SplineData = TRUE;
	      if (!FP_EQUAL(keys[key].After[4], 0.0))
		hier[curObject-1].SplineData = TRUE;
	    }
	}
	}
      if (numKeys > NUM_KEYS)
	Key_Print(outPtr, keys, NUM_KEYS, curObject, 1, 
		  hier[curObject-1], hier);
      else
      Key_Print(outPtr, keys, numKeys, curObject, 1, 
		hier[curObject-1], hier);
    }
  if (hier != NULL)
    free(hier);
  if (keys != NULL)
    free(keys);
  return(M2E_NoErr);
}


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n", LW_VERSION_STRING);
  printf("   Convert Lightwave Scene files to an ASCII SDF hierarchy/anim file.\n");
  printf("   -l    \tConvert the lights in the scene file.(default=no lights)\n");
  printf("   -hpb <0-11> \tChanges the ordering of the transforms to compensate for gimbal lock. Default=0\n");
  printf("   -sa <Name> \tAppend the model processing script to the end of this file.\n");
  printf("   -sn <Name> \tWrite the model processing script to this file. (default=stderr)\n"); 
  printf("   -tn <Name> \tWrite the SDF TexBlends to this file. (default=output file)\n");
  printf("   -mn <Name> \tWrite the SDF MatArray to this file. (default=output file)\n");
  printf("   -m  \tWrite out the MatArray and the TexArray into a separate file.\n");
  printf("   -e    \tDon't strip off extensions.\n");
  printf("   -case \tDon't convert names to lowercase.\n");
  printf("   -t    \tOnly output geometry, no textures.\n");
  printf("   -smooth \tForce all flat polygons to be smooth.\n");
  printf("   -envmap \tDon't convert environment map data.\n");
  printf("   -envmult \tMultiply enviroment maps by material instead of blending.\n");
}

/* Read the Lightwave file format stuff */

int main(int argc, char *argv[])
{
  M2Err err;
  int argn;
  char fileIn[256];
  char fileOut[256];
  char scriptFile[256];
  bool validScript = FALSE;

#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.lwo dumb.sdf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
  
#else

  if (argc < 3) 
    {
      printf("Usage: %s <FileIn> <FileOut>\n",argv[0]);
	  print_description();
      exit(-1);
    }

  strcpy(fileIn, argv[1]);
  argn = 2;
  while ( (argn < argc) && (argv[argn][0] == '-') && (argv[argn][1] != '\0') )
    {
      if ( strcmp( argv[argn], "-smooth")==0 )
        {
	  ++argn;
	  NoFlat = TRUE;
        }
      else if ( strcmp( argv[argn], "-case")==0 )
        {
	  ++argn;
	  ToLower = FALSE;
        }
      else if ( strcmp( argv[argn], "-envmap")==0 )
        {
	  ++argn;
	  DoEnvironmentMap = !DoEnvironmentMap;
        }
      else if ( strcmp( argv[argn], "-envmult")==0 )
        {
	  ++argn;
	  EnvMult = !EnvMult;
        }
      else if ( strcmp( argv[argn], "-hpb")==0 )
        {
	  ++argn;
	  HPB = atoi(argv[argn]);
	  ++argn;
        }
      else if ( strcmp( argv[argn], "-l")==0 )
        {
	  ++argn;
	  DoLights = TRUE;
        }
      else if ( strcmp( argv[argn], "-m")==0 )
        {
	  ++argn;
	  SepMatTex = TRUE;
        }
      else if ( strcmp( argv[argn], "-t")==0 )
        {
	  ++argn;
	  NoTextures = TRUE;
        }
      else if ( strcmp( argv[argn], "-e")==0 )
        {
	  ++argn;
	  RemoveExtensions = FALSE;
        }
      else if ( strcmp( argv[argn], "-sa")==0 )
        {
	  ++argn;
	  strcpy(scriptFile,argv[argn]);
	  ++argn;
	  OutScript = fopen( scriptFile, "a");
	  if (OutScript == NULL)
	    {
	      printf("ERROR: Can't of script file %s\n",scriptFile);
	      return(-1);
	    }
	  validScript = TRUE;
        }
      else if ( strcmp( argv[argn], "-sn")==0 )
        {
		  ++argn;
		  strcpy(scriptFile,argv[argn]);
		  ++argn;
		  OutScript = fopen( scriptFile, "w");
		  if (OutScript == NULL)
		    {
		      printf("ERROR: Can't of script file %s\n",scriptFile);
		      return(-1);
		    }
		  validScript = TRUE;
        }
      else if ( strcmp( argv[argn], "-tn")==0 )
        {
	  ++argn;
	  strcpy(TexFileName,argv[argn]);
	  ++argn;
	  ValidTexScript = TRUE;
	  FirstTexScript = TRUE;
        }
      else if ( strcmp( argv[argn], "-mn")==0 )
        {
	  ++argn;
	  strcpy(MatFileName,argv[argn]);
	  ++argn;
	  ValidMatScript = TRUE;
	  FirstMatScript = TRUE;
        }
      else
	{
	  printf("ERROR:Unknown argument \"%s\"\n", argv[argn]);
	  printf("Usage: %s <FileIn> <FileOut>\n",argv[0]);
	  print_description();
	  return(-1);
	}
    }
  
  /* Open the input file. */
  if (argn != argc)
    {
      strcpy( fileOut, argv[argn] );
    }
  else
    {
      /* No input file specified. */
      printf("Usage: %s <FileIn> <FileOut>\n",argn, argc, argv[0]);
      return(-1);   
    }

#endif

  if (!validScript)
    {
  OutScript = stderr;
  }
  err = LWS_Process(fileIn, fileOut, OutScript);

  if (OutScript != stderr)
    fclose(OutScript);
  
  if (err == M2E_NoErr)
    return(0);
  else
    return(-1);
}
