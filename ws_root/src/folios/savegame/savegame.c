/* @(#) savegame.c 96/07/09 1.2 */

#include <stdio.h>
#include <kernel/types.h>
#include <kernel/tags.h>
#include <kernel/mem.h>
#include <misc/compression.h>
#include <misc/iff.h>
#include <misc/savegame.h>
#include <string.h>

Err GameReadCompressedChunk(IFFParser *p, void *d, uint32 l)
{
    void        *loadbuff, *decompbuff;
    uint32       loadlen, decomplen;
    Err          ret;
    ContextNode *cn;

    cn = GetCurrentContext(p);

    loadlen = (cn->cn_Size - cn->cn_Offset);
    loadbuff = AllocMem(loadlen , MEMTYPE_ANY);
    if (loadbuff)
    {
        uint32      readwords, decompwords;

        decompwords = ((l-1) >> 2) + 1;
        decomplen = (decompwords << 2);
        decompbuff = AllocMem(decomplen, MEMTYPE_ANY);
        if (decompbuff)
        {

            ret = ReadChunk(p, loadbuff, loadlen );
            if (ret >= 0)
            {
                readwords = ((ret-1) >> 2) + 1;

                ret = SimpleDecompress(loadbuff, readwords, decompbuff, decompwords);
                if (ret >= 0)
                    memcpy(d, decompbuff, l);
            }
            FreeMem(decompbuff, decomplen);
        }
        else
            ret = SG_ERR_NOMEM;

        FreeMem(loadbuff, loadlen);
    }
    else
        ret = SG_ERR_NOMEM;

    return(ret);
}

Err GameWriteCompressedChunk(IFFParser *p, void *d, uint32 l)
{
    void        *tempbuff;
    Err          ret;
    uint32       wordlen, wordleninbytes;

    if (l)
        wordlen =        ((l-1) >> 2) + 1;      /* divide by four, round up */
    else
        wordlen = 0;

    wordleninbytes = (wordlen << 2);

    tempbuff = AllocMem( wordleninbytes, MEMTYPE_ANY);
    if (tempbuff)
    {
        ret = SimpleCompress(d, wordlen, tempbuff, wordlen);
        if (ret >= 0)
        {
            CompressedGameDataChunk     gdc;
            uint32                      compressedlen;

            compressedlen =     ret << 2;

            gdc.Length  =       l;
            ret = PushChunk(p, 0, ID_GDTC, IFF_SIZE_UNKNOWN_32);
            if (ret >= 0)
            {
                ret = WriteChunk(p, &gdc, sizeof(CompressedGameDataChunk));
                if (ret >= 0)
                    ret = WriteChunk(p, tempbuff, compressedlen);
                PopChunk(p);
            }
        }
        else /* If the data didn't compress at least to less than original */
        {
            ret = PushChunk(p, 0, ID_GDTA, IFF_SIZE_UNKNOWN_32);
            if (ret >= 0)
            {
                WriteChunk(p, d, l);
                PopChunk(p);
            }
        }
        FreeMem(tempbuff, wordleninbytes);
    }
    else
        ret = SG_ERR_NOMEM;

    return(ret);
}

