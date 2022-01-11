/*
 *  @(#) iconcode.c 96/04/24 1.2
 *  Exposed Icon routines.
 */

#include <ui/icon.h>
#include <kernel/tags.h>
#include <misc/iff.h>
#include <graphics/frame2d/loadtxtr.h>
#include <dipir/dipirpub.h>
#include "icon_protos.h"

Err LoadIcon(Icon **icon, const TagArg *tags)
{
    Err                  ret;
    IFFParser           *p;
    char                *filename;
    uint32               source;
    uint32               parsetype;
    TagArg              *FileTag, *IFFParserTag, *IFFParseTypeTag;
    TagArg              *FSTag,   *DriverTag,    *HWTag;
    HardwareID           hwid;

    enum                 srcmodes
    {
        FROM_FILE,      /* Load Icon from a file */
        FROM_DDF,       /* Load Icon from a device */
        FROM_DIPIR,     /* Load Icon from Dipir */
        FROM_FS         /* Load Icon for a FS */
    };

    /* --------- Preinitialize --------- */
    filename        =                   NULL;
    p               =   (IFFParser *)   NULL;
    parsetype       =                   -1;
    source          =                   -1;
    hwid            =                   0;

    /* --------- Make sure the IFF Folio is open --------- */
    ret = OpenIFFFolio();
    if (ret >= 0)
    {
        /* --------- Parse TagArg parameters --------- */
        
        FileTag         =   FindTagArg(tags, LOADICON_TAG_FILENAME);
        IFFParserTag    =   FindTagArg(tags, LOADICON_TAG_IFFPARSER);
        IFFParseTypeTag =   FindTagArg(tags, LOADICON_TAG_IFFPARSETYPE);
        FSTag           =   FindTagArg(tags, LOADICON_TAG_FILESYSTEM);
        DriverTag       =   FindTagArg(tags, LOADICON_TAG_DRIVER);
        HWTag           =   FindTagArg(tags, LOADICON_TAG_HARDWARE);
    
        if ( (!FileTag) && (!IFFParserTag) && (!FSTag) &&       
             (!DriverTag) && (!HWTag) )
            ret = ICON_ERR_ARGUMENTS;
    
        /* --------- If from a file, get filename --------- */
        if ( (FileTag) && (ret >= 0) )
        {
            if ( (IFFParserTag) || (FSTag) || (DriverTag) || (HWTag) )
                ret         =   ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                filename    =   FileTag->ta_Arg;
                source      =   FROM_FILE;
                ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, filename, TAG_END);
                if (ret >= 0)
                {
                    parsetype   =   LOADICON_TYPE_AUTOPARSE;
                }
            }
        }
    
        /* --------- If from an already existing IFF stream --------- */
        if ( (IFFParserTag) && (ret >= 0) )
        {
            if ( (FileTag) || (FSTag) || (DriverTag) || (HWTag) )
                ret = ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                if (IFFParseTypeTag)
                {
                    p           =   IFFParserTag->ta_Arg;
                    source      =   FROM_FILE;
                    parsetype   =   (uint32) IFFParseTypeTag->ta_Arg;
                }
                else
                    ret = ICON_ERR_ARGUMENTS;
            }
        }
    
        /* --------- From HW? --------- */
        if ( (HWTag) && (ret >= 0) )
        {
            if ( (IFFParserTag) || (FileTag) || (FSTag) || (DriverTag) )
                ret             =   ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                source          =   FROM_DIPIR;
                hwid            =   (uint32) HWTag->ta_Arg;
            }
        }
    
        /* --------- From a driver? --------- */
        if ( (DriverTag) && (ret >= 0) )
        {
            if ( (HWTag) || (IFFParserTag) || (FileTag) || (FSTag) )
                ret             =   ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                source          =   FROM_DDF;
                filename        =   DriverTag->ta_Arg;
            }
        }
    
        /* --------- From a filesystem? --------- */
        if ( (FSTag) && (ret >= 0) )
        {
            if ( (DriverTag) || (HWTag) || (IFFParserTag) || (FileTag) )
                ret             =   ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                source          =   FROM_FS;
                filename        =   FSTag->ta_Arg;
            }
        }
    
        /* --------- Should we preparse the stream? --------- */
        if ( (ret >= 0) && (source == FROM_FILE) && 
             (parsetype == LOADICON_TYPE_AUTOPARSE) )
        {
            /* Let's look for the first chunk. */
            /* If it's not FORM ICON, we dunno what to do */
            ret = ParseIFF(p, IFF_PARSE_RAWSTEP);
            if (ret >= 0)
            {   
                if ( (ret == IFF_PARSE_EOF) || (ret == IFF_PARSE_EOC) )
                    ret = ICON_ERR_UNKNOWNFORMAT;
                if (!ret)
                {
                    ContextNode *cn;
                    cn = GetCurrentContext(p);
                    if (!( (cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_ICON) ))
                        ret = ICON_ERR_UNKNOWNFORMAT;
                }
            }
        }
    
        /* --------- Main apex, where to get Icon from ? --------- */
        if (ret >= 0)
        {
            switch  (source)
            {
                case FROM_FILE:
                {
                    ret = LoadIconIFF(p, icon);
                    break;
                }
                case FROM_DIPIR:
                {
                    ret = LoadIconDIPIR(hwid, icon);
                    break;
                }
                case FROM_FS:
                {
                    ret = LoadIconFS(filename, icon);
                    break;
                }
                case FROM_DDF:
                {
                    char ddfpath[132];
                    ret = FindDDFPath(filename, ddfpath, 131);
                    if (ret >= 0)
                    {
                        ret = LoadIconDDF(ddfpath, icon);
                    }
                    break;
                }
            }
        }
    
        /* --------- Cleanup --------- */
        if ( (filename) && (p) )
        {
            /* If we created the parser, delete it */
            DeleteIFFParser(p);
        }
        CloseIFFFolio();
    }   
    
    return(ret);
}

