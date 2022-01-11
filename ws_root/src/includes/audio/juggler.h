#ifndef __AUDIO_JUGGLER_H
#define __AUDIO_JUGGLER_H


/****************************************************************************
**
**  @(#) juggler.h 95/12/15 1.9
**
**  Juggler Includes
**
****************************************************************************/


#ifndef __AUDIO_COBJ_H
#include <audio/cobj.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


typedef  uint32 Time;

/* Define basic root class Instance Variables for Juggler. */
#define JuggleeIV \
	COBObjectIV; \
	void   *jglr_Parent;  /* Who started you in hierarchy. */ \
	Time    jglr_StartTime; \
	Time    jglr_NextTime; \
	int32   jglr_RepeatCount; /* Decremented each time till zero */ \
	int32   jglr_Active; /* True if currently executing. */ \
	int32 (*jglr_StartFunction)( void *, uint32 ); \
	int32 (*jglr_RepeatFunction)( void *, uint32); \
	int32 (*jglr_StopFunction)( void *, uint32 ); \
	uint32  jglr_StartDelay; \
	uint32  jglr_RepeatDelay; \
	uint32  jglr_StopDelay; \
	int32   jglr_CurrentIndex;   /* Index of current thing. */ \
	void   *jglr_UserContext; \
	int32   jglr_Many;       /* Number of valid subunits */ \
	uint32  jglr_Flags

typedef struct
{
		JuggleeIV;
} Jugglee;

/* Flags for Juggler */
#define JGLR_FLAG_MUTE (0x0001)

extern COBClass JuggleeClass;

/* Sequence Structure */
#define SequenceIV \
	JuggleeIV; \
	int32 (*seq_InterpFunction)( void *, void *, void * ); \
	int32   seq_Max;        /* Number of Events allocated. */ \
	int32   seq_EventSize;  /* Size in bytes of an event */ \
	char   *seq_Events     /* Pointer to event data */

typedef struct
{
	SequenceIV;
} Sequence;

extern COBClass SequenceClass;

/* Collection Structure ***************************************/
#define CollectionIV \
	JuggleeIV; \
	int32 (*col_SelectorFunction)(); \
	List    col_Children; \
	int32   col_Pending

typedef struct
{
	CollectionIV;
} Collection;


extern COBClass CollectionClass;

typedef struct
{
	List	jcon_ActiveObjects;
	Time	jcon_NextTime;
	Time	jcon_CurrentTime;
	Time	jcon_NextSignals;
	Time	jcon_CurrentSignals;
} JugglerContext;

extern JugglerContext JugglerCon;

typedef struct PlaceHolder
{
	Node     plch_Node;
	Jugglee *plch_Thing;
	int32    plch_NumRepeats;
} PlaceHolder;

/* Define TAG ARGS */
enum juggler_tags
{
        JGLR_TAG_CONTEXT = TAG_ITEM_LAST+1,
        JGLR_TAG_START_DELAY,
        JGLR_TAG_REPEAT_DELAY,
        JGLR_TAG_STOP_DELAY,
        JGLR_TAG_START_FUNCTION,
        JGLR_TAG_REPEAT_FUNCTION,
        JGLR_TAG_STOP_FUNCTION,
        JGLR_TAG_SELECTOR_FUNCTION,
        JGLR_TAG_INTERPRETER_FUNCTION,
        JGLR_TAG_DURATION,
        JGLR_TAG_MAX,
        JGLR_TAG_EVENTS,
        JGLR_TAG_EVENT_SIZE,
        JGLR_TAG_MANY,
        JGLR_TAG_MUTE
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32 InitJuggler( void );
int32 TermJuggler( void );
int32 BumpJuggler( Time CurrentTime, Time *NextTime,
	int32 CurrentSignals, int32 *NextSignals);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_JUGGLER_H */
