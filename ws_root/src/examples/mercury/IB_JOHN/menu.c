#include "mpeg.h"
#include "fight.h"

#define TOP_ROW      40
#define RIGHT_COLUMN 512
#define BOTTOM_ROW   373
#define LEFT_COLUMN  50
#define DELTA_X      94
#define DELTA_Y      67


typedef struct {
  BlitObject *cursor;
  float      selX, selY;
  BlitObject *profile;
  float      profX, profY;
  int        character;
  long       ievent;
} PlayerSelect;

uint16  MenuPos[32] =
{
  LEFT_COLUMN+2*DELTA_X,
  BOTTOM_ROW,
  LEFT_COLUMN+DELTA_X,
  BOTTOM_ROW,
  LEFT_COLUMN,
  BOTTOM_ROW-DELTA_Y,
  LEFT_COLUMN,
  BOTTOM_ROW-2*DELTA_Y,
  LEFT_COLUMN,
  BOTTOM_ROW-3*DELTA_Y,
  LEFT_COLUMN,
  BOTTOM_ROW-4*DELTA_Y,
  LEFT_COLUMN+DELTA_X,
  TOP_ROW,
  LEFT_COLUMN+2*DELTA_X,
  TOP_ROW,
  LEFT_COLUMN+3*DELTA_X,
  TOP_ROW,
  LEFT_COLUMN+4*DELTA_X,
  TOP_ROW,
  RIGHT_COLUMN,
  BOTTOM_ROW-4*DELTA_Y,
  RIGHT_COLUMN,
  BOTTOM_ROW-3*DELTA_Y,
  RIGHT_COLUMN,
  BOTTOM_ROW-2*DELTA_Y,
  RIGHT_COLUMN,
  BOTTOM_ROW-DELTA_Y,
  LEFT_COLUMN+4*DELTA_X,
  BOTTOM_ROW,
  LEFT_COLUMN+3*DELTA_X,
  BOTTOM_ROW
};

void Menu_InitBlits(BlitObject *bo, PlayerSelect *ps, int numPlayers, List *profiles)
{
  BlitterSnippetHeader *snippets[2];
  BlitObjectNode *node;
  int i;

  
  snippets[0] = &bo->bo_dbl->dbl_header;
  SetDBlendAttr (snippets, DBLA_BlendEnable, 1);
  SetDBlendAttr (snippets, DBLA_DiscardAlpha0, 1);

  bo->bo_dbl->dbl_userGenCntl |= (FV_DBUSERCONTROL_BLENDEN_MASK | FV_DBUSERCONTROL_SRCEN_MASK);
  bo->bo_dbl->dbl_txtMultCntl =
    CLA_DBAMULTCNTL(RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR,
		    RC_DBAMULTCNTL_AMULTCOEFSELECT_TEXALPHA,
		    0,
		    0);
  bo->bo_dbl->dbl_srcMultCntl =
    CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
		    RC_DBBMULTCNTL_BMULTCOEFSELECT_TEXALPHACOMPLEMENT,
		    0,
		    0);
  bo->bo_dbl->dbl_aluCntl =
    CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A_PLUS_BCLAMP, 0);
  
  snippets[0] = &bo->bo_tbl->txb_header;
  snippets[1] = &bo->bo_pip->ppl_header;
  
  SetTextureAttr (snippets, TXA_TextureEnable, 1);
  SetTextureAttr (snippets, TXA_ColorOut, TX_BlendOutSelectTex);
  SetTextureAttr (snippets, TXA_PipAlphaSelect, TX_PipSelectColorTable);
  SetTextureAttr (snippets, TXA_AlphaOut, TX_BlendOutSelectTex);
  
  for (i=0; i<numPlayers; i++)
    {
      ps[i].selX =(float)MenuPos[2*ps[i].character];
      ps[i].selY =(float)MenuPos[2*ps[i].character+1];
      ps[i].cursor = bo;   /* For now, assume everybody is using the same cursor */
      ps[i].profY = (float)150;
    }
  ps[0].profX = (float)175;
  ps[1].profX = (float)352;
  ScanList(profiles, node, BlitObjectNode)
    {
      /*
      Blt_ReuseSnippet(bo, node->bon_BlitObject, BLIT_TAG_DBLEND);
      Blt_ReuseSnippet(bo, node->bon_BlitObject, BLIT_TAG_TBLEND);
      */
    }
}


