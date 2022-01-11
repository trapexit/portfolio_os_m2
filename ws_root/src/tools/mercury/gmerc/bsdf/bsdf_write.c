#include "bsdf_iff.h"
#include "writeiff.h"

#include "gp.i"
#include "tmutils.h"
#include "bsdf_proto.h"
#include "bsdf_write.h"
#include "bsdf_anim.h"
#include "M2TXTypes.h"
#include "texpage.h"
#include "geoitpr.h"

Endianness gEndian = eMotorola;

static uint32 animflag = GEOM_FLAG | ANIM_FLAG;
static uint32 collapseflag = 1;
static uint32 messageflag = 0;
static uint32 frameworkflag = 0;
int mercid = 1;

SurfaceData *process_Surface(GfxRef surf, uint32 options);

uint32 GetFrameworkFlag(void)          
{
  return frameworkflag;         
}

void SetFrameworkFlag(uint32 flag)         
{
  frameworkflag = flag;         
}

uint32 GetMessageFlag(void)          
{
  return messageflag;         
}

void SetMessageFlag(uint32 flag)         
{
  messageflag = flag;         
}

uint32 GetCollapseFlag(void)
{
  return collapseflag;
}

void SetCollapseFlag(uint32 flag)
{
  collapseflag = flag;
}

uint32 GetAnimFlag(void)
{
  return animflag;
}

void SetAnimFlag(uint32 flag)
{
  animflag = flag;
}

void ResetMercid(void)
{
  mercid = 1;
}

#define INTERFACE_CODE	1
#ifdef INTERFACE_CODE


typedef struct HierItem {
  unsigned int    type      : 1;
  unsigned int    flags     : 15;
  unsigned int    n         : 16;
  Point3          pivot;
  uint32			mdl_id;
} HierItem;

typedef struct CharChunk {
  int32 objid;
  int32 pid;
  int32 flags;
  MatrixData mat;
} CharChunk;

typedef struct MhdrChunk {
  int32 chunkid;
  int32 size;
  int32 objid;
  int32 minid;
  int32 maxid;
  int32 numAM;
  int32 numTexpage;
  int32 numPod;
} MhdrChunk;

typedef struct PodChunkHeader
{
  uint32	flags;
  uint16	caseType;		/* POD case type */
  uint16	texPageIndex;	/* POD texture page index */
  uint32	materialId;
  uint32	lightId;
  uint16	numSharedVerts;
  uint16	numVerts;
  uint32	numVertIndices;
  uint32	numTexCoords;
  uint32	numTriangles;

  float fxmin,fymin,fzmin;
  float fxextent,fyextent,fzextent;
} PodChunkHeader;

static int32 modelid = 1;
IFFParser	*giff = NULL;
static int32 numPod = 0;
static int32 numModel = 0;
static int podindex = 0;

/*
** calculate the chunk location from the start of the file 
*/
static Err
SDFB_GetChunkOffset(
		    IFFParser	*iff,
		    uint32	*offset
		    )
{
  ContextNode *cur;
  *offset = 0;
	
  if ( !( cur = GetCurrentContext( iff ) ) )
    return IFF_PARSE_EOF;
		
  *offset = cur->cn_Offset;
}

static void PrintErrorCase
(
 int32 id
 )
{
  char errstr[32];
  switch (id)
    {
    case Class_Light:
      strcpy(errstr, "light");
      break;
    case Class_Scene:
      strcpy(errstr, "scene");
      break;
    case Class_Camera:
      strcpy(errstr, "camera");
      break;
    case Class_Surface:
      strcpy(errstr, "surface");
      break;
    case Class_Texture:
      strcpy(errstr, "texture");
      break;
    case Class_TexBlend:
      strcpy(errstr, "texblend");
      break;
    case Class_Transform:
      strcpy(errstr, "transform");
      break;
    case Class_PipTable:
      strcpy(errstr, "piptable");
      break;
    case Class_Engine:
      strcpy(errstr, "engine");
      break;
    case Class_Link_Array:
      strcpy(errstr, "link_array");
      break;
    case Class_Data_Array:
      strcpy(errstr, "data_array");
      break;
    case Class_Int32:
      strcpy(errstr, "int32");
      break;
    case Class_Float:
      strcpy(errstr, "float");
      break;
    case Class_Point3:
      strcpy(errstr, "point3");
      break;
    case Class_Color4:
      strcpy(errstr, "color4");
      break;
    case Class_Box3:
      strcpy(errstr, "box3");
      break;
    }
  printf("ERROR: The tool does not handle %s data. Remove %s data from the sdf file.\n", errstr, errstr);
  printf("Stop.\n");
  exit(0);
}

static int32 Merc_GetCharNum
(
 SDFBObject *sdfb,
 CharData *group
 )
{
  CharData *child, *sibling;
  int32 num = 0, num1 = 0, num2 = 0;
	
  if (group == NULL)
    return 0;
  if (((GfxObj *)group)->m_Type == Class_Model)
    num = 1;
  else if (((GfxObj *)group)->m_Type == Class_Character)
    num = 1;
  else
    PrintErrorCase(((GfxObj *)group)->m_Type);
  child = (CharData *)group->m_First;
  if (child)
    num1 = Merc_GetCharNum(sdfb, child);
  sibling = (CharData *)group->m_Next;
  if (sibling)
    num2 = Merc_GetCharNum(sdfb, sibling);
  return num + num1 + num2;
}

static HierItem *hiertable = NULL;
static int32 hiercount = 0;
static TreeInfo tree_info;
/* static int32 treedepth = 0; */
static int32 *modltable = NULL;
static int32 modlcount = 0;
static int32 *animtable = NULL;
static int32 animcount = 0;

static uint32 Merc_StackPop
(
 CharData *group
 )
{
  uint32 stack_pop = 0;
  CharData *cptr;

  cptr = group;
  /* if this is last child of a root node do not increment. 
     this to avoid stack underflow */
  while( cptr && ( cptr->m_Parent != NULL ) && ( cptr->m_Next == NULL ) )
    {
      stack_pop++;
      cptr = ( CharData *)cptr->m_Parent;
    }

  return ( stack_pop );
}

