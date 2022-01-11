
/******************************************************************************
**
**  @(#) typeiff.c 96/02/26 1.7
**
**  Dump IFF file contents
**
******************************************************************************/

/**
|||	AUTODOC -class Shell_Commands -name TypeIFF
|||	Displays contents of an IFF file.
|||
|||	  Format
|||
|||	    TypeIFF [-short|-full] <file name>...
|||
|||	  Description
|||
|||	    This command displays the contents of the named IFF files to the debugging
|||	    terminal. By default, just the chunk names, sizes, and hierarchy are
|||	    displayed. Optional switches provide a way to display the contents of chunks
|||	    in hexadecimal.
|||
|||	  Arguments
|||
|||	    <file name>
|||	        Name of an IFF file to display.
|||
|||	    -short
|||	        Display the first 256 bytes of each chunk.
|||
|||	    -full
|||	        Display the entire contents of each chunk.
|||
|||	  Implementation
|||
|||	    Command implemented in V27.
|||
|||	  Location
|||
|||	    System.m2/Programs/TypeIFF
**/


#include <kernel/operror.h>
#include <misc/iff.h>
#include <stdio.h>
#include <string.h>


/* -------------------- Debug */

#define DEBUG_EOC 0     /* display EOC info */


/* -------------------- Macros */

#define MAX(a,b)    ((a)>(b)?(a):(b))
#define MIN(a,b)    ((a)<(b)?(a):(b))


/* -------------------- Defines */

    /* option flags */
#define OPT_F_DUMP_SHORT    0x01    /* dump up to DUMP_SHORT_MAX bytes of each chunk */
#define OPT_F_DUMP_FULL     0x02    /* dump entire contents of each chunk */

#define DUMP_SHORT_MAX      256     /* max number of bytes to dump for OPT_F_DUMP_SHORT */


/* -------------------- Code */

static void DumpIFF (const char *filename, uint8 optFlags);
static bool IsDataChunk (const ContextNode *);
static void PrintChunkID (const ContextNode *);
#if DEBUG_EOC
  static void PrintEOC (const ContextNode *);
#endif
static void PrintIndent (const ContextNode *);
static Err DumpChunk (IFFParser *, ContextNode *, uint64 dumpLimit);
static void DumpRecord (uint64 offset, const uint8 *buf, uint32 len);
static void DumpLine (uint64 offset, const uint8 *buf, uint32 len);

int main (int argc, char *argv[])
{
    Err errcode;
    uint8 optFlags = 0;

    if (argc < 2) {
        printf ("usage: %s [-short|-full] <file name>...\n", argv[0]);
        return 0;
    }

        /* open folios */
    if ((errcode = OpenIFFFolio()) < 0) goto clean;

        /* scan switches */
    {
        int i;

        for (i=1; i<argc; i++) {
            const char * const arg = argv[i];

            if (arg[0] == '-') {
                if (!strcasecmp (arg, "-short")) optFlags |= OPT_F_DUMP_SHORT;
                else if (!strcasecmp (arg, "-full")) optFlags |= OPT_F_DUMP_FULL;
                else printf ("%s: Unknown switch '%s'\n", argv[0], arg);
            }
        }
    }

        /* process files */
    {
        int i;

        for (i=1; i<argc; i++) {
            const char * const arg = argv[i];

            if (arg[0] != '-') {
                DumpIFF (arg, optFlags);
            }
        }
    }

        /* success */
    errcode = 0;

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);
    CloseIFFFolio();
    return 0;
}

static void DumpIFF (const char *filename, uint8 optFlags)
{
    IFFParser *iff = NULL;
    Err errcode;

    printf ("\n%s:\n", filename);

    if ((errcode = CreateIFFParserVA (&iff, FALSE, IFF_TAG_FILE, filename, TAG_NOP, 0, TAG_END)) < 0) goto fail;

    for (;;) {
        /*
         * Return code:         Reason:
         * 0                    Entered new context.
         * IFF_PARSE_EOC        About to leave a context.
         * IFF_PARSE_EOF        Encountered end-of-file.
         * <anything else>      A parsing error.
         */

        switch (errcode = ParseIFF (iff, IFF_PARSE_RAWSTEP)) {
            case IFF_PARSE_EOC:
                 #if DEBUG_EOC
                    {
                        const ContextNode * const top = GetCurrentContext (iff);

                        if (top) PrintEOC (top);
                    }
                 #endif
                    break;

            case IFF_PARSE_EOF:
                    printf ("End of file.\n");
                    goto done;

            case 0:
                    {
                        ContextNode * const top = GetCurrentContext (iff);

                        if (top) {
                            PrintChunkID (top);
                            if (optFlags & (OPT_F_DUMP_SHORT | OPT_F_DUMP_FULL) && IsDataChunk (top)) {
                                if ((errcode = DumpChunk (iff, top, optFlags & OPT_F_DUMP_FULL ? top->cn_Size : MIN (top->cn_Size, DUMP_SHORT_MAX))) < 0) goto fail;
                            }
                        }
                    }
                    break;

            default:
                    goto fail;
        }
    }

done:
    errcode = 0;

fail:
    if (errcode < 0) PrintError (NULL, "dump", filename, errcode);
    DeleteIFFParser (iff);
}

static bool IsDataChunk (const ContextNode *top)
{
    return top->cn_ID != ID_FORM &&
           top->cn_ID != ID_LIST &&
           top->cn_ID != ID_CAT &&
           top->cn_ID != ID_PROP;
}

static void PrintChunkID (const ContextNode *top)
{
    PrintIndent (top);
    printf ("%.4s 0x%Lx (%Lu) %.4s\n", &top->cn_ID, top->cn_Size, top->cn_Size, &top->cn_Type);
}

#if DEBUG_EOC
static void PrintEOC (const ContextNode *top)
{
    PrintIndent (top);
    printf ("End of %.4s %.4s %Lu context.", &top->cn_ID, &top->cn_Type, top->cn_Offset);
}
#endif

static void PrintIndent (const ContextNode *cn)
{
    while (cn = GetParentContext(cn)) printf (". ");
}

static Err DumpChunk (IFFParser *iff, ContextNode *cn, uint64 dumpLimit)
{
    uint8 buf[256];
    int32 len;

    for (;;) {
        const uint64 chunkOffset = cn->cn_Offset;

        if ((len = ReadChunk (iff, buf, MIN (sizeof buf, (dumpLimit - chunkOffset)))) <= 0) break;
        DumpRecord (chunkOffset, buf, len);
    }

    if (cn->cn_Offset < cn->cn_Size) printf ("    ...\n");

    return len;
}

#define BYTESPERLINE 16

static void DumpRecord (uint64 offset, const uint8 *buf, uint32 len)
{
    uint32 linebytes;

    while (len) {
        DumpLine (offset, buf, linebytes = MIN (BYTESPERLINE, len));
        offset += linebytes;
        buf    += linebytes;
        len    -= linebytes;
    }
}

/* @@@ len must be <= BYTESPERLINE */
static void DumpLine (uint64 offset, const uint8 *buf, uint32 len)
{
    uint32 i;
    char asciibuf[BYTESPERLINE+1];

    printf ("    %04Lx:", offset);

    for (i=0; i<len; i++) {
        const uint8 c = buf[i];

        if (!(i & 3)) printf (" ");
        printf ("%02x", c);
        asciibuf[i] = (c >= 0x20 && c <= 0x7e) ? c : '-';
    }
    asciibuf[len] = '\0';
    printf ("%*s%s\n", (BYTESPERLINE - len) * 2 + (BYTESPERLINE - len) / 4 + 1, "", asciibuf);
}
