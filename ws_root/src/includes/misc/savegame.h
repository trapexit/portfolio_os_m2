#ifndef __MISC_SAVEGAME_H
#define __MISC_SAVEGAME_H


/******************************************************************************
**
**  @(#) savegame.h 96/06/13 1.3
**
**  Includes and defines for the savegame folio
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __UI_ICON_H
#include <ui/icon.h>
#endif


/*****************************************************************************/


/* kernel interface definitions */
#define SAVEGAME_FOLIONAME   "savegame"


/*****************************************************************************/


/* Error codes */

#define MakeSGErr(svr,class,err) MakeErr(ER_FOLI,ER_SAVEGAME,svr,ER_E_SSTM,class,err)

/* Bad pointer passed in */
#define SG_ERR_BADPTR                MakeSGErr(ER_SEVERE, ER_C_STND, ER_BadPtr)

/* Not enough memory available */
#define SG_ERR_NOMEM                 MakeSGErr(ER_SEVERE, ER_C_STND, ER_NoMem)

/* The options given are mutually exclusive */
#define SG_ERR_MUTUALLYEXCLUSIVE     MakeSGErr(ER_SEVERE, ER_C_NSTND, 1)
/* The data encountered is in an unknown format */
#define SG_ERR_UNKNOWNFORMAT         MakeSGErr(ER_SEVERE, ER_C_NSTND, 2)
/* The parameters given are incomplete */
#define SG_ERR_INCOMPLETE            MakeSGErr(ER_SEVERE, ER_C_NSTND, 3)
/* We have more data than buffers to save the data to */
#define SG_ERR_OVERFLOW              MakeSGErr(ER_SEVERE, ER_C_NSTND, 4)


/*****************************************************************************/


/* definition for IFF chunks */
#define ID_SGME MAKE_ID('S', 'G', 'M', 'E')
#define ID_GDTA MAKE_ID('G', 'D', 'T', 'A')
#define ID_GDTC MAKE_ID('G', 'D', 'T', 'C')
#define ID_GINF MAKE_ID('G', 'I', 'N', 'F')

/* Associated with the GINF chunk */
typedef struct GameInfoChunk
{
    char        GameIdentifier[32];
} GameInfoChunk;

/* Associated with the GDTC chunk */
typedef struct CompressedGameDataChunk
{
    uint16      Length;
    /* ... followed by compressed data ... */
} CompressedGameDataChunk;

/* The SGData structure */
typedef struct SGData
{
    void        *Buffer;
    uint32       Length;
    uint32       Actual;
} SGData;

/* Callback function */
typedef Err (*SGCallBack)(SGData *gs, void *PrivateData);

typedef enum gameloadtags
{
    LOADGAMEDATA_TAG_FILENAME=10,   /* The filename of a file to load */
    LOADGAMEDATA_TAG_IFFPARSER,     /* To use an IFFParser * instead  */
    LOADGAMEDATA_TAG_BUFFERARRAY,   /* An array of buffers to load to */
    LOADGAMEDATA_TAG_CALLBACK,      /* A callback for each data chunk */
    LOADGAMEDATA_TAG_CALLBACKDATA,  /* Private data for above         */
    LOADGAMEDATA_TAG_ICON,          /* Where to load an icon to       */
    LOADGAMEDATA_TAG_IDSTRING,      /* Where to load the id string to */
};

typedef enum savegametags
{
    SAVEGAMEDATA_TAG_FILENAME=10,   /* The filename to save to        */
    SAVEGAMEDATA_TAG_IFFPARSER,     /* Save to an IFFParser * instead */
    SAVEGAMEDATA_TAG_IDSTRING,      /* Identifier for this game       */
    SAVEGAMEDATA_TAG_ICON,          /* UTF data for icon imagery      */
    SAVEGAMEDATA_TAG_BUFFERARRAY,   /* Array of ptrs to data to save  */
    SAVEGAMEDATA_TAG_CALLBACK,      /* A callback for each data chunk */
    SAVEGAMEDATA_TAG_CALLBACKDATA,  /* Private data for above         */
    SAVEGAMEDATA_TAG_TIMEBETWEENFRAMES, /* Time btwn anim frames      */
};


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* folio management */
Err OpenSaveGameFolio(void);
Err CloseSaveGameFolio(void);

/* Function prototypes */
Err LoadGameData(const TagArg *tags);
Err LoadGameDataVA(uint32 tag, ...);
Err SaveGameData(char *AppName, const TagArg *tags);
Err SaveGameDataVA(char *AppName, uint32 tag, ...);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif /* __MISC_SAVEGAME_H */
