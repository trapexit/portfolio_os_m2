
#define SDFMAT_DIFFUSE_MASK 0x0001
#define SDFMAT_AMBIENT_MASK  0x0002
#define SDFMAT_SPECULAR_MASK 0x0004
#define SDFMAT_EMISSION_MASK 0x0008
#define SDFMAT_SHINE_MASK 0x0010
#define SDFMAT_F2SIDED_MASK 0x0020
#define SDFMAT_ENABLE_MASK 0x0040


#define MAT_BUF_SIZE 20

typedef struct
{
  float  diffuse[4];
  float  ambient[4];
  float  specular[4];
  float  emission[4];
  float  shine;
  uint16 ValSet;
  char  *OrigName;
} SDFMat;

void SDFMat_Init(SDFMat *mat);
bool SDFMat_Compare(SDFMat *mat1, SDFMat *mat2);
void SDFMat_Copy(SDFMat *mat1, SDFMat *mat2);
M2Err LWSURF_ToSDFMat(LWSURF surf, SDFMat *mat, char *matName);
void SDFMat_Print(SDFMat *mat, FILE *fPtr, int tab);
M2Err Material_ReadFile(char *fileIn, SDFMat **myMat, int *numMat, int *curMat);
M2Err SDFMat_Add(SDFMat *mat, SDFMat **Materials, int *NMats, int *CurMat, 
		 bool collapse, int *MatIndex);
M2Err LWSURF_MakeWhiteMat(LWSURF surf, float intensity, SDFMat *mat);