static void SetHeaderName
(
 SDFBObject *sdfb,
 CharData *group,
 int modlcount
 )
{
#if 0
  DictIter iter;
  Symbol *s, *os;
  DictEntry   *e, *oe;
  Character*	c;
  GfxObj*     o;
  int32 new_sym;
  Err result = GFX_OK;
  Dictionary	*addr2entry = NULL;
  void *obj_addr;
  uint32 obj_id;
  char *obj_names = NULL;
  int i, str_len = 0, str_offset = 0;
  char *str_names = NULL;
  uint32 name_count = 0;
  uint32 id_offset[2];
  SDFClass *cls;
  void *addr;
	
  /* create address to original dict entry mapping dict */
  addr2entry = Dict_Create( DICT_NumKeys );

  /* add all the object addresses */
  Dict_ForAll( sdfb->srcSDF->Symbols, &iter );
  while ( e = Dict_Next( &iter ) )
    {
      s = Dict_Get( e, Symbol );
      if ( !Sym_IsObj( s ) ) continue;

      o = Sym_GetObj( s );
      cls = s->Parent;
		
      if (cls->IsUserClass)
	{ 
	  addr = ((char*) o) + cls->Parent->Size;
	} else addr = ( char * ) o;

		
      oe = Dict_Find( addr2entry, addr );
	
      if ( oe == NULL ) 
	{
	  oe = Dict_Add( addr2entry, addr, &new_sym );
	
	  if( !new_sym ) 
	    {
	      GLIB_WARNING(("SDF: could not add fixup value\r"));
	    }
			
	  Dict_SetData( oe, (void *)e );
			
	}
    }

  /* get the object names */
  Dict_ForAll( sdfb->fixup, &iter );
  while ( e = Dict_Next( &iter ) )
    {
		
      obj_addr = Dict_GetNumKey( e );
      obj_id = ( uint32 ) Dict_GetData( e );
		
      oe = Dict_Find( addr2entry, ( char * ) obj_addr );
      if( oe != NULL )
	{
	  oe = ( DictEntry *) Dict_GetData( oe );
	  os = Dict_Get( oe, Symbol );
	  obj_names =  Sym_GetName( os );
	  if (group == (CharData *)obj_addr)
	    AddDefine(obj_names, modlcount);
	}
    }
#endif
}

static void Merc_SetHier
(
 SDFBObject *sdfb,
 CharData *group,
 int32 depth
 )
{
  CharData *child, *sibling, *cptr;
  uint32 type = 0, childnum = 0;
  HierItem am;
	
  if (group == NULL)
    return;
  if (tree_info.treeDepth < depth)
    tree_info.treeDepth = depth;
  *(modltable + modlcount) = ((GfxObj *)group)->m_Objid;
  am.flags = 0;
  am.mdl_id = ((GfxObj *)group)->m_Objid;

  /* setting defines here */
  if (HasHeaderFile())
    SetHeaderName(sdfb, group, modlcount);
#if 0
  printf("modlcount = %d, id = %d\n", modlcount, *(modltable + modlcount));
#endif
  modlcount++;
  if (((GfxObj *)group)->m_Ref)
    *(animtable + animcount) = ((GfxObj *)((GfxObj *)group)->m_Ref)->m_Objid;
  else
    *(animtable + animcount) = 0;
#if 0
  printf("animcount = %d, id = %d\n", animcount, *(animtable + animcount));
#endif
  animcount++;
  child = (CharData *)group->m_First;
  sibling = (CharData *)group->m_Next;
  if ((((GfxObj *)group)->m_Type == Class_Model) || (((GfxObj *)group)->m_Type == Class_Character))
    {
      am.n = 0;
      if (child)
	{
	  am.type = ePushNode;
	  cptr = child;
	  while (cptr)
	    {
	      am.n++;
	      cptr = (CharData *)cptr->m_Next;
	    }
	}
      else
	{
	  am.type = ePopNode;
	  am.n = Merc_StackPop( group );
	}
      if (((GfxObj *)group)->m_Ref)
	{
	  KfEngine *kf_eng = ( KfEngine *)((GfxObj *)group)->m_Ref;
	  am.pivot.x = kf_eng->mObjPivot.x;
	  am.pivot.y = kf_eng->mObjPivot.y;
	  am.pivot.z = kf_eng->mObjPivot.z;
	  if( tree_info.control.beginFrame < kf_eng->startFrame )
	    tree_info.control.beginFrame = kf_eng->startFrame;
	  if( tree_info.control.endFrame < kf_eng->endFrame )
	    tree_info.control.endFrame = kf_eng->endFrame;
	  if( tree_info.control.lockedFPS < kf_eng->mEngine.m_Speed )
	    tree_info.control.lockedFPS = kf_eng->mEngine.m_Speed;
	}
      else
	{
	  am.pivot.x = 0.0;
	  am.pivot.y = 0.0;
	  am.pivot.z = 0.0;
	}
      memcpy(hiertable+hiercount, &am, sizeof(HierItem));
      hiercount++;
      if (((GfxObj *)group)->m_Type == Class_Model)
	if (((ModelData *)group)->m_Surface)
	  tree_info.numPods += ((ModelData *)group)->m_NumPod;
    }
  else
    PrintErrorCase(((GfxObj *)group)->m_Type);
  if (child)
    Merc_SetHier(sdfb, child, depth+1);
  if (sibling)
    Merc_SetHier(sdfb, sibling, depth);
}

/*
**	Hierarchy Collapser. 
**	The last two nodes gets collapsed because an extra node is introduced
**	in the conversion tools to get around the SDF parser ideosynchrosy
**	of not treating the model as a hierarchy node.
**	Hierarchy collapse happens with the following conditions :
**	
**	1. The leaf (leaf) node and it's parent ( root0 ) should exist.
**	2. root0 should have animation attached to it ( for now ).
**	3. if the leaf is shared then do not collapse ( collapsed for now ) 
*/

#define MAX_LEAF_NODE_COUNT 2000
/*
** collapse the two levels from the leaf node 
*/
static void Merc_NodeCollpse
(
 SDFBObject *sdfb,
 CharData *group
 )
{
  CharData *root0, *prnt;
  CharData *root1, *leaf;
  CharData *ch;
  unsigned int i, j;


  /* the leaf node must be a model and need to have a parent 
     to collapse with */
  if ( ( ((GfxObj *)group)->m_Type != Class_Model ) || 
       ( ((CharData *)group)->m_Parent == NULL ) ) return;

#if 0
  fprintf( stderr, "Model = 0x%x, Use Count = %d\n", 
	   group, Obj_GetUse( (GfxObj *)group ) );
#endif

#if 0
  /* if the model is used multiple time then do not collapse */
  /* FIX ME: model sharing is not working properly, when this works
     enable the following line to suppress collapse of the shared
     models */
  if( Obj_GetUse( (GfxObj *)group ) > 1 ) return;
#endif

  leaf = (CharData *)group;
  root0 = (CharData *)leaf->m_Parent;
  prnt = (CharData *)root0->m_Parent;

  /* if the parent node does not have animation then do not collapse.
  ** this is to preserve the local transform for the hierarchy node.
  ** when there is an animation this local transform will be replaced
  ** by the animation definition so it can be collapsed
  ** FIX ME: because of the way animation nodes are collected 
  ** collapsing non-animated nodes are not possible. This is because
  ** models gets written out with leaf node matrix before collapse happens
  */ 
  if( (((GfxObj *)root0)->m_Ref == NULL) || ( prnt == NULL) ) return;
 
  /* disconnect the root and the leaf node */

#ifdef GMERC_V1
  Char_Remove( (CharData *)leaf );
  Char_Remove( (CharData *)root0 );
#endif

  /* connect root children to the leaf node */
  ch = leaf->m_First =  root0->m_First;
  root0->m_First =  NULL;
  while( ch )
    {
      ((CharData *)ch)->m_Parent = (CharData *)leaf;
      ch = ((CharData *)ch)->m_Next;
    }
	
  /* assign root animation to the model */
  ((GfxObj *)leaf)->m_Ref = ((GfxObj *)root0)->m_Ref;

  /* assign root transform to the leaf */
  for( i = 0; i < 4; i++ )
    for( j = 0; j < 4; j++ )
      leaf->m_Transform.data[i][j] = root0->m_Transform.data[i][j];
	
  /* Append the leaf node to the parent */	
#ifdef GMERC_V1
  Char_Append( (Character *)prnt, (Character *)leaf );
#endif

  /* delete the extra two nodes */
#ifdef GMERC_V1
  Char_Delete( (Character *)root0 );
#endif
}

