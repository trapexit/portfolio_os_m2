

long M2TXHeader_ComputeLODSize(M2TXHeader *header, uint16 lod);
M2Err PageCLT_CreateRef(TxPage *subTxPage, TxPage *txPage, int index);
M2Err PageCLT_CreatePageLoad(TxPage *txPage, uint32 totalBytes);
M2Err PageCLT_CreatePIPLoad(CltSnippet *snip, TxPage *txPage, bool hasPIP);
void TxPage_TABInterpret(TxPage *pg, uint32 command, uint32 value);
void TxPage_DABInterpret(TxPage *pg, uint32 command, uint32 value);
void TxPage_Init(TxPage *pg);
M2Err PageNames_Init(PageNames *pn);
M2Err PageNames_Extend(PageNames *pn, uint32 size);
M2Err PageCLT_Init(CltSnippet *snip);
M2Err PageCLT_Extend(CltSnippet *snip, uint32 size);
M2Err TexPageArray_ReadFile(char *fileIn, SDFTex **myTex, int *numTex, int *cTex,
			    PageNames **myPages, int *numPages, int *cPage, 
			    bool *gotTPA);




