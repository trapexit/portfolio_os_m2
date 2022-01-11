

struct Color4 { gfloat r, g, b, a; };
struct Vector3 { gfloat x, y, z; };
typedef struct Vector3 Point3;
typedef struct Color4 Color4;

#define Col_SetRGB(c, R, G, B)                          \
                   ((c)->r = ((gfloat)R), (c)->g = ((gfloat)G),    \
		   (c)->b = ((gfloat)B), (c)->a = 1.0)
#define Col_Set(c, R, G, B, A) \
               ((c)->r = ((gfloat)R), (c)->g = ((gfloat)G), \
	       (c)->b = ((gfloat)B), (c)->a = ((gfloat)A))

typedef struct
{
  uint16 ValSet;
  uint16 TFLG;
  uint16 TVAL;
  gfloat TAMP;
  uint16 TFRQ;
  Point3 TSIZ;
  Point3 TCTR;
  Point3 TFAL;
  Point3 TVEL;
  char *Name;
  char *TIMG;
  gfloat TSP0, TSP1, TSP2;
  gfloat TFP0, TFP1;
} LWTex;


#define LWTex_TFLGMask 0x0001
#define LWTex_TSIZMask 0x0002
#define LWTex_TCTRMask 0x0004
#define LWTex_TVALMask 0x0008
#define LWTex_TFALMask 0x0010
#define LWTex_TVELMask 0x0020
#define LWTex_TAMPMask 0x0040
#define LWTex_TFRQMask 0x0080
#define LWTex_TSP0Mask 0x0100
#define LWTex_TSP1Mask 0x0200
#define LWTex_TSP2Mask 0x0400
#define LWTex_TFP0Mask 0x0800
#define LWTex_TFP1Mask 0x1000

#define LWTex_FXAxis 0x0001
#define LWTex_FYAxis 0x0002
#define LWTex_FZAxis 0x0004
#define LWTex_FWorld 0x0008
#define LWTex_FNegative 0x0010
#define LWTex_FBlend 0x0020
#define LWTex_FAnti 0x0040



typedef struct
{
  uint32 COLR;
  uint16 LUMI, DIFF, SPEC, REFL, GLOS, TRAN;
  uint16 FLAG;
  uint16 ValSet;
  char *RIMG;
  gfloat SMAN, RSAN, EDGE;
  LWTex *CTEX;
  LWTex *DTEX;
  LWTex *STEX;
  LWTex *RTEX;
  LWTex *TTEX;
  LWTex *BTEX;
  LWTex *UsedTEX;
  int16 TexIndex;
  int16 MatIndex;
  float MinU, MinV, MaxU, MaxV;
  bool  XTile,  YTile;
  bool  EnvMap;
} LWSURF;

#define LWSURF_COLRMask 0x0001
#define LWSURF_LUMIMask 0x0002
#define LWSURF_DIFFMask 0x0004
#define LWSURF_SPECMask 0x0008
#define LWSURF_REFLMask 0x0010
#define LWSURF_GLOSMask 0x0020
#define LWSURF_TRANMask 0x0040
#define LWSURF_FLAGMask 0x0080
#define LWSURF_SMANMask 0x0100

#define LWSURF_FLuminous 0x0001
#define LWSURF_FOutline 0x0002
#define LWSURF_FSmooth 0x0004
#define LWSURF_FColHilite 0x0008
#define LWSURF_FColFilter 0x0010
#define LWSURF_FTransEdge 0x0020
#define LWSURF_FSharp 0x0040
#define LWSURF_F2Sided 0x0080
#define LWSURF_FAdditive 0x0100

void LWSURF_Init(LWSURF *surf);

