/*********************************************************************/

Item Play_Init(char *fname);
void Play_Start(void);
void Play_Pause(void);
void Play_Stop(void);
void Play_PrintInfo(void);										/* Print out info about the current song */
void Play_PrintSamples(void);									/* Print out the list of samples, and some info */

void Play_GetVoice(int16 voice, int16 *pitch, int16 *volume);	/* Just a very basic way of showing sychronization */
void Play_GetInst(int16 inst, int16 *pitch, int16 *volume);		/* Just a very basic way of showing sychronization */
int16 Play_GetFlash(void);										/* This is NOT the best way to do it... */
int16 Play_GetBigFlash(void);									/* This is NOT the best way to do it... */

void Play_ChangeTempo(float32 multiplier);						/* Changes the tempo (global) */
void Play_ChangePitch(float32 multiplier);						/* Changes the pitch (global) */
void Play_ChangeVolume(float32 multiplier);						/* Changes the volume (global) */

void Play_FastForward(void);									/* Just like a tape deck, only cooler */
void Play_Rewind(void);
void Play_Randomize(void);										/* Randomizes the orderlist */

/*********************************************************************/

