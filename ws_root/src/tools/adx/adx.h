/* @(#) adx.h 95/12/04 1.13 */

#ifndef __ADX_H
#define __ADX_H


/****************************************************************************/


/* A bunch of stuff stolen from the Portfolio includes */

typedef signed char	int8;
typedef signed short	int16;
typedef signed long	int32;
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned long	uint32;
typedef uint8	        bool;

#ifndef TRUE
#define TRUE ((bool)1)
#define FALSE ((bool)0)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct Node
{
	struct Node *n_Next;
	struct Node *n_Prev;
	char        *n_Name;
} Node;

typedef struct Link
{
	struct Link *flink;
	struct Link *blink;
} Link;

typedef union ListAnchor
{
    struct
    {
	Link links;
	Link *filler;
    } head;
    struct
    {
	Link *filler;
	Link links;
    } tail;
} ListAnchor;

typedef struct List
{
	uint32 l_Flags;
	ListAnchor ListAnchor;
} List;


/* return the first node on the list or the anchor if empty */
#define FIRSTNODE(l)	((Node *)((l)->ListAnchor.head.links.flink))
#define FirstNode(l)	((Node *)((l)->ListAnchor.head.links.flink))

/* return the last node on the list or the anchor if empty */
#define LASTNODE(l)	((Node *)((l)->ListAnchor.tail.links.blink))
#define LastNode(l)	((Node *)((l)->ListAnchor.tail.links.blink))

/* define for finding end while using flink */
#define ISNODE(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.blink))
#define IsNode(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.blink))

/* define for finding end while using blink */
#define ISNODEB(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.flink))
#define IsNodeB(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.flink))

#define ISLISTEMPTY(l)	(!ISNODE((l),FIRSTNODE(l)))
#define IsListEmpty(l)	(!IsNode((l),FirstNode(l)))

#define ISEMPTYLIST(l)	(!ISNODE((l),FIRSTNODE(l)))
#define IsEmptyList(l)	(!IsNode((l),FirstNode(l)))

#define NEXTNODE(n)	(((Node *)(n))->n_Next)
#define NextNode(n)	(((Node *)(n))->n_Next)
#define PREVNODE(n)	(((Node *)(n))->n_Prev)
#define PrevNode(n)	(((Node *)(n))->n_Prev)

/* Scan list l, for nodes n, of type t.
 *
 * WARNING: You cannot remove the current node from the list when using this
 *          macro.
 */
#define SCANLIST(l,n,t) for (n=(t *)FIRSTNODE(l);ISNODE(l,n);n=(t *)NEXTNODE(n))
#define ScanList(l,n,t) for (n=(t *)FIRSTNODE(l);ISNODE(l,n);n=(t *)NEXTNODE(n))

/* Scan a list backward, from tail to head */
#define SCANLISTB(l,n,t) for (n=(t *)LASTNODE(l);ISNODEB(l,n);n=(t *)PREVNODE(n))
#define ScanListB(l,n,t) for (n=(t *)LASTNODE(l);ISNODEB(l,n);n=(t *)PREVNODE(n))

extern Node *RemHead(List *l);
extern Node *RemTail(List *l);
extern void AddTail(List *l, Node *n);
extern void AddHead(List *l, Node *n);
extern void RemNode( Node *n);
extern void PrepList(List *l);
extern void InsertNodeBefore(Node *oldNode, Node *newNode);
extern void InsertNodeAlpha(List *l, Node *newNode);
extern Node *FindNamedNode(const List *l, const char *name);

extern void *AllocNode(uint32 nodeSize, const char *name);
extern void FreeNode(void *);


/*****************************************************************************/


typedef enum
{
    FT_NORMAL,
    FT_PREFORMATTED
} FormatType;

typedef struct
{
    Node  ln_Link;
    char *ln_Data;
} Line;

typedef struct
{
    Node        s_Link;
    List        s_Lines;
    FormatType  s_Type;
} Section;

typedef struct
{
    Node  ad_Link;
    List  ad_HiddenAliases;
    List  ad_VisibleAliases;
    List  ad_Description;
    List  ad_Sections;
    bool  ad_Private;
} Autodoc;

typedef struct
{
    Node  gr_Link;
    List  gr_Autodocs;
} Group;

typedef struct
{
    Node     ch_Link;
    Autodoc *ch_Autodoc;
    List     ch_Groups;
    char    *ch_FullName;
    char    *ch_ManualName;
} Chapter;

typedef struct
{
    Node     mn_Link;
    Autodoc *mn_Autodoc;
    List     mn_Chapters;
    char    *mn_FullName;
} Manual;


/* Example:
 *
 *   Manual (Portfolio Programmer's Reference)
 *     Chapter (Kernel)
 *       Group (Lists)
 *         Autodoc (PrepList)
 *           Section (Name)
 *           Section (Synopsis)
 *           Section (Description)
 *         Autodoc (AddHead)
 *         Autodoc (AddTail)
 *       Group (Memory)
 *       Group (Semaphores)
 *     Chapter (Compression Folio)
 *     Chapter (Event Broker)
 *
 *   Manual (Music Programmer's Reference)
 *     Chapter (Loud Music)
 *     Chapter (Soft Music)
 *
 * There can be any number of manuals. Each manual contains any number
 * of chapters. Within a chapter, entries are divided by groups of related
 * entries. Each group can have any number of individual autodocs. Each
 * autodoc can have any number of individual sections. And finally, each
 * section can have any number of lines.
 */


/****************************************************************************/


int32 OutputASCII(const List *manualList, const char *outputDir);
int32 OutputHTML(const List *manualList, const char *outputDir);
int32 Output411(const List *manualList, const char *outputDir);
int32 OutputMAN(const List *manualList, const char *outputDir);
int32 OutputPrint(const List *manualList, const char *outputDir);


/****************************************************************************/


void ParseWarning(const char *msg, ...);
void ParseError(const char *msg, ...);
void IOError(const char *msg, ...);
void NoMemory(void);


/****************************************************************************/


/* Path name portability */
#ifdef macintosh
#define SEP ':'
#else
#define SEP '/'
#endif


/****************************************************************************/


#endif /* __ADX_H */