Err LoadGameData(const TagArg *tags)
{
    Err              ret;
    IFFParser       *p;
    TagArg          *FoundTag;
    SGData          *SGDArray;
    SGCallBack       CB;
    void            *CBData;
    uint32           Depth;
    bool             LoadedIcon;
    GameInfoChunk    GIData;
    Icon           **PutIcon;
    char            *PutIDString;
    char            *filename;

    /* --------- Preinitialization --------- */
    p           = (IFFParser *)     NULL;
    filename    =                   NULL;
    SGDArray    = (SGData *)        NULL;
    CB          = (SGCallBack)      NULL;
    CBData      = (void *)          NULL;
    Depth       =                   1;
    LoadedIcon  =                   FALSE;
    PutIcon     = (Icon **)         NULL;
    PutIDString =                   NULL;


    memset(&GIData, 0, sizeof(GameInfoChunk));

    ret = OpenIconFolio();
    if (ret >= 0)
    {
        /* Parse Taglists */

        /* Did they give us a filename? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_FILENAME);
        if (FoundTag)
        {
            if (FindTagArg(tags, LOADGAMEDATA_TAG_IFFPARSER))
                ret = SG_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                filename = FoundTag->ta_Arg;

                ret = CreateIFFParserVA(&p, FALSE, IFF_TAG_FILE, filename,
                                        TAG_END);
                /* Preparse out to the FORM SGME header */
                if (ret >= 0)
                {
                    ret = ParseIFF(p, IFF_PARSE_RAWSTEP);
                    if (ret >= 0)
                    {
                        ContextNode     *cn;

                        cn = GetCurrentContext(p);
                        if ( !((cn->cn_ID == ID_FORM) && (cn->cn_Type == ID_SGME)) )
                            ret = SG_ERR_UNKNOWNFORMAT;

                    }
                }
            }
        }

        /* Did they give us an IFFParser? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_IFFPARSER);
        if (FoundTag)
        {
            p = (IFFParser *)           FoundTag->ta_Arg;
        }

        /* How 'bout an array of SGData's? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_BUFFERARRAY);
        if (FoundTag)
        {
            if (FindTagArg(tags, LOADGAMEDATA_TAG_CALLBACK))
                ret = SG_ERR_MUTUALLYEXCLUSIVE;
            else
                SGDArray = (SGData *)   FoundTag->ta_Arg;
        }

        /* A callback? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_CALLBACK);
        if (FoundTag)
        {
            CB = (SGCallBack)           FoundTag->ta_Arg;
        }

        /* CB private data? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_CALLBACKDATA);
        if (FoundTag)
        {
            if (FindTagArg(tags, LOADGAMEDATA_TAG_CALLBACK))
            {
                CBData = (void *)       FoundTag->ta_Arg;
            }
            else
                ret = SG_ERR_INCOMPLETE;
        }

        /* Do they want the icon? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_ICON);
        if (FoundTag)
            PutIcon = (Icon **)         FoundTag->ta_Arg;

        /* Do they want the ID string? */
        FoundTag = FindTagArg(tags, LOADGAMEDATA_TAG_IDSTRING);
        if (FoundTag)
            PutIDString = (char *)      FoundTag->ta_Arg;

        /* --------- Verify minimum parameters --------- */
        if ( (p) && ((CB) || (SGDArray)) && (ret >= 0) )
        {
            while (TRUE)
            {
                ret = ParseIFF(p, IFF_PARSE_RAWSTEP);
                if (ret == IFF_PARSE_EOF)
                {
                    ret = 0;
                    break;
                }
                if (ret == IFF_PARSE_EOC)
                {
                    Depth--;
                    if (!Depth)
                    {
                        ret = 0;
                        break;
                    }
                    continue;
                }
                if (!ret)
                {
                    ContextNode         *cn;

                    cn = GetCurrentContext(p);

                    Depth++;

                    switch (cn->cn_ID)
                    {
                        case ID_GINF:
                        {
                            if (PutIDString)
                            {
                                ret = ReadChunk(p, &GIData, sizeof(GameInfoChunk));
                                if (ret >= 0)
                                {
                                    strncpy(&PutIDString[0], &GIData.GameIdentifier[0], 31);
                                    PutIDString[31]=0;
                                }
                            }
                            break;
                        }
                        case ID_GDTC:
                        {
                            CompressedGameDataChunk gdc;

                            ret = ReadChunk(p, &gdc, sizeof(CompressedGameDataChunk));
                            if (ret >= 0)
                            {
                                if (CB)
                                {
                                    SGData          sgd;
                                    uint32          len;

                                    memset(&sgd, 0, sizeof(SGData));
                                    len = sgd.Length = gdc.Length;

                                    ret = (*CB)(&sgd, CBData);
                                    if (sgd.Length < len)
                                        len = sgd.Length;

                                    if (ret >= 0)
                                        ret = GameReadCompressedChunk(p, sgd.Buffer, len);
                                }
                                else
                                {
                                    if (SGDArray->Length)
                                    {
                                        ret = GameReadCompressedChunk(p, SGDArray->Buffer,
                                                                     SGDArray->Length);
                                        if (ret >= 0)
                                        {
                                            SGDArray->Actual = ret;
                                            SGDArray++;
                                        }

                                    }
                                    else
                                        ret = SG_ERR_OVERFLOW;
                                }
                            }
                            break;
                        }
                        case ID_GDTA:
                        {
                            if (CB)
                            {
                                SGData          sgd;
                                uint32          len;

                                memset(&sgd, 0, sizeof(SGData));
                                len = sgd.Length = cn->cn_Size;

                                ret = (*CB)(&sgd, CBData);
                                if (sgd.Length < len)
                                    len = sgd.Length;

                                if (ret >= 0)
                                    ret = ReadChunk(p, sgd.Buffer, len);
                            }
                            else
                            {
                                if (SGDArray->Length)
                                {
                                    ret = ReadChunk(p, SGDArray->Buffer,
                                                       SGDArray->Length);
                                    if (ret >= 0)
                                    {
                                        SGDArray->Actual = ret;
                                        SGDArray++;
                                    }

                                }
                                else
                                    ret = SG_ERR_OVERFLOW;
                            }
                            break;
                        }

                        case ID_FORM:
                        {
                            if ( (cn->cn_Type == ID_ICON) && (PutIcon) )
                            {
                                ret = LoadIconVA(PutIcon,
                                        LOADICON_TAG_IFFPARSER, p,
                                        LOADICON_TAG_IFFPARSETYPE, LOADICON_TYPE_PARSED,
                                        TAG_END);
                                if (ret >= 0)
                                    LoadedIcon = TRUE;

                                Depth--;
                            }
                            break;
                        }

                        default:
                            break;
                    }
                }
                else
                    break;
                if (ret < 0)
                    break;
            }
        }
        else
        {
            if (ret >= 0)
                ret = SG_ERR_INCOMPLETE;
        }

        /* --------- Cleanup --------- */

        /* If they gave us a filename, and an IFFParser was
         * allocated for them, free it now.
         */
        if ( (filename) && (p) )
            DeleteIFFParser(p);

        /* If we failed, but an icon was loaded, get rid of it */
        if ( (ret < 0) && (LoadedIcon) )
            UnloadIcon(*PutIcon);

        CloseIconFolio();
    }

    return(ret);
}