/*
** hierarchy collapser
*/
static void Merc_HierCollectLeafNodes
(
 SDFBObject *sdfb,
 CharData *group,
 CharData **leaf_buff,
 uint32 *leaf_count
 )
{
  CharData*  c;
  uint32 i, j;

  if( group == NULL ) return;

  c = ((CharData *)group)->m_First;

  /* if this is a leaf node then collapse data and return */
  if( c == NULL ) 
    {
      leaf_buff[*leaf_count] = group;
      *leaf_count += 1;

      /* check for temp leaf node buffer over-run */
      if( *leaf_count >= MAX_LEAF_NODE_COUNT )
	{
	  fprintf( stderr, "ERROR: Leaf node buffer over flow\n" );
	  exit( 0 );
	}
      return;
    }

  while ( c )
    {
      Merc_HierCollectLeafNodes( sdfb, c, leaf_buff, leaf_count );	
      c = ((CharData *)c)->m_Next;
    }
}

/*
** hierarchy collapser
*/
static void Merc_HierCollapse
(
 SDFBObject *sdfb,
 CharData *group
 )
{
  CharData * leaf_buff[MAX_LEAF_NODE_COUNT];
  uint32 leaf_count = 0;
  uint32 i;

  Merc_HierCollectLeafNodes( sdfb, 
			     group, 
			     leaf_buff, &leaf_count );

  for( i = 0; i < leaf_count; i++ )
    Merc_NodeCollpse( sdfb, leaf_buff[i] );
}

