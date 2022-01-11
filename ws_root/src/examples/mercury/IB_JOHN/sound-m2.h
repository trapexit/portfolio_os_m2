
void M2_SetFrq(uint8 voice, uint32 period);
void M2_SetPan(uint8 voice, uint8 Pos);
void M2_SetVol(uint8 voice, uint8 Vol);
void M2_PlayVoice(uint8 voice, uint8 Instr, uint16 Offs);
void M2_StopVoice(uint8 voice);
void M2_Init(void);
void M2_Remove(void);
void M2_SetupInst(uint8 instr, RawFile *s3m, s3minstr *CurInst);

extern instrument instrs[99];								/* These are the actual instruments to be played */
extern float32 globalvolume;								/* This is the global volume adjustment */
extern float32 globalpitch;									/* This is the global pitch adjustment */