Err SaveIcon(char *utf, char *app, TagArg *tags)
{
    Err          ret;
    IFFParser   *p;
    char        *filename;
    TimeVal     *tbf, zerotbf={0,0};
    TagArg      *FileTag, *IFFParserTag, *TimeFramesTag;

    /* --------- Preinitialize --------- */
    filename        =                   NULL;
    tbf             =                   &zerotbf; 
    p               =   (IFFParser *)   NULL;

    /* --------- Make sure the IFF Folio is open --------- */

    ret = OpenIFFFolio();
    if (ret >= 0)
    {
        /* --------- Parse the IFF Tags --------- */
    
        FileTag         =   FindTagArg(tags, SAVEICON_TAG_FILENAME);
        IFFParserTag    =   FindTagArg(tags, SAVEICON_TAG_IFFPARSER);
        TimeFramesTag   =   FindTagArg(tags, SAVEICON_TAG_TIMEBETWEENFRAMES);
    
        /* --------- If they're just giving a filename --------- */
        if (FileTag)
        {
            /* They can't have both FileTag and IFFParserTag ... */
            if (IFFParserTag)
                ret = ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                filename = FileTag->ta_Arg;
                /* Open the IFF file and starting parsing */
                ret = CreateIFFParserVA(&p, TRUE, IFF_TAG_FILE, filename, TAG_END);
            }
        }
    
        /* --------- If they passed in an IFFParser --------- */
        if (IFFParserTag)
        {
            if (FileTag)
                ret = ICON_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                p = (IFFParser *) IFFParserTag->ta_Arg;
            }
        }
    
        /* --------- Make sure they gave us something --------- */
        if ( (!FileTag) && (!IFFParserTag) )
            ret = ICON_ERR_ARGUMENTS;
    
        /* --------- Did they ask for time between frames? --------- */
        if (TimeFramesTag)
            tbf = (TimeVal *) TimeFramesTag->ta_Arg;
    
        /* --------- Write out the icon --------- */
        if (ret >= 0)
        {
            ret = SaveIconIFF(p, utf, app, tbf);
        }
    
        /* --------- Cleanup --------- */
        if ( (filename) && (p) )
        {
            /* If we allocated the IFF Parser, we delete it */
            DeleteIFFParser(p);
        }
        CloseIFFFolio();
    }
    
    return(ret);
}