static void Merc_WriteHier
(
 SDFBObject *sdfb,
 CharData *group
 )
{
  Err result;
  CharData *child, *sibling;
  int32 num;
  int32 mchunk[2];
  int32 achunk[2];
  int32 i;
	
  if (group == NULL)
    return;
  num = Merc_GetCharNum(sdfb, group);
  hiertable = (HierItem *) malloc(num * sizeof(HierItem));
  modltable = (int32 *) malloc(num * sizeof(int32));
  animtable = (int32 *) malloc(num * sizeof(int32));
  tree_info.numNodes = num;
  tree_info.treeDepth = 0;
  tree_info.numPods = 0;
  tree_info.control.flags = updateTheAnim | updateToRealFPS | cycleTheAnim;
  tree_info.control.beginFrame = 0.0;
  tree_info.control.endFrame = 0.0;
  tree_info.control.lockedFPS = 30.0;
  tree_info.control.curTime = 0.0;
  tree_info.control.curInc = 1.0;
  hiercount = 0;
  modlcount = 0;
  animcount = 0;

  Merc_SetHier(sdfb, group, 1);

  tree_info.control.timeStart = tree_info.control.beginFrame;
  tree_info.control.timeEnd = tree_info.control.endFrame;
  /* write out the hierarchy data here */
  if (GetAnimFlag() & GEOM_FLAG)
    {
      result = PushChunk( sdfb->iff, 0L, ID_AMDL, IFF_SIZE_UNKNOWN_32 );
      if( result < 0 ) goto err;
    }

  if (GetAnimFlag() & GEOM_FLAG)
    {
      uint32 tmplong;
      /* begin write HIER chunk */
      result = PushChunk( sdfb->iff, 0L, ID_HIER, IFF_SIZE_UNKNOWN_32 );
      if( result < 0 ) goto err;
      if( PutUInt32s( sdfb->iff, (uint32 *)&tree_info, 3, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      for (i = 0; i < num; i++)
	{
#ifdef __WINTEL__
	  tmplong = 0;
	  if (hiertable[i].type)
	    tmplong += 1 << 31;
	  if (hiertable[i].flags)
	    tmplong += hiertable[i].flags << 30;
	  if (hiertable[i].n)
	    tmplong += hiertable[i].n;
	  if( PutUInt32s( sdfb->iff, (uint32 *)&tmplong, 1, gEndian ) == -1 )
	    {
	      result = -1;
	      goto err;
	    }
#else
	  if( PutUInt32s( sdfb->iff, (uint32 *)&hiertable[i], 1, gEndian ) == -1 )
	    {
	      result = -1;
	      goto err;
	    }	
#endif
	  if( PutFloats( sdfb->iff, (float *)&hiertable[i].pivot, 3, gEndian ) == -1 )
	    {
	      result = -1;
	      goto err;
	    }	
	  if( PutUInt32s( sdfb->iff, (uint32 *)&hiertable[i].mdl_id, 1, gEndian ) == -1 )
	    {
	      result = -1;
	      goto err;
	    }	
	}
      result = PopChunk( sdfb->iff );
      if( result < 0 ) goto err;
      /* end write HIER chunk */
    }
  if (GetAnimFlag() & ANIM_FLAG)
    {
      /* begin write ANMA chunk */
      result = PushChunk( sdfb->iff, 0L, ID_ANMA, IFF_SIZE_UNKNOWN_32 );
      if( result < 0 ) goto err;
      if( PutUInt32( sdfb->iff, tree_info.control.flags, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutFloats( sdfb->iff, (float *)&tree_info.control.beginFrame, 7, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutInt32s( sdfb->iff, animtable, num, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      result = PopChunk( sdfb->iff );
      if( result < 0 ) goto err;
      /* end write ANMA chunk */
    }

  if (GetAnimFlag() & GEOM_FLAG)
    {
      result = PopChunk( sdfb->iff );
      if( result < 0 ) goto err;
    }
  if (hiertable)
    free(hiertable);
  if (modltable)
    free(modltable);
  if (animtable)
    free(animtable);
  hiercount = 0;
  tree_info.treeDepth = 0;
  modlcount = 0;
  animcount = 0;
err:
  return;
}

static void Merc_WriteCharAll
(
 SDFBObject *sdfb,
 CharData *group
 )
{
  Err result;
  CharData *child, *sibling;
  CharChunk cc;
  uint32 id;
  int32 chunk[3];
  SurfaceData *surf;
  PrimHeader *pSrcHeader;
	
  if (group == NULL)
    return;
  if ((((GfxObj *)group)->m_Type != Class_Model) && (((GfxObj *)group)->m_Type != Class_Character))
    PrintErrorCase(((GfxObj *)group)->m_Type);

  ((GfxObj *)group)->m_Objid = mercid++;
  cc.objid = ((GfxObj *)group)->m_Objid;
  cc.pid = 0;
  cc.flags = SDFB_Char_HasTransform;
  memcpy(&cc.mat[0][0], &group->m_Transform.data[0][0], 64);
	
  if (((GfxObj *)group)->m_Type == Class_Model)
    {
      /* begin write ID_MODL chunk */
      result = PushChunk( sdfb->iff, 0L, ID_MODL, IFF_SIZE_UNKNOWN_32 );
      if( result < 0 ) goto err;
      if( PutInt32( sdfb->iff, cc.objid, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}
      if( PutInt32( sdfb->iff, cc.pid, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutInt32( sdfb->iff, cc.flags, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}
      if( PutFloats( sdfb->iff, (float *)&cc.mat, 16, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if (((ModelData *)group)->m_Surface)
	{
	  int32 index;
	  ModelData *tmpmdl;
	  SurfaceData *tmpsurf;
			
	  index = ((ModelData *)group)->m_ModelIndex;
	  tmpmdl = (ModelData *)Glib_GetModel(index);
	  tmpsurf = (SurfaceData *)(tmpmdl->m_Surface);
	  ((ModelData *)group)->m_NumPod = tmpmdl->m_NumPod;
	  chunk[0] = ((GfxObj *)tmpsurf)->m_Objid;
	  /* if (((ModelData *)group)->m_Materials) */
	  chunk[1] = Glib_GetMatId();
	  /*
	    else
	    chunk[1] = 0; */
	  chunk[2] = 0;
	}
      else
	{
	  chunk[0] = 0;
	  chunk[1] = 0;
	  chunk[2] = 0;
	  printf("ERROR: No model data is defines\n");
	}
      if( PutInt32s( sdfb->iff, &chunk[0], 3, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      result = PopChunk( sdfb->iff );
      if( result < 0 ) goto err;
      /* end write ID_MODL chunk */
    }
  else if (((GfxObj *)group)->m_Type == Class_Character)
    {
      /* begin write ID_CHAR chunk */
      result = PushChunk( sdfb->iff, 0L, ID_CHAR, IFF_SIZE_UNKNOWN_32 );
      if( result < 0 ) goto err;
      if( PutInt32( sdfb->iff, cc.objid, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutInt32( sdfb->iff, cc.pid, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutInt32( sdfb->iff, cc.flags, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}
      if( PutFloats( sdfb->iff, (float *)&cc.mat, 16, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      result = PopChunk( sdfb->iff );
      if( result < 0 ) goto err;
      /* end write ID_CHAR chunk */
    }

  child = (CharData *)group->m_First;
  if (child)
    Merc_WriteCharAll(sdfb, child);
  sibling = (CharData *)group->m_Next;
  if (sibling)
    Merc_WriteCharAll(sdfb, sibling);
err:
  return;
}

static void Merc_WriteMata
(
 SDFBObject *sdfb
 )
{
  MatProp *mat = NULL, *tmat = NULL;
  uint32 mata_len = Glib_GetMatCount();
  uint32 mat_size = sizeof( MatProp );
  int32 i, objID;
  Err result = GFX_OK;
	
  result = PushChunk( sdfb->iff, 0L, ID_MATA, IFF_SIZE_UNKNOWN_32 );
  if( result < 0 ) goto err;

  if (mata_len == 0) {
    printf("WARNING: no material in the sdf file. A default material is applied.\n");
    tmat = Mat_Create();
    Col_SetRGB(&tmat->Diffuse, 0.7, 0.7, 0.7);
    Col_SetRGB(&tmat->Specular, 1, 1, 1);
    Col_SetRGB(&tmat->Emission, 0, 0, 0);
    Col_SetRGB(&tmat->Ambient, 0.3, 0.3, 0.3);
    tmat->Shine = 0.05;
    Glib_AddMat(tmat);
  }

  Glib_SetMatId(mercid++);
  objID = Glib_GetMatId();

  if( PutInt32( sdfb->iff, objID, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
	    
  for( i = 0; i < mata_len; i++, mat++ )
    {	
      mat = Glib_GetMat(i);
      if( PutUInt32( sdfb->iff, mat->ShadeEnable, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutFloats( sdfb->iff, (float *)&mat->Diffuse, 4, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutFloats( sdfb->iff, (float *)&mat->Specular, 4, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutFloats( sdfb->iff, (float *)&mat->Emission, 4, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutFloats( sdfb->iff, (float *)&mat->Ambient, 4, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
      if( PutFloat( sdfb->iff, mat->Shine, gEndian ) == -1 )
	{
	  result = -1;
	  goto err;
	}	
    }	
	    
  result = PopChunk( sdfb->iff );
  if( result < 0 ) goto err;

err:
  return;
}

static Err Merc_WritePodgChunk
(
 SDFBObject *sdfb,
 uint32 *geom,
 int32 podsize
 )
{
  Err result = GFX_OK;
  SurfToken *tok;
  uint16 *ui16;
  PodChunkHeader hd;

  tok = (SurfToken *)geom;	
  memcpy( &hd, tok, sizeof( PodChunkHeader ) );
  /* begin PodChunkHeader */
  if( PutUInt32( sdfb->iff, hd.flags, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutUInt16( sdfb->iff, hd.caseType, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutUInt16( sdfb->iff, hd.texPageIndex, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutUInt32s( sdfb->iff, &hd.materialId, 2, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutUInt16( sdfb->iff, hd.numSharedVerts, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutUInt16( sdfb->iff, hd.numVerts, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutUInt32s( sdfb->iff, &hd.numVertIndices, 3, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  if( PutFloats( sdfb->iff, &hd.fxmin, 6, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  /* end PodChunkHeader */
  /* begin vertices */
  tok = (SurfToken *)( (char *)tok + sizeof( PodChunkHeader ) );
  if (hd.numSharedVerts > 0) {
    if( PutFloats( sdfb->iff, (float *)tok, 6 * hd.numVerts, gEndian ) == -1 )
      {
	result = -1;
	goto err;
      }	
    tok += 6 * hd.numVerts;
    ui16 = (uint16*)tok;
    if( PutUInt16s( sdfb->iff, (uint16 *)ui16, 2* hd.numSharedVerts, gEndian ) == -1 )
      {
	result = -1;
	goto err;
      }	
    tok = (SurfToken *)((uint16 *)tok + 2 * hd.numSharedVerts);
  }
  else {
    if( PutFloats( sdfb->iff, (float *)tok, 6 * hd.numVerts, gEndian ) == -1 )
      {
	result = -1;
	goto err;
      }	
    tok += 6 * hd.numVerts;
  }
  /* end vertices */

  /* begin indices */
  ui16 = (uint16*)tok;
  if( PutUInt16s( sdfb->iff, (uint16 *)ui16, hd.numVertIndices, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  /* end indices */
  tok = (SurfToken *)( ui16 + hd.numVertIndices );
  /* begin texture coordinate */
  if( PutFloats( sdfb->iff, (float *)tok, 2 * hd.numTexCoords, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  /* end texture coordinate */
err:
  return result;
}

static void Merc_WritePods
(
 SDFBObject *sdfb,
 CharData *group
 )
{
  Err result = GFX_OK;
  CharData *child, *sibling;
  SurfaceData *surf;
  PrimHeader *pSrcHeader;
  SurfaceData*	csurf = NULL, *oldsurf = NULL;
  GeoPrim *geo;
  uint32 chunk[2];
  int32 schunk[2];
  int32 podsize;
  char *podptr;
  PodChunkHeader *hd;	
  MatProp *mat;
  int i;
  uint32 offset;
  int32 surfpod = 0;
  ModelData *tmpmdl;
  SurfaceData *tmpsurf;

  if (group == NULL)
    return;
  if (((GfxObj *)group)->m_Type == Class_Model)
    {
      if ( result < 0 ) goto err;	    
      tmpmdl = (ModelData *)Glib_GetModel(((ModelData *)group)->m_ModelIndex); /* Get the REAL model */
      if (tmpmdl->m_Visited == 0)
	if (tmpmdl->m_Surface)
	  {
	    tmpmdl->m_Visited = 1;
		
	    surf = (SurfaceData *) tmpmdl->m_Surface;
	    oldsurf = surf;
	    result = SDFB_GetChunkOffset( sdfb->iff, &offset);
	    offset += 12;
	    /* begin write ID_SURF chunk */
	    result = PushChunk( sdfb->iff, 0L, ID_SURF, IFF_SIZE_UNKNOWN_32 );
	    if( result < 0 ) goto err;
	    ((GfxObj *)surf)->m_Objid = mercid++;
	    tmpsurf = (SurfaceData *)tmpmdl->m_Surface;
	    ((GfxObj *)tmpsurf)->m_Objid = ((GfxObj *)surf)->m_Objid;

	    schunk[0] = ((GfxObj *)surf)->m_Objid;
	    schunk[1] = 0;
	    if( PutInt32s( sdfb->iff, &schunk[0], 2, gEndian ) == -1 )
	      {
		result = -1;
		goto err;
	      }	
	    result = PopChunk( sdfb->iff );
	    if( result < 0 ) goto err;
	    /* end write ID_SURF chunk */

	    csurf = (SurfaceData *)process_Surface( tmpmdl->m_Surface, sdfb->compileOptions );
	    tmpmdl->m_Surface = (GfxRef)csurf;
	    surf = (SurfaceData *) csurf;
	    ((GfxObj *)surf)->m_Objid = schunk[0];
	    tmpmdl->m_Surface = (GfxObj *)surf;

	    pSrcHeader = (PrimHeader *)surf->m_FirstPrim;
	    while (pSrcHeader)
	      {
		geo = ( ( GeoPrim* ) pSrcHeader );
		/* start writing the trimesh chunk data */
		if (pSrcHeader->primType == GEO_TriMesh) 
		  {
		    if( sdfb->compileOptions & SDF_CompileSurfaces )
		      {
			result = PushChunk( sdfb->iff, 0L, ID_PODG, IFF_SIZE_UNKNOWN_32 );
			chunk[0] = pSrcHeader->podindex;	/* pod id */ 
			chunk[1] = ((GfxObj *)surf)->m_Objid; /* surf id */
			if( PutUInt32s( sdfb->iff, chunk, 2, gEndian ) == -1 )
			  {
			    result = -1;
			    goto err;
			  }	
			hd = (PodChunkHeader *)(pSrcHeader + 1);
			hd->texPageIndex = (uint16)pSrcHeader->texpage;
			if (Glib_GetMatCount())
			  {
			    mat = Glib_GetMat(hd->materialId);
			    if (hd->caseType == DynLitCase)
			      {
				if ((mat->ShadeEnable & MAT_Diffuse) && 
				    (mat->Diffuse.a != 1.0))
				  {
				    hd->caseType = DynLitTransCase;
				  }
				if (mat->ShadeEnable & MAT_Specular)
				  {
				    if (hd->caseType != DynLitTransCase)
				      {
					hd->caseType = DynLitSpecCase;
					hd->flags = hd->flags | specularFLAG;
				      }
				    else
				      {
					hd->caseType = DynLitTransSpecCase;
					hd->flags = hd->flags | specularFLAG;
				      }
				  }
			      }
			    else if (hd->caseType == DynLitTexCase)
			      {
				if ((mat->ShadeEnable & MAT_Diffuse) &&
				    (mat->Diffuse.a != 1.0))
				  {
				    hd->caseType = DynLitTransTexCase;
				  }
				if (mat->ShadeEnable & MAT_Specular)
				  {
				    if (hd->caseType != DynLitTransTexCase)
				      {
					hd->caseType = DynLitSpecTexCase;
					hd->flags = hd->flags | specularFLAG;
				      }
				    else
				      printf("WARNING: Can not mix DynLitTransTexCase with DynLitSpecTexCase. Assuming DynLitTransTexCase is used.\n");
				  }
			      }
			  }
			if (pSrcHeader->texgenkind)
			  {
			    switch (hd->caseType)
			      {
			      case DynLitTexCase:
			      case DynLitCase:
				hd->caseType = DynLitEnvCase;
				hd->flags = hd->flags | environFLAG;
				break;
			      case DynLitTransTexCase:
			      case DynLitTransCase:
				hd->caseType = DynLitTransEnvCase;
				hd->flags = hd->flags | environFLAG;
				break;
			      case DynLitSpecTexCase:
			      case DynLitSpecCase:
				hd->caseType = DynLitSpecEnvCase;
				hd->flags = hd->flags | environFLAG;
				break;
			      default:
				break;
			      }
			  }

			podsize = pSrcHeader->primSize - sizeof( PrimHeader );
			podptr = (char *)( pSrcHeader + 1 );
			Merc_WritePodgChunk(sdfb, (uint32 *)podptr, podsize);
			result = PopChunk( sdfb->iff );
			numPod++;
		      } else
			printf("This is not a PODG chunk data = %x\n");
		  } else
		    printf("This is not a PODG chunk data = %x\n");
		pSrcHeader = pSrcHeader->nextPrim;
		tmpmdl->m_NumPod++;
		surfpod++;
	      }
	    Surf_Delete(oldsurf);

	    {
	      IFFParser *iff = sdfb->iff;
	      ContextNode *top;
	      uint32 *ids;
	      int32 rbytes = 4;

	      if ( !( top = GetCurrentContext( iff ) ) )
		{
		  result = IFF_PARSE_EOF;
		  goto err;
		}
	      ids = ( uint32 * )&surfpod;

	      result = (* iff->iff_IOFuncs->io_Seek)( iff, - top->cn_Offset + offset );
	      if( result < 0 ) goto err;
    
#ifdef __WINTEL__
	      *ids = Swap32Bits(*ids);
#endif
	      result = (* iff->iff_IOFuncs->io_Write)( iff, ids, rbytes );
	      if( result < 0 ) goto err;
	      /* seek to the original position */
	      result = (* iff->iff_IOFuncs->io_Seek)( iff, top->cn_Offset - ( rbytes + offset ) );
	      if( result < 0 ) goto err;
	    }
	  }
    }
  child = (CharData *)group->m_First;
  if (child)
    Merc_WritePods(sdfb, child);
  sibling = (CharData *)group->m_Next;
  if (sibling)
    Merc_WritePods(sdfb, sibling);
err:
  return;
}

#endif

static void OBJN_CharSetName
(
 CharData *group,
 char **obj_names,
 int32 *str_len,
 int32 *name_count
 )
{
  CharData *child, *sibling;
  uint32 objid;

  objid = ((GfxObj *)group)->m_Objid;
  if (group == NULL)
    return;
  obj_names[objid] = group->name;
  *str_len = *str_len + strlen(obj_names[objid]) + 1;
  *name_count = *name_count + 1;
  child = (CharData *)group->m_First;
  if (child)
    OBJN_CharSetName(child, obj_names, str_len, name_count);
  sibling = (CharData *)group->m_Next;
  if (sibling)
    OBJN_CharSetName(sibling, obj_names, str_len, name_count);
}

static Err 
OBJN_WriteChunk
(
 SDFBObject *sdfb
 )
{
  int32 i;
  CharData *group;
  char **obj_names = NULL;
  char *str_names = NULL;
  uint32 id_offset[2];
  int32 str_len = 0, str_offset = 0;
  Err result = GFX_OK;
  int32 name_count = 0;
  uint32 objid;

  obj_names = (char **)malloc(sizeof(char *) * (mercid + 1));
  for (i = 0; i <= mercid; i++)
    obj_names[i] = NULL;

	
  objid = Glib_GetMatId();
  obj_names[objid] = Glib_GetMatArrayName();
  str_len = str_len + strlen(obj_names[objid]) + 1;
  name_count++;

  for (i = 0; i < Glib_GetRootCharCount(); i++)
    {
      group = Glib_GetRootChar(i);
      OBJN_CharSetName(group, obj_names, &str_len, &name_count);
    }
  result = PushChunk(sdfb->iff, 0L, ID_OBJN, IFF_SIZE_UNKNOWN_32);
  if (result < 0) goto cleanup;
  if (WriteChunk(sdfb->iff, &name_count, 4L) != 4)
    {
      result = -1;
      goto cleanup;
    }
  str_names = (char *)malloc(sizeof(char) * str_len);
  for ( i = 0; i<= mercid; i++)
    {
      if (obj_names[i] != NULL)
	{
	  id_offset[0] = i;
	  id_offset[1] = str_offset;
	  strcpy(str_names+str_offset, obj_names[i]);
	  str_offset += strlen(obj_names[i]) + 1;
	  if (WriteChunk(sdfb->iff, id_offset, 8L) != 8)
	    {
	      result = -1;
	      goto cleanup;
	    }
	}
    }
  if (WriteChunk(sdfb->iff, str_names, str_len) != str_len)
    {
      result = -1;
      goto cleanup;
    }

  result = PopChunk(sdfb->iff);
  if (result < 0) goto cleanup;

cleanup:
  if (obj_names) free(obj_names);
  if (str_names) free(str_names);
	
  return result;
}


/*
** Function to patch the 'First ID' and 'Max ID' fields in SHDR chunk
*/
static Err
SHDR_CallBack(
	      SDFBObject *sdfb,
	      void *userData, 
	      void *userData1,
	      MhdrChunk *mh
	      )
{
  IFFParser *iff = sdfb->iff;
  ContextNode *top;
  uint32 *ids;
  int32 *user_ids;
  Err result;
  int32 i;
  /*
   * minid
   * maxid
   * numAM
   * numTexpage
   * numPod
   */
  int32	rbytes = 20;
  uint32 offset = sdfb->hdrChunkOffset + 12; /* offset to the First ID field */

  if ( !( top = GetCurrentContext( iff ) ) )
    {
      result = IFF_PARSE_EOF;
      goto err;
    }

  ids = ( uint32 * )userData;
  user_ids = ( int32 * )userData1;

  result = (* iff->iff_IOFuncs->io_Seek)( iff, - top->cn_Offset + offset );
  if( result < 0 ) goto err;
    
#ifdef INTERFACE_CODE
  ids = ( uint32 * ) &mh->minid;
  /* write object id data */
#ifdef __WINTEL__
  for (i = 0; i < rbytes / 4; i++)
    *(ids+i) = Swap32Bits(*(ids+i));
#endif
  result = (* iff->iff_IOFuncs->io_Write)( iff, ids, rbytes );
  if( result < 0 ) goto err;
  /* seek to the original position */
  result = (* iff->iff_IOFuncs->io_Seek)( iff, top->cn_Offset - ( rbytes + offset ) );
  if( result < 0 ) goto err;
#endif
    
err:
#ifdef SDFB_DEBUG    
  fprintf( stderr, "SHDR_CallBack IDs ( %d, %d ), result = %d\r", ids[0], ids[1], result );
#endif

  return ( result );
} 

/*
**	Initialize the data for the parser
*/
static Err
SDFB_InitWrite(
	       IFFParser	*iffp,
	       SDFBObject	*sdfb,
	       uint32		options
	       )
{
  Err result;
	
  sdfb->iff = iffp;
	
  sdfb->objIdRange[0] = 1;
  sdfb->objIdRange[1] = 0;
  sdfb->userIdRange[0] = 1;
  sdfb->userIdRange[1] = 0;
	
  /* sdfb->srcSDF = sdf; */
  sdfb->compileOptions = options;

  /* initialize the write functions */
  /* SDFB_InstallWriteMethods( sdfb ); */

  /* initialize the address-to-ID mapping dictionary */
  /* sdfb->fixup = Dict_Create( DICT_NumKeys ); */
  /*
    if ( sdfb->fixup == NULL )
    {
    result = GFX_ErrorNoMemory;
    if( result < 0 ) goto err;
    }
    */

  /* initialize the user class address-to-ID mapping dictionary */
  /* sdfb->userClasses = Dict_Create( DICT_NumKeys ); */
  /*
    if ( sdfb->userClasses == NULL )
    {
    result = GFX_ErrorNoMemory;
    if( result < 0 ) goto err;
    }
    */
		
  /* write SFDB form chunk */
  result = PushChunk( iffp, ID_SDFB, ID_FORM, IFF_SIZE_UNKNOWN_32 );

err:
  return ( result );
}

/*
**	Initialize the data for the parser
*/
static Err
SDFB_EndWrite(
	      SDFBObject	*sdfb,
	      MhdrChunk *mh
	      )
{
  Err result;

  /* result = OBJN_WriteChunk( sdfb );
     if( result < 0 ) goto err; */

  /*
    Dict_Delete( sdfb->fixup );
    Dict_Delete( sdfb->userClasses );
    */
			
  /* store current chunk offset */
  sdfb->objIdRange[0] = 1;
  sdfb->objIdRange[1] = mercid - 1;
  sdfb->userIdRange[0] = 1;
  sdfb->userIdRange[1] = mercid - 1;
  result = SHDR_CallBack( sdfb, &sdfb->objIdRange, &sdfb->userIdRange , mh);
  if( result < 0 ) goto err;
	
  /* pop SFDB form chunk */ 
  result = PopChunk( sdfb->iff );
  if( result < 0 ) goto err;

  /* result = SDFB_FreeSortedMembers( sdfb );
     if( result < 0 ) goto err; */
	
err:
  return ( result );
}

/*
**	Write SAGE data
*/
static Err
SAQM_WriteChunk(
		SDFBObject* pData, 
		PrimHeader* pSrcHeader,
		uint32 surfID
		)
{
  uint32 data1[2], data2[3];
  uint32 flags = 0, chunk_size;
  uint16 indxs[2];
  GeoPrim *geo_prim;
  QuadMesh *geom;
  Err result = GFX_OK;

  printf("SAQM_WriteChunk\n");
  return GFX_OK;
  /* non-compiled primitive */
  result = PushChunk( pData->iff, 0L, ID_SAQM, IFF_SIZE_UNKNOWN_32 );
  if( result < 0 ) goto err;

  geo_prim = ( ( GeoPrim* ) pSrcHeader );
  geom = ( QuadMesh * ) ( &( geo_prim->geoData ) );

  /* get the vertex flags */
  if( geom->Style & GEO_Colors ) 
    flags |= ( SDFB_Geo_UseColors | SDFB_Geo_HasColors );
  if( geom->Style & GEO_Normals ) 
    flags |= ( SDFB_Geo_UseNormals | SDFB_Geo_HasNormals );
  if( geom->Style & GEO_TexCoords ) 
    flags |= ( SDFB_Geo_UseTexCoords | SDFB_Geo_HasTexCoords );

  data1[0] = surfID;
  /* data1[1] = geo_prim->primType; */
  data2[0] = geom->XSize;
  data2[1] = geom->YSize;
  data2[2] = flags;
  indxs[0] = geo_prim->matIndex;
  indxs[1] = geo_prim->texIndex;

#ifdef SDFB_DEBUG
  fprintf( stderr, "Non-compiled Primitive type = %d\r", data1[1] );
#endif

  /* write header data */	 
  if( WriteChunk( pData->iff, data1, 4L ) != 4 )
    {
      result = -1;
      goto err;
    }	
  if( WriteChunk( pData->iff, indxs, 4L ) != 4 )
    {
      result = -1;
      goto err;
    }	
  if( WriteChunk( pData->iff, data2, 12L ) != 12 )
    {
      result = -1;
      goto err;
    }	
	
  /* write position data */
  chunk_size = geom->Size * sizeof( Point3 );
  if( WriteChunk( pData->iff, geom->Locations, chunk_size ) != chunk_size )
    {
      result = -1;
      goto err;
    }	

  /* write normals data */
  if( geom->Style & GEO_Normals )
    {
      chunk_size = geom->Size * sizeof( Vector3 );
      if( WriteChunk( pData->iff, geom->Normals, chunk_size ) != chunk_size )
	{
	  result = -1;
	  goto err;
	}	
    }

  /* write colors data */
  if( geom->Style & GEO_Colors )
    {
      chunk_size = geom->Size * sizeof( Color4 );
      if( WriteChunk( pData->iff, geom->Colors, chunk_size ) != chunk_size )
	{
	  result = -1;
	  goto err;
	}	
    }
		
  /* write normals data */
  if( geom->Style & GEO_TexCoords )
    {
      chunk_size = geom->Size * sizeof( TexCoord );
      if( WriteChunk( pData->iff, geom->TexCoords, chunk_size ) != chunk_size )
	{
	  result = -1;
	  goto err;
	}	
    }
	
  result = PopChunk( pData->iff );
  if( result < 0 ) goto err;

err:    
  return result;
}

/*
**	Write SAGE data
*/
static Err
SAGE_WriteChunk(
		SDFBObject* pData, 
		PrimHeader* pSrcHeader,
		uint32 surfID
		)
{
  uint32 data1[2], data2[2];
  uint32 flags = 0, chunk_size;
  uint16 indxs[2];
  GeoPrim *geo_prim;
  Geometry *geom;
  Err result = GFX_OK;

  printf("SAGE_WriteChunk\n");
  return GFX_OK;
  /* non-compiled primitive */
  result = PushChunk( pData->iff, 0L, ID_SAGE, IFF_SIZE_UNKNOWN_32 );
  if( result < 0 ) goto err;

  geo_prim = ( ( GeoPrim* ) pSrcHeader );

  data1[0] = surfID;
  data1[1] = geo_prim->primType;
  indxs[0] = geo_prim->matIndex;
  indxs[1] = geo_prim->texIndex;

#ifdef SDFB_DEBUG
  fprintf( stderr, "Non-compiled Primitive type = %d\r", data1[1] );
#endif

  /* write header data */	 
  if( WriteChunk( pData->iff, data1, 8L ) != 8 )
    {
      result = -1;
      goto err;
    }	
  if( WriteChunk( pData->iff, indxs, 4L ) != 4 )
    {
      result = -1;
      goto err;
    }	

  /* if the geometry is not a null primitive then write the rest of the data */
  if ( pSrcHeader->primType != GEO_Null )
    {
      geom = ( Geometry * ) ( &( geo_prim->geoData ) );
	
      /* get the vertex flags */
      if( geom->Style & GEO_Colors ) 
	flags |= ( SDFB_Geo_UseColors | SDFB_Geo_HasColors );
      if( geom->Style & GEO_Normals ) 
	flags |= ( SDFB_Geo_UseNormals | SDFB_Geo_HasNormals );
      if( geom->Style & GEO_TexCoords ) 
	flags |= ( SDFB_Geo_UseTexCoords | SDFB_Geo_HasTexCoords );
			
      data2[0] = geom->Size;
      data2[1] = flags;
	
      if( WriteChunk( pData->iff, data2, 8L ) != 8 )
	{
	  result = -1;
	  goto err;
	}	
		
      /* write position data */
      chunk_size = geom->Size * sizeof( Point3 );
      if( WriteChunk( pData->iff, geom->Locations, chunk_size ) != chunk_size )
	{
	  result = -1;
	  goto err;
	}	
	
      /* write normals data */
      if( geom->Style & GEO_Normals )
	{
	  chunk_size = geom->Size * sizeof( Vector3 );
	  if( WriteChunk( pData->iff, geom->Normals, chunk_size ) != chunk_size )
	    {
	      result = -1;
	      goto err;
	    }	
	}
		
      /* write colors data */
      if( geom->Style & GEO_Colors )
	{
	  chunk_size = geom->Size * sizeof( Color4 );
	  if( WriteChunk( pData->iff, geom->Colors, chunk_size ) != chunk_size )
	    {
	      result = -1;
	      goto err;
	    }	
	}
		
      /* write texture coordinate data */
      if( geom->Style & GEO_TexCoords )
	{
	  chunk_size = geom->Size * sizeof( TexCoord );
	  if( WriteChunk( pData->iff, geom->TexCoords, chunk_size ) != chunk_size )
	    {
	      result = -1;
	      goto err;
	    }	
	}
    }
	
  result = PopChunk( pData->iff );
  if( result < 0 ) goto err;

err:    
  return result;
}

/*
**	Write SATM data
*/
static Err
SATM_WriteChunk(
		SDFBObject* pData, 
		PrimHeader* pSrcHeader,
		uint32 surfID
		)
{
  uint32 primSize;
  uint32 chunk[2];
  BlockHeader *blockHeader;
  Err result = GFX_OK;

  printf("SATM_WriteChunk\n");
  return GFX_OK;
  /* start writing the chunk data */
  primSize = pSrcHeader->primSize;
  /* subtract the PrimHeader  */
  primSize -= sizeof( PrimHeader );
	
  /* compiled TriMesh prim header */
  blockHeader = (BlockHeader*)(pSrcHeader+1);
	
  result = PushChunk( pData->iff, 0L, ID_SATM, primSize + 8 );
  if( result < 0 ) goto err;

  /* write header data */
  chunk[0] = 1;	 
  chunk[1] = surfID;
  if( WriteChunk( pData->iff, chunk, 8L ) != 8 )
    {
      result = -1;
      goto err;
    }	
	
  /* write primitive data */
  if( WriteChunk( pData->iff, blockHeader, primSize ) != primSize )
    {
      result = -1;
      goto err;
    }	
  result = PopChunk( pData->iff );
  if( result < 0 ) goto err;

err:    
  return result;
}
	
/*
** Write all CHAR chunks from the SDF dictionary
*/
static Err
SHDR_WriteReferences(
		     SDFBObject	*sdfb
		     )
{
  uint32 result;
  CharData *group;
  int32 i;
  KfEngine *kf;
		
  /* write user template chunks for user classes */

  /* 
   * Group is the top node. We can get the following information from here
   * 1. Hierarchy information
   * 2. Character/Model chunk data
   * 3. Surface data
   * 4. Material array
   * 5. Texture array
   * 6. Material index
   * 7. Texture index
   */
	
  giff = sdfb->iff;

  if (GetAnimFlag() & GEOM_FLAG)
    {
      Merc_WriteMata(sdfb);
      for (i = 0; i < Glib_GetRootCharCount(); i++)
	{
	  /* collapse hierarchy */
	  if ((GetAnimFlag() & GEOM_FLAG) && GetCollapseFlag())
	    Merc_HierCollapse( sdfb, (CharData *)Glib_GetRootChar(i) );
	  Merc_WritePods(sdfb, (CharData *)Glib_GetRootChar(i));
	}
	
      if (Glib_GetRootCharCount() <= 0)
	{
	  for (i = 0; i < Glib_GetModelCount(); i++)
	    {
	      group = (CharData *)Glib_GetModel(i);
	      Merc_WritePods(sdfb, group);
	    }
	}
    }
  for (i = 0; i < Glib_GetKfEngCount(); i++)
    {
      kf = Glib_GetKfEng(i);
      ANIM_WriteChunk(sdfb, (GfxObj *)kf);
    }
  if (GetAnimFlag() & GEOM_FLAG)
    {
      for (i = 0; i < Glib_GetRootCharCount(); i++)
	{
	  group = Glib_GetRootChar(i);
	  Merc_WriteCharAll(sdfb, group);
	}
      if (Glib_GetRootCharCount() == 0)
	{
	  for (i = 0; i < Glib_GetModelCount(); i++)
	    {
	      group = (CharData *)Glib_GetModel(i);
	      Merc_WriteCharAll(sdfb, group);
	    }
	}
    }
	
err:
  return ( result );
}

/*
** Write binary SDF file. This is  same as quad-byte aligned IFF format.
** Field 'options' is used to write compiled surface formats into the file.
*/
Err
SDFB_WriteIFF(
	      char *fileName,
	      uint32 options
	      )
{
  Err result = GFX_OK;
  SDFBObject	sdfbo;
  IFFParser	*iff = NULL;
  TagArg		tags[2];
  int32		i;
  int32		numhier;
  MhdrChunk	mh;
  CharData *group;

  /*
   * Interface between the framework dictionary and geometry 
   */

  /* create the BSDF file and IFF parser */
  tags[0].ta_Tag = IFF_TAG_FILE;
  tags[0].ta_Arg = fileName;
  tags[1].ta_Tag = TAG_END;      
  result = CreateIFFParser( &iff, TRUE, tags );
  if( result < 0 ) goto err;

  /* initialize the parser */
  result = SDFB_InitWrite( iff, &sdfbo, options );
  if( result < 0 ) goto err;

  result = SDFB_GetChunkOffset( sdfbo.iff, &sdfbo.hdrChunkOffset	);

  /* write out sdfh chunk */
  mh.chunkid = ID_MHDR;
  mh.size = sizeof(MhdrChunk) - 8;
  mh.objid = 0;
  mh.minid = 1;
  mh.maxid = 1;
  mh.numAM = 0;
  mh.numTexpage = 0;
  mh.numPod = 0;
  result = PushChunk( sdfbo.iff, 0L, ID_MHDR, IFF_SIZE_UNKNOWN_32 );
  if( result < 0 ) goto err;
  if( PutInt32s( sdfbo.iff, (int32 *)&mh.objid, 6, gEndian ) == -1 )
    {
      result = -1;
      goto err;
    }	
  result = PopChunk( sdfbo.iff );
  if( result < 0 ) goto err;
	
  /* write all the refernced object data here */
  result = SHDR_WriteReferences( &sdfbo );

  for (i = 0; i < Glib_GetRootCharCount(); i++)
    {
      group = Glib_GetRootChar(i);
      Merc_WriteHier(&sdfbo, group);
    }
  if (Glib_GetRootCharCount() == 0)
    {
      for (i = 0; i < Glib_GetModelCount(); i++)
	{
	  group = (CharData *)Glib_GetModel(i);
	  Merc_WriteHier(&sdfbo, group);
	}
    }

#if 0
  numhier = Glib_GetRootCharCount();

  for (i = 0; i < numhier; i++)
    {
      /* collapse hierarchy */
      if ((GetAnimFlag() & GEOM_FLAG) && GetCollapseFlag())
	Merc_HierCollapse( &sdfbo, (Character *)Glib_GetRootChar(i) );
		
      /* write the hierarchy */
      Merc_WriteHier(&sdfbo, (CharData *)Glib_GetRootChar(i));
    }
#endif

  if( result < 0 ) goto err;
	
  mh.maxid = mercid - 1;
  mh.numAM = Glib_GetRootCharCount();
  if (Glib_GetRootCharCount() == 0)
    mh.numAM = Glib_GetModelCount();
  if (GetAnimFlag() & GEOM_FLAG)
    {
      mh.numTexpage = GetUsedTexCount();
      mh.numTexpage = mh.numTexpage + Glib_GetMarkedTexBlend();
      mh.numPod = numPod;
    }
  /* end writing */
  if (GetFrameworkFlag())
    OBJN_WriteChunk( &sdfbo );
  result = SDFB_EndWrite( &sdfbo , &mh);
  if( result < 0 ) goto err;
	
err:
  if( result < 0 ) return ( result ); 
#if 0
  fprintf( stderr, "Error Number = %d\r", result );
#endif
	
die:
  if( iff ) result = DeleteIFFParser(iff);
  
  return ( result );
}