void Player_Select(PlayerSelect *ps)
{
  bool changed = FALSE;

  if (ps->ievent & ControlRight)
    {
      ps->character++;
      if (ps->character>15)
	ps->character=0;
    }
  else if (ps->ievent & ControlLeft)
    {
      ps->character--;
      if (ps->character<0)
	ps->character=15;
    }
}

void Menu_Draw(GraphicsEnv *genv, int numPlayers, uint32 *char0, uint32 *char1, OptionsRec *options)
{
  long i;
  float x[2], y[2];
  PlayerSelect *ps;
  PlayerSelect players[2];
  TagArg tags[2];
  char fileName[20] = "profiles.utf";

  Err err;
  BlitObject *selector;
  BlitObject *profile;
  BlitObjectNode *node;
  List       profiles;

  players[0].character = *char0;
  players[1].character = *char1;

  GS_SetView(genv->gs, genv->d->view );

  GeneralInit();

  err = Blt_LoadUTF(&selector, "RRing.utf");
  if (err<0)
    PrintfSysErr(err);
  tags[0].ta_Tag = LOADTEXTURE_TAG_FILENAME;
  tags[0].ta_Arg = fileName;
  tags[1].ta_Tag = TAG_END;
  
  err = Blt_LoadTexture(&profiles, tags);
  if (err<0)
    PrintfSysErr(err);
  printf("Successful read of profiles\n");

  Menu_InitBlits(selector, players, numPlayers, &profiles);

  MStill.stillID = MPS_LoadMPEGStill("menu.mpg", &MStill.data, &MStill.size);

  gScreenSelect = 0;
  
  while(InGameLoop == YES) {
    for(i = 0; i < numPlayers; i++) 
      {
	ps = &players[i];
	GetControlPad(i+1, FALSE, &PodControl[i]);
	newievent = PodControl[i].cped_ButtonBits;
	PodControl[i].cped_ButtonBits = 0;
	ps->ievent = newievent ^ LastBits[i];
	ps->ievent &= newievent;
	ps->ievent |= (newievent & ContinueBits[i]);
	LastBits[i] = newievent;

	if(ps->ievent & ControlX) {
	  InGameLoop = NO;
	  *char0 = players[0].character;
	  *char1 = players[1].character;
	  return;
	}
	else
	  Player_Select(&players[i]);
      }

    
    GS_BeginFrame(genv->gs);

    /* CLT_ClearFrameBuffer(gs, 0.0, 0.0, 0.0, 0., TRUE, FALSE); */
    MPS_DrawMPEGStill(genv->gs, genv->bitmaps[gScreenSelect], MStill.data, MStill.size);
    
    for (i=0; i<numPlayers; i++)
      {
	ps = &players[i];
	ps->selX=(float)MenuPos[2*ps->character];
	ps->selY=(float)MenuPos[2*ps->character+1];
	Blt_MoveVertices(ps->cursor->bo_vertices, ps->selX, ps->selY);
	err = Blt_BlitObjectToBitmap(genv->gs, selector, genv->bitmaps[gScreenSelect], 0);
	if (err<0)
	  PrintfSysErr(err);
	Blt_MoveVertices(ps->cursor->bo_vertices,-ps->selX,-ps->selY);      
      }
    
    node = (BlitObjectNode *)FirstNode(&profiles);
    for (i=0; i<players[0].character; i++, node=(BlitObjectNode *)NextNode(node))
      ;
    profile = node->bon_BlitObject;
    Blt_MoveVertices(profile->bo_vertices, players[0].profX, players[0].profY);
    err = Blt_BlitObjectToBitmap(genv->gs, profile, genv->bitmaps[gScreenSelect], 0);
    if (err<0)
      PrintfSysErr(err);
    Blt_MoveVertices(profile->bo_vertices, -players[0].profX, -players[0].profY);
   
    node = (BlitObjectNode *)FirstNode(&profiles);
    for (i=0; i<players[1].character; i++, node=(BlitObjectNode *)NextNode(node))
      ;
    profile = node->bon_BlitObject;
    Blt_MoveVertices(profile->bo_vertices, players[1].profX, players[1].profY);
    err = Blt_BlitObjectToBitmap(genv->gs, profile, genv->bitmaps[gScreenSelect], 0);
    if (err<0)
      PrintfSysErr(err);
    Blt_MoveVertices(profile->bo_vertices, -players[1].profX, -players[1].profY);
   

    CalcFrameTime();

    pen.pen_X       = 500.;
    pen.pen_Y       = 425.;
    sprintf(txtBuffer, "%2.1f", (1. / _frametime));
    DrawString(genv->gs, font, &pen, txtBuffer, strlen(txtBuffer));

    GS_SendList(genv->gs);
    GS_EndFrame(genv->gs);
    
    gScreenSelect = gScreenSelect++;
    if (gScreenSelect >= genv->d->numScreens)
      gScreenSelect = 0;

    GS_SetDestBuffer(genv->gs, genv->bitmaps[gScreenSelect]);
  }
  
  MPS_FreeMPEGStill(MStill.data);
  MPS_TeardownMPEGStill();
}