bool LWSURF_GetCOLR(LWSURF surf, uint32 *color);
bool LWSURF_GetRed(LWSURF surf, uint8 *red);
bool LWSURF_GetGreen(LWSURF surf, uint8 *green);
bool LWSURF_GetBlue(LWSURF surf, uint8 *blue);
bool LWSURF_GetLUMI(LWSURF surf, uint16 *lumi);
bool LWSURF_GetDIFF(LWSURF surf, uint16 *diff);
bool LWSURF_GetSPEC(LWSURF surf, uint16 *spec);
bool LWSURF_GetREFL(LWSURF surf, uint16 *refl);
bool LWSURF_GetGLOS(LWSURF surf, uint16 *glos);
bool LWSURF_GetTRAN(LWSURF surf, uint16 *tran);
bool LWSURF_GetSMAN(LWSURF surf, gfloat *data);
bool LWSURF_GetFLAG(LWSURF surf, uint16 *flag);
bool LWSURF_GetFLuminous(LWSURF surf, bool *flag);
bool LWSURF_GetF2Sided(LWSURF surf, bool *flag);
bool LWSURF_GetFColHilite(LWSURF surf, bool *flag);
bool LWSURF_GetFSmooth(LWSURF surf, bool *flag);
bool LWTex_GetFXAxis(LWTex tex, bool *flag);
bool LWTex_GetFYAxis(LWTex tex, bool *flag);
bool LWTex_GetFZAxis(LWTex tex, bool *flag);
bool LWTex_GetFWorld(LWTex tex, bool *flag);
bool LWTex_GetTSIZ(LWTex tex, Point3 *size);
bool LWTex_GetTSP0(LWTex tex, gfloat *data);
bool LWTex_GetTSP1(LWTex tex, gfloat *data);
bool LWTex_GetTFP0(LWTex tex, gfloat *data);
bool LWTex_GetTFP1(LWTex tex, gfloat *data);
bool LWTex_GetTFLG(LWTex tex, uint16 *data);
bool LWTex_GetTCTR(LWTex tex, Point3 *center);
bool LWTex_GetTIMG(LWTex tex, char **timg);
bool LWTex_GetFNegative(LWTex tex, bool *flag);
bool LWTex_GetFBlend(LWTex tex, bool *flag);
bool LWTex_GetFAnti(LWTex tex, bool *flag);
void LWTex_Init(LWTex *tex);



void LWTex_SetTIMG(LWTex *tex, int16 nameLen, char *nameBuf);
void LWTex_SetTSIZ(LWTex *tex, Point3 size);
void LWTex_SetTCTR(LWTex *tex, Point3 size);
void LWTex_SetTVEL(LWTex *tex, Point3 size);
void LWTex_SetTFAL(LWTex *tex, Point3 size);
void LWTex_SetTAMP(LWTex *tex, gfloat size);
void LWSURF_SetCOLR(LWSURF *surf, uint8 color[4]);
void LWSURF_SetRed(LWSURF *surf, uint8 red);
void LWSURF_SetGreen(LWSURF *surf, uint8 green);
void LWSURF_SetBlue(LWSURF *surf, uint8 blue);
void LWSURF_SetLUMI(LWSURF *surf, uint16 lumi);
void LWSURF_SetDIFF(LWSURF *surf, uint16 diff);
void LWSURF_SetSPEC(LWSURF *surf, uint16 spec);
void LWSURF_SetREFL(LWSURF *surf, uint16 refl);
void LWSURF_SetGLOS(LWSURF *surf, uint16 glos);
void LWSURF_SetSMAN(LWSURF *surf, gfloat param);
void LWSURF_SetTRAN(LWSURF *surf, uint16 tran);
void LWSURF_SetFLAG(LWSURF *surf, uint16 flag);
void LWSURF_SetRIMG(LWSURF *surf, int16 nameLen, char *nameBuf);


void LWTex_SetTFP0(LWTex *tex, gfloat param);
void LWTex_SetTFP1(LWTex *tex, gfloat param);
void LWTex_SetTSP0(LWTex *tex, gfloat param);
void LWTex_SetTSP1(LWTex *tex, gfloat param);
void LWTex_SetTSP2(LWTex *tex, gfloat param);
void LWTex_SetTVAL(LWTex *tex, uint16 value);
void LWTex_SetTFLG(LWTex *tex, uint16 flags);
void LWTex_SetTFRQ(LWTex *tex, uint16 frequency);
void LWSURF_SetCTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex);
void LWSURF_SetDTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex);
void LWSURF_SetSTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex);
void LWSURF_SetRTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex);
void LWSURF_SetTTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex);
void LWSURF_SetBTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex);