Err SaveGameData(char *appname, const TagArg *tags)
{
    Err              ret;
    IFFParser       *p;
    TagArg          *FoundTag;
    SGData          *SGDArray;
    SGCallBack       CB;
    void            *CBData;
    GameInfoChunk    GIData;
    char            *filename;
    char            *utffile;
    char            *idstring;
    TimeVal          tbf;

    /* --------- Preinitialization --------- */
    p           = (IFFParser *)     NULL;
    filename    =                   NULL;
    SGDArray    = (SGData *)        NULL;
    CB          = (SGCallBack)      NULL;
    CBData      = (void *)          NULL;
    idstring    =                   "Saved Game";
    utffile     =                   NULL;

    memset(&GIData, 0, sizeof(GameInfoChunk));
    memset(&tbf, 0, sizeof(TimeVal));

    ret = OpenIconFolio();
    if (ret >= 0)
    {
        /* Parse Taglists */

        /* Did they give us a filename? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_FILENAME);
        if (FoundTag)
        {
            if (FindTagArg(tags, SAVEGAMEDATA_TAG_IFFPARSER))
                ret = SG_ERR_MUTUALLYEXCLUSIVE;
            else
            {
                filename = FoundTag->ta_Arg;

                ret = CreateIFFParserVA(&p, TRUE, IFF_TAG_FILE, filename,
                                        TAG_END);
            }
        }

        /* Did they give us an IFFParser? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_IFFPARSER);
        if (FoundTag)
        {
            p = (IFFParser *)           FoundTag->ta_Arg;
        }

        /* How 'bout an array of SGData's? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_BUFFERARRAY);
        if (FoundTag)
        {
            if (FindTagArg(tags, SAVEGAMEDATA_TAG_CALLBACK))
                ret = SG_ERR_MUTUALLYEXCLUSIVE;
            else
                SGDArray = (SGData *)   FoundTag->ta_Arg;
        }

        /* A callback? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_CALLBACK);
        if (FoundTag)
        {
            CB = (SGCallBack)           FoundTag->ta_Arg;
        }

        /* CB private data? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_CALLBACKDATA);
        if (FoundTag)
        {
            if (FindTagArg(tags, SAVEGAMEDATA_TAG_CALLBACK))
            {
                CBData = (void *)       FoundTag->ta_Arg;
            }
            else
                ret = SG_ERR_INCOMPLETE;
        }

        /* Time between frames? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_TIMEBETWEENFRAMES);
        if (FoundTag)
            memcpy(&tbf, FoundTag->ta_Arg, sizeof(TimeVal));

        /* UTF filename? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_ICON);
        if (FoundTag)
            utffile  =                  FoundTag->ta_Arg;

        /* ID string? */
        FoundTag = FindTagArg(tags, SAVEGAMEDATA_TAG_IDSTRING);
        if (FoundTag)
            idstring =                  FoundTag->ta_Arg;

        /* --------- Check for minimum parameters --------- */
        if ( (ret >= 0) && (p) && ((CB) || (SGDArray)) &&
             (idstring) )
        {
            ret = PushChunk(p, ID_SGME, ID_FORM, IFF_SIZE_UNKNOWN_32);
            if (ret >= 0)
            {
                /* Write out the GINF header */
                ret = PushChunk(p, 0, ID_GINF, IFF_SIZE_UNKNOWN_32);
                if (ret >= 0)
                {
                    strncpy(&GIData.GameIdentifier[0], idstring, 31);
                    GIData.GameIdentifier[31]=0;
                    ret = WriteChunk(p, &GIData, sizeof(GameInfoChunk));
                    PopChunk(p);
                }
                /* Write out the body */

                if (CB)
                {
                    while (TRUE)
                    {
                        SGData          sgd;

                        /* Ask them for the data */
                        memset(&sgd, 0, sizeof(SGData));

                        ret = (*CB)(&sgd, CBData);
                        if (ret >= 0)
                        {
                            if (!sgd.Length)
                                break;

                            ret = GameWriteCompressedChunk(p, sgd.Buffer, sgd.Length);
                        }
                        if (ret < 0)
                            break;
                    }

                }
                else
                {
                    while (SGDArray->Length)
                    {
                        ret = GameWriteCompressedChunk(p, SGDArray->Buffer,
                                                              SGDArray->Length);
                        SGDArray++;
                    }
                }

                /* Lastly, write out the icon */
                if ((utffile) && (appname))
                {
                    ret = SaveIconVA(utffile, appname,
                            SAVEICON_TAG_IFFPARSER, p,
                            SAVEICON_TAG_TIMEBETWEENFRAMES, &tbf,
                            TAG_END);
                }
                PopChunk(p);
            }
            /* If we created the IFFParser, we delete it */
            if ((p) && (filename))
                DeleteIFFParser(p);
        }
        else
        {
            if (ret >= 0)
                ret = SG_ERR_INCOMPLETE;
        }
        CloseIconFolio();
    }

    return(ret);
}

