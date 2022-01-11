#include "mpeg.h"

void main(int32 argc, char **argv)
{
  long i;
  Err err;
  
  SetUpDisplay();
  GeneralInit();
  
  MStill.stillID = MPS_LoadMPEGStill("clean.mpg", &MStill.data, &MStill.size);
  
  gScreenSelect = 0;
  
  while(InGameLoop == YES) {
    for(i = 0; i < 1; i++) GetControlPad(i+1, FALSE, &PodControl[i]);
    newievent = PodControl[0].cped_ButtonBits;
    PodControl[0].cped_ButtonBits = 0;
    ievent = newievent ^ LastBits[0];
    ievent &= newievent;
    ievent |= (newievent & ContinueBits[0]);
    LastBits[0] = newievent;
    
    if(ievent & ControlX) {
      InGameLoop = NO;
      printf("Exiting ! !\n");
      exit(0);
    }
    
    GS_BeginFrame(gs);
    
    /* CLT_ClearFrameBuffer(gs, 0.0, 0.0, 0.0, 0., TRUE, FALSE); */
    MPS_DrawMPEGStill(gs, bitmaps[gScreenSelect], MStill.data, MStill.size);
    
    CalcFrameTime();

    pen.pen_X       = 500.;
    pen.pen_Y       = 425.;
    sprintf(txtBuffer, "%2.1f", (1. / _frametime));
    DrawString(gs, font, &pen, txtBuffer, strlen(txtBuffer));

    GS_SendList(gs);
    GS_EndFrame(gs);
    
    gScreenSelect = 1 - gScreenSelect;
    GS_SetDestBuffer(gs, bitmaps[gScreenSelect]);
  }
  
  MPS_FreeMPEGStill(MStill.data);
  MPS_TeardownMPEGStill();
}

void WaitForAnyButton(ControlPadEventData *cped)
{
  GetControlPad(1, TRUE, cped); /* Button down */
  GetControlPad(1, TRUE, cped); /* Button up */
}

void SetUpDisplay(void)
{
  int32	i;
  Bitmap	*bmp;
  
  gScreenSelect = 0;
  OpenGraphicsFolio();
  renderSig = AllocSignal(0);
  
  if(renderSig<0) 
    printf("ERROR in allocsignal\n");
  
  gs = GS_Create();
  GS_AllocLists(gs, 2, 16384);
  GS_AllocBitmaps(bitmaps, 640, 480, BMTYPE_16, 3, 0);
  GS_SetDestBuffer(gs, bitmaps[0]);
  
  if (GS_SetVidSignal(gs, renderSig)<0) printf("ERROR in setvidsignal\n");
  
  /*
   * Clear the frame buffers.
   */
  for (i = 0; i < 3; i++)
    {
      bmp = (Bitmap *) LookupItem(bitmaps[i]);
      memset(bmp->bm_Buffer, 0, bmp->bm_BufferSize);
    }
  
  viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			  VIEWTAG_VIEWTYPE, VIEWTYPE_16_640_LACE,	/* VIEWTYPE_16	*/
			  VIEWTAG_RENDERSIGNAL, renderSig,
			  VIEWTAG_BITMAP, bitmaps[0],
			  TAG_END);
  if (viewItem < 0)
    {
      PrintfSysErr(viewItem); 
      exit(1); 
    }
  AddViewToViewList(viewItem, 0);
  GS_SetView(gs, viewItem);
  
  return;
}

void GeneralInit(void)
{
  gfloat i,ii;
  long t;
  Err err;
  
  InitEventUtility (1, 0, LC_ISFOCUSED);
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
  stat.codec_TagArg[1].ta_Arg = (void *) 16L;
  stat.codec_TagArg[2].ta_Tag = VID_CODEC_TAG_HSIZE;
  stat.codec_TagArg[2].ta_Arg = (void *) 640L;
  stat.codec_TagArg[3].ta_Tag = VID_CODEC_TAG_VSIZE;
  stat.codec_TagArg[3].ta_Arg = (void *) 480L;
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

