
#define TAB_SIZE 32
#define DAB_SIZE 64
typedef struct
{
  char *FileName;
  char *OrigName;
  uint32 TABSet;
  uint32 TAB[TAB_SIZE];
  uint32 DABSetHi;
  uint32 DABSetLo;
  uint32 DAB[DAB_SIZE];
  long XClampPos, YClampPos;
  int  XTile,  YTile;
  int  PageIndex[2];
  int  EnvMap;
} SDFTex;

void SDFTex_Init(SDFTex *tb);
void SDFTex_Free(SDFTex *tb);
bool SDFTex_Compare(SDFTex *tb1, SDFTex *tb2);
void SDFTex_Copy(SDFTex *tb1, SDFTex *tb2);
void SDFTex_SetTAB(SDFTex *tb, uint32 command, uint32 value);
bool SDFTex_GetTAB(SDFTex *tb, uint32 command, uint32 *value);
void SDFTex_SetDAB(SDFTex *tb, uint32 command, uint32 value);
bool SDFTex_GetDAB(SDFTex *tb, uint32 command, uint32 *value);
void SDFTex_Print(SDFTex *tex, FILE *fPtr, int tab);
M2Err Texture_ReadFile(char *fileIn, SDFTex **myTex, int *numTex, int *cTex);


