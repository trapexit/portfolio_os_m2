#define CLT_BUF_SIZE   64
#define TEX_BUF_SIZE  64
#define TRAM_SIZE     16384
#define PAGE_BUF_SIZE 64

typedef enum
{
  TXA_Count= 0,
  TXA_AddrCntl,
  TXA_TexType,
  TXA_PIPCntl,
  TXA_TABCntl,
  TXA_PIPConstSSB0,
  TXA_PIPConstSSB1,
  TXA_TABConst0,
  TXA_TABConst1,
  TXA_NoMoreRegs
} Pg_TABReg;

typedef enum
{
  DBLA_UsrCntl= 0,
  DBLA_DiscardCntl,
  DBLA_XWinClip,
  DBLA_YWinClip,
  DBLA_ZCntl,
  DBLA_ZOffset,
  DBLA_SSBDSBCntl,
  DBLA_ConstIn,
  DBLA_AMultCntl,
  DBLA_AMConstSSB0,
  DBLA_AMConstSSB1,
  DBLA_BMultCntl,
  DBLA_BMConstSSB0,
  DBLA_BMConstSSB1,
  DBLA_ALUCntl,
  DBLA_SrcAlphaCntl,
  DBLA_DestAlphaCntl,
  DBLA_DestAlphaConst,
  DBLA_DitherMatA,
  DBLA_DitherMatB,
  DBLA_NoMoreRegs
} Pg_DABReg;


#define TAB_REG_SIZE 9
#define DAB_REG_SIZE 21

typedef struct tag_TxPage
{
  M2TX         *Tex;
  M2TXPgHeader PgHeader;
  uint32       TABRegSet;
  uint32       TABReg[TAB_REG_SIZE];
  uint32       DABRegSet;
  uint32       DABReg[DAB_REG_SIZE];
  uint32       PatchOffset[4*TEX_BUF_SIZE];  /* 4 LODs Max for each texture in a page */
  float        UVScale[2*TEX_BUF_SIZE];      /* U and V for each texture in a page */
  CltSnippet   CLT;
} TxPage;

typedef struct tag_PageNames
{
  char      *PageName;
  char      **SubNames;
  uint32    NumNames;
  uint32    Allocated;
} PageNames;

