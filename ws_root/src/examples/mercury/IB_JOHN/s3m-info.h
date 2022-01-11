
/*********************************************************************/
#define MAX_INSTRUMENTS					(99)				/* Total number of instruments that can be loaded */
#define MAX_PATTERNS					(99)				/* Maximum number of patterns in a song */
#define MAX_CHANNELS					(20)				/* We can play up to this many sounds at the same time */
#define MAX_TEMPO						(250)				/* This is the maximum tempo (lower number speeds things up) */


/*********************************************************************/

typedef struct ntype						/* a single note entry */
{
	uint8 pitchoct;							/* pitch / octave */
	uint8 instr;							/* instrument number */
	uint8 volume;							/* volume / efx */
	uint8 command;							/* standard efx */
	uint8 vinfo;							/* volume /efx infobyte */
	uint8 info;								/* standard efx info */
} Note;

typedef struct rowst						/* a single, lonely row of notes */
{
	Note note[32];
} Row;

typedef struct patternst					/* a single pattern */
{
	Row row[64];
} Pattern;

typedef struct
{
	uint8 note;								/* last known note */
	uint8 octave;							/* last known octave */
	uint16 pitch;							/* last known pitch */
	uint16 tpitch;							/* for efx that destroy main pitch */
	uint8 instr;							/* last known instrument */
	char vol;								/* current volume */
	uint8 new;								/* 1 if new note found,should be cleared after play on */
	uint8 ninst;							/* 1 if new instrument entry found */
	uint8 off;								/* 1 if pitcoct=254="^^^"=noteoff */
	uint8 info;								/* last known info */
	uint8 efx;								/* last known efx */
	uint8 vsl;								/* last info at vslide ("D") */
	uint8 tdn;								/* last info at tone down */
	uint8 tup;								/* last info at tone up */
	uint8 prt;								/* last info at note port */
	uint16 prtend;							/* portamento target */
	uint8 vib;								/* last info at vibrato*/
	uint8 sinepos;							/* vib sine position */
	uint8 trg;								/* last retrig */	uint8 trgcnt;							/* retrig counter */
	uint8 cut;								/* notecut count */
	uint8 dly;								/* notedelay count */
} channel;

			
/* 			instrument type definitions			*/

typedef struct instype
{
	int8 *sampledata;						/* This is the sound in RAW 8-bit format */
	uint32 length;							/* The size of the sample */
	uint32 loopstart;						/* loop start offset */
	uint32 loopend;							/* loop end or end offset*/
	uint8  volume;							/* instrument volume */
	uint8  loop;							/* loop infobyte (on/off) */
	uint16  c2spd;							/* s3m tunning value */
	uint16  tune[12];						/* low octave tunning */
	char  dosname[12];						/* dos filename */
	char  name[32];							/* instrument name */
} instrument;


/*********************************************************************/


typedef struct s3mfileheadtype				/* s3m header */
{
	char Sname[28];
	uint8 Byte1A;
	uint8 Type;
	uint16 hole1;
	uint16 OrdNum;
	uint16 InsNum;
	uint16 PatNum;
	uint16 Flags;
	uint16 Version;
	uint16 FileForm;
	char SCRM[4];
	uint8 GlobVol;
	uint8 InitSpd;
	uint8 InitTem;
	uint8 MasterVol;
	uint8 UltrClkRem;
	uint8 DefPan;
	uint16 hole2[4];
	uint16 Special;
	uint8 ChnSet[32];
} s3mhead;


typedef struct ScrsHeadtype					/* s3m & digiplayer instrument header */
{
	uint8 Type;
	char DOSName[12];
	uint8 MemSegHi;
	uint16 MemSegLo;
	uint16 Length;
	uint16 LengthHi;
	uint16 LoopBg;
	uint16 LoopBgHi;
	uint16 LoopNd;
	uint16 LoopNdHi;
	uint8 Vol;
	uint8 Hole1;
	uint8 Pack;
	uint8 Flags;
	uint16 C2spd;
	uint16 C2sHi;
	uint8 Hole2[4];
	uint16 GravPos;
	uint16 SbLoop;
	uint16 LastUs;
	uint16 LastUsHi;
	char Sname[28];
	char SCRS[4];
} s3minstr;


/*********************************************************************/

void Abort(char *ermsg);
void Play_InnerClean(void);

/*********************************************************************/