void WaitForAnyButton(ControlPadEventData *cped)
{
  GetControlPad(1, TRUE, cped); /* Button down */
  GetControlPad(1, TRUE, cped); /* Button up */
}


void GeneralInit(void)
{
  gfloat i,ii;
  long t;
  Err err;
  
  InitEventUtility (2, 0, LC_ISFOCUSED);
  ContinueBits[0] = 0;
  
  OpenFontFolio();
  GameFontName[0] = "default_14";
  font = OpenFont(GameFontName[0]);
  if (font < 0)
    printf("ERROR in loading font\n");
  memset(&pen, 0, sizeof(PenInfo));
  pen.pen_FgColor = 0x00888800;
  pen.pen_BgColor = 0;
  pen.pen_XScale = pen.pen_YScale = 1.0;
  
  MPS_InitMPEGStill();
  
  /* Init the timer values */
  SampleSystemTimeTT(&curTT);  /* will be used for elapsedTime measuring */
}

void CalcFrameTime(void)
{
  prevTT = curTT;
  SampleSystemTimeTT(&curTT);
  SubTimerTicks(&prevTT,&curTT,&resultTT);
  ConvertTimerTicksToTimeVal(&resultTT,&tv);
  _frametime = tv.tv_sec + tv.tv_usec * .000001;
}

/*
 * This function initializes the MPEG device to display a 640x480 at
 * 16 bit image.  It also creates the necessary IO Items needed to
 * communicate with the MPEG device.  This function must be called
 * before attempting to draw an image with MPS_DrawMPEGStill.
 */
Err MPS_InitMPEGStill(void)
{
  int32				res;
  List				*list;
  Item				vcmdItem;
  IOInfo				vcmdInfo;
  CODECDeviceStatus	stat;
  
  /*
   * Check to see if we're already initialized.
   */	
  if (mpegDevItem >= 0)
    return 0;
  
  /*
   * Open the MPEG device
   */	
  res = CreateDeviceStackListVA(&list, "cmds", DDF_EQ, DDF_INT, 1,
				MPEGVIDEOCMD_CONTROL,	NULL);
  CHECK_RES("CreateDeviceStackListVA", res);
  
  mpegDevItem = OpenDeviceStack((DeviceStack *) FirstNode(list));
  DeleteDeviceStackList(list);
  CHECK_RES("OpenDeviceStack", mpegDevItem);
  
  /*
   * Configure the MPEG device
   */
  vcmdItem = CreateIOReq(0, 0, mpegDevItem, 0);
  CHECK_RES("CreateIOReq", vcmdItem);

  bzero(&vcmdInfo, sizeof(IOInfo));
  bzero(&stat, sizeof(CODECDeviceStatus));
  
  stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_STANDARD;
  stat.codec_TagArg[0].ta_Arg = (void *) kCODEC_SQUARE_RESAMPLE;
  stat.codec_TagArg[1].ta_Tag = VID_CODEC_TAG_DEPTH;
  if (IB_SCREENDEPTH == 32)
    stat.codec_TagArg[1].ta_Arg = (void *) 24L;
  else
    stat.codec_TagArg[1].ta_Arg = (void *) IB_SCREENDEPTH;
  stat.codec_TagArg[2].ta_Tag = VID_CODEC_TAG_HSIZE;
  stat.codec_TagArg[2].ta_Arg = (void *) IB_SCREENWIDTH;
  stat.codec_TagArg[3].ta_Tag = VID_CODEC_TAG_VSIZE;
  stat.codec_TagArg[3].ta_Arg = (void *) IB_SCREENHEIGHT;
  stat.codec_TagArg[4].ta_Tag = VID_CODEC_TAG_M2MODE;
  stat.codec_TagArg[5].ta_Tag = VID_CODEC_TAG_KEYFRAMES;
  stat.codec_TagArg[6].ta_Tag = TAG_END;
  
  vcmdInfo.ioi_Command = MPEGVIDEOCMD_CONTROL;
  vcmdInfo.ioi_Send.iob_Buffer = &stat;
  vcmdInfo.ioi_Send.iob_Len = sizeof(CODECDeviceStatus);
  

  res = DoIO(vcmdItem, &vcmdInfo);
  DeleteIOReq(vcmdItem);
  CHECK_RES("DoIO", res);
  
  /*
   * Create the needed IO request items.
   */
  vreadItem = CreateIOReq(0, 0, mpegDevItem, 0);
  CHECK_RES("CreateIOReq", vreadItem);
  
  vwriteItem1 = CreateIOReq(0, 0, mpegDevItem, 0);
  CHECK_RES("CreateIOReq", vwriteItem1);
  
  vwriteItem2 = CreateIOReq(0, 0, mpegDevItem, 0);
  CHECK_RES("CreateIOReq", vwriteItem2);
  
  vwriteItem3 = CreateIOReq(0, 0, mpegDevItem, 0);
  CHECK_RES("CreateIOReq", vwriteItem3);
  
  eosItem = CreateIOReq(0, 0, mpegDevItem, 0);
  CHECK_RES("CreateIOReq", eosItem);
  
  return 0;
}

