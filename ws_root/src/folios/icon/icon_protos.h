
Err SaveIconIFF(IFFParser *p, char *utfs, char *appname, TimeVal *tbf);
Err LoadIconIFF(IFFParser *p, Icon **iconout);
Err internalGetHWIcon(HardwareID id, void *dest, uint32 length);
Err GetHWIcon(HardwareID id, void *d, uint32 l);
Err LoadIconDIPIR(HardwareID hwid, Icon **iout);
Err ConvertDipirToSprite(VideoImage *vi, SpriteObj *sp);
Err ConvertHWIDToSprite(HardwareID hwid, SpriteObj *sp);
Err LoadIconDDF(char *path, Icon **iconout);
Err LoadIconFS(char *path, Icon **iconout);
Err FindDDFPath(char *driver, char *outpath, int32 outlen);

