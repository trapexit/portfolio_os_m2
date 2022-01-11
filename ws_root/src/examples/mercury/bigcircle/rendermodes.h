/* Mode used for rendering */
#define LIT_DYN		0
#define LIT_PRE		1

#define FS_NONE		0
#define FS_FOG		1
#define FS_SPEC		2

#define TRANS_NONE	0
#define TRANS_TRUE	1

#define TEX_NONE	0
#define TEX_TRUE	1
#define TEX_ENV		2

#define DYN		0
#define DYNTEX		1
#define DYNENV		2
#define DYNTRANS	3
#define DYNTRANSTEX	4
#define DYNTRANSENV	5
#define DYNFOG		6
#define DYNFOGTEX	7
#define DYNFOGENV	8
#define DYNFOGTRANS	9
#define DYNFOGTRANSTEX	10 /* Illegal */
#define DYNFOGTRANSENV	11 /* Illegal */
#define DYNSPEC		12
#define DYNSPECTEX	13
#define DYNSPECENV	14
#define DYNSPECTRANS	15
#define DYNSPECTRANSTEX	16 /* Illegal */
#define DYNSPECTRANSENV	17 /* Illegal */
#define PRE		18
#define PRETEX		19
#define PREENV		20 /* Illegal */
#define PRETRANS	21
#define PRETRANSTEX	22
#define PRETRANSENV	23 /* Illegal */
#define PREFOG		24
#define PREFOGTEX	25
#define PREFOGENV	26 /* Illegal */
#define PREFOGTRANS	27
#define PREFOGTRANSTEX	28 /* Illegal */
#define PREFOGTRANSENV	29 /* Illegal */
#define PRESPEC		30 /* Illegal */
#define PRESPECTEX	31 /* Illegal */
#define PRESPECENV	32 /* Illegal */
#define PRESPECTRANS	33 /* Illegal */
#define PRESPECTRANSTEX	34 /* Illegal */
#define PRESPECTRANSENV	35 /* Illegal */

#define CASECODE(dynpre, fogspec, trans, tex) (((dynpre)*18) + ((fogspec)*6) + (trans)*3 + (tex))

extern uint32 dynpre;
extern uint32 fogspec;
extern uint32 transp;
extern uint32 texp;
extern uint32 rendermode;

/* Texture filter modes */
#define POINT 		0
#define LINEAR		1
#define BILINEAR	2
#define TRILINEAR	3

extern uint32 filtermode;