/*
 * Draws a 640x480 at 16 bit MPEG still to the active frame buffer.  The
 * caveat to this function is that it's synchronous and takes 0.02
 * seconds to operate.
 *
 * The parameters to this function are a pointer to the program's GState,
 * a pointer to the start of the MPEG still data, and the size of the
 * MPEG still.
 */
Err MPS_DrawMPEGStill(GState *gs, Item bmi, char *data, int32 size)
{
  Err				res;
  IOInfo			vreadInfo;
  IOInfo			vwriteInfo1, vwriteInfo2, vwriteInfo3;
  IOInfo			eosInfo;
  FMVIOReqOptions	eosOptions;
  
  GS_WaitIO(gs);
  
  /*
   * Initialize IO request for reading decompressed data
   */	
  bzero(&vreadInfo, sizeof(IOInfo));
  vreadInfo.ioi_Command = MPEGVIDEOCMD_READ;
  vreadInfo.ioi_Send.iob_Buffer = (void *) &bmi;
  vreadInfo.ioi_Send.iob_Len = sizeof(Item);
  
  /*
   * Initialize IO request for sending compressed data.  There is
   * currently a bug in the MPEG device driver which will not accept
   * buffers larger then (64K - 1).  This code works around the
   * problem for files smaller then 189K.  It's not pretty but works.
   */
  if (size > 193536)
    {
      printf("MPEG Image too large for 64K workaround.\n");
      return (-1);
    }	
  else if (size > 129024)
    {
      bzero(&vwriteInfo1, sizeof(IOInfo));
      vwriteInfo1.ioi_Command = MPEGVIDEOCMD_WRITE;
      vwriteInfo1.ioi_Send.iob_Buffer = data;
      vwriteInfo1.ioi_Send.iob_Len = 64512;
      
      bzero(&vwriteInfo2, sizeof(IOInfo));
      vwriteInfo2.ioi_Command = MPEGVIDEOCMD_WRITE;
      vwriteInfo2.ioi_Send.iob_Buffer = (void *) (data + 64512);
      vwriteInfo2.ioi_Send.iob_Len = 64512;
      
      bzero(&vwriteInfo3, sizeof(IOInfo));
      vwriteInfo3.ioi_Command = MPEGVIDEOCMD_WRITE;
      vwriteInfo3.ioi_Send.iob_Buffer = (void *) (data + 129024);
      vwriteInfo3.ioi_Send.iob_Len = size - 129024;
    }
  else if (size > 64512)
	{
	  bzero(&vwriteInfo1, sizeof(IOInfo));
	  vwriteInfo1.ioi_Command = MPEGVIDEOCMD_WRITE;
	  vwriteInfo1.ioi_Send.iob_Buffer = data;
	  vwriteInfo1.ioi_Send.iob_Len = 64512;
	  
	  bzero(&vwriteInfo2, sizeof(IOInfo));
	  vwriteInfo2.ioi_Command = MPEGVIDEOCMD_WRITE;
	  vwriteInfo2.ioi_Send.iob_Buffer = (void *) (data + 64512);
	  vwriteInfo2.ioi_Send.iob_Len = size - 64512;
	}
  else
    {
      bzero(&vwriteInfo1, sizeof(IOInfo));
      vwriteInfo1.ioi_Command = MPEGVIDEOCMD_WRITE;
      vwriteInfo1.ioi_Send.iob_Buffer = data;
      vwriteInfo1.ioi_Send.iob_Len = size;
    }
  
  /*
   * Queue up the read and write requests.
   */	
  res = SendIO(vreadItem, &vreadInfo);
  CHECK_RES("SendIO", res);
  
  if (size > 129024)
    {
      res = SendIO(vwriteItem1, &vwriteInfo1);
      CHECK_RES("SendIO", res);
      
      res = SendIO(vwriteItem2, &vwriteInfo2);
      CHECK_RES("SendIO", res);
      
      res = SendIO(vwriteItem3, &vwriteInfo3);
      CHECK_RES("SendIO", res);
    }
  else if (size > 64512)
    {
      res = SendIO(vwriteItem1, &vwriteInfo1);
      CHECK_RES("SendIO", res);
      
      res = SendIO(vwriteItem2, &vwriteInfo2);
      CHECK_RES("SendIO", res);
    }
  else
    {
      res = SendIO(vwriteItem1, &vwriteInfo1);
      CHECK_RES("SendIO", res);
    }
  
  /*
   * Inform the MPEG device that it's received a complete picture.
   */	
  bzero(&eosOptions, sizeof(FMVIOReqOptions));
  bzero(&eosInfo, sizeof(IOInfo));
  
  eosInfo.ioi_Command = MPEGVIDEOCMD_WRITE;
  eosInfo.ioi_Send.iob_Buffer = 0;
  eosInfo.ioi_Send.iob_Len = 0;
  eosOptions.FMVOpt_Flags = FMV_END_OF_STREAM_FLAG;
  eosInfo.ioi_CmdOptions = (int32) &eosOptions;
  
  res = SendIO(eosItem, &eosInfo);
  CHECK_RES("SendIO", res);

  
  res = WaitIO(vreadItem);
  CHECK_RES("WaitIO", res);
  
  return 0;
}

/*
 * Cleans up all resources allocated and initialized by the function
 * MPS_InitMPEGStill().
 */
Err MPS_TeardownMPEGStill(void)
{
  DeleteIOReq(eosItem);
  eosItem = -1;
  
  DeleteIOReq(vwriteItem1);
  vwriteItem1 = -1;
  
  DeleteIOReq(vwriteItem2);
  vwriteItem2 = -1;
  
  DeleteIOReq(vwriteItem3);
  vwriteItem3 = -1;
  
  DeleteIOReq(vreadItem);
  vreadItem = -1;
  
  CloseDeviceStack(mpegDevItem);
  mpegDevItem = -1;
  
  return 0;
}

/*
 * This function will load a single MPEG still into memory from a file.
 * If the value in data points to NULL, memory will be allocated by this
 * function.  Otherwise, a user provided memory space can be passed in.
 * The size of the file is returned in size.
 */
Err MPS_LoadMPEGStill(char *fname, void **data, int32 *size)
{
  RawFile			*file;
  FileInfo		fileInfo;
  Err				res;
  static int32	stillID = 0;
  
  res = OpenRawFile(&file, fname, FILEOPEN_READ);
  CHECK_RES("OpenRawFile", res);
  
  res = GetRawFileInfo(file, &fileInfo, sizeof(FileInfo));
  CHECK_RES("GetRawFileInfo", res);
  
  if (*data == NULL)
    *data = AllocMem(fileInfo.fi_ByteCount, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
  
  res = ReadRawFile(file, *data, fileInfo.fi_ByteCount);
  CHECK_RES("ReadRawFile", res);
  
  *size = res;
  
  res = CloseRawFile(file);
  CHECK_RES("CloseRawFile", res);
  
  stillID++;
  
  return(stillID);
}

void MPS_FreeMPEGStill(void *data)
{
  if (data)
    FreeMem(data, TRACKED_SIZE);
  
  return;
}

