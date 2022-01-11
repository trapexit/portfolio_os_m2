/* @(#) fontitem.c 96/07/09 1.22 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/usermodeservices.h>
#include <kernel/kernel.h>
#include <kernel/time.h>
#include <file/fileio.h>
#include <file/filefunctions.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include <stdio.h>
#include <string.h>
#include "font_folio.h"
#include "fontfile.h"
#include "fontitem.h"

#define DPRT(x) /*printf x*/
#define DPRTF(x) /*printf x*/

/****************************************************************************/

/**
|||	AUTODOC -class Font -name OpenFont
|||	Loads a 3DO font file.
|||
|||	  Synopsis
|||
|||	    Item OpenFont(const char *fontName);
|||
|||	  Description
|||
|||	    Loads the named font into memory and prepares it for use.
|||	    Use CloseFont() to release all resources acquired by OpenFont().
|||
|||	    The fontName argument is used to determine the filename of the
|||	    font file. The routine first tries to load
|||	    $fonts/<fontName>.font. If that fails, it tries to load
|||	    $app/fonts/<fontName>.font.
|||
|||	    You can override the font search path by supplying an absolute
|||	    pathname (one starting with a /). In such a case, the exact
|||	    supplied path is used without any modifications.
|||
|||	    The system comes with one standard font which can be used
|||	    in production codes, but is mostly meant for use during
|||	    development and debugging. You can use this font by doing
|||	    OpenFont("default_14").
|||
|||	  Arguments
|||
|||	    fileName
|||	        Font to load.
|||
|||	  Return Value
|||
|||	    The item number of the loaded font, or a negative error code
|||	    for failure.
|||
|||	  Example
|||
|||	    // This shows how to use an absolute path to load an explicit
|||	    // font file.
|||	    {
|||	    char path[FILESYSTEM_MAX_PATH_LEN];
|||	    Item font;
|||
|||	        // get an absolute path to the current directory
|||	        GetDirectory(path,sizeof(path));
|||
|||	        // append the name of the font file we want
|||	        strncat(path,"MyFont",sizeof(path));
|||
|||	        // make sure it's NUL-terminated
|||	        path[sizeof(path)-1] = 0;
|||
|||	        // open the darn thing
|||	        font = OpenFont(path);
|||	    }
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V27.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CloseFont()
|||
**/

/**
|||	AUTODOC -class Font -name CloseFont
|||	Releases font resources.
|||
|||	  Synopsis
|||
|||	    void CloseFont(Item font);
|||
|||	  Description
|||
|||	    Releases resources acquired during OpenFont() processing. If the
|||	    font was loaded via OpenFont(), releases the FontDescriptor and
|||	    unloads the file image, releasing the memory it occupied.
|||
|||	    Because TextSprite structures contain a reference to a
|||	    FontDescriptor, you must delete all TextSprites that reference
|||	    a FontDescriptor before you call CloseFont() for that descriptor.
|||
|||	  Arguments
|||
|||	    font
|||	        Item number of the font to close.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V27.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    OpenFont()
|||
**/

#define CREATEFONT_TAG_TEMPLATE TAG_ITEM_LAST+1

#define AddToPtr(ptr, val) ((void*)((((char *)(ptr)) + (long)(val))))
/* Sub this from that: */
#define SubFromPtr(this, that) ((uint32)((((long)(this)) - (long)(that))))


/****************************************************************************/

int32 GetFontCharInfo(const FontDescriptor *fd, int32 theChar, void **blitInfo)
{
    FontCharInfo *	fci;
    uint32			firstChar;
    int32			width = 0;

    fci = (FontCharInfo *)fd->fd_charInfo;
    firstChar = fd->fd_firstChar;

    if (theChar >= firstChar && theChar <= fd->fd_lastChar)
    {
        fci   += theChar - firstChar;
        width  = fci->fci_charWidth;
    }

    if (blitInfo)
    {
        *blitInfo = (void *)fci;
    }

    return width;
}

/****************************************************************************/

#define TRAM_SIZE (16 * 1024)
#define TRAM_WIDTH (1024)

typedef struct
{
    char *umd_FontName;
    Item  umd_Client;
} UserModeData;


/****************************************************************************/


static Err ParseFont(FontDescriptor *fd, void *fontImage)
{
    FontHeader *fh;
    uint32      formSize;
    uint32      chunkSize;
    uint32      charCount;
    uint32      charsWidth;

    fh = (FontHeader *)fontImage;

        /* Subtract the form header size. */
    formSize = GetMemTrackSize(fontImage) - 8;

        /* Subtract the form header, the chunkSize and chunkId field sizes. */
    chunkSize = formSize - 12;

    if (fh->form.label != FORM_LABEL
        || fh->form.size > formSize
        || fh->form.id != CHUNK_FONT
        || fh->chunkID != CHUNK_FONT
        || fh->chunkSize > chunkSize )
    {
        return FONT_ERR_BADFONTFILE;
    }

    if (fh->chunkVersion != FONT_VERSION)
        return FONT_ERR_BADVERSION;

    if (fh->fontDesc.fontBitsPerPixel != 4)
        return FONT_ERR_BADBPP;

    fd->fd.n_Version        = fh->fontVersion;
    fd->fd.n_Revision       = fh->fontRevision;
    fd->fd_fontFlags        = fh->fontDesc.fontFlags;
    fd->fd_bitsPerPixel     = fh->fontDesc.fontBitsPerPixel;
    fd->fd_firstChar        = fh->fontDesc.fontFirstChar;
    fd->fd_lastChar         = fh->fontDesc.fontLastChar;
    fd->fd_ascent           = fh->fontDesc.fontAscent;
    fd->fd_descent          = fh->fontDesc.fontDescent;
    fd->fd_charExtra        = fh->fontDesc.fontSpacing;
    fd->fd_leading          = fh->fontDesc.fontLeading;
    fd->fd_bytesPerRow      = fh->charRowBytes;
    fd->fd_charWidth        = fh->charWidth;
    fd->fd_charHeight       = fh->charHeight;
    fd->fd_fontHeader       = fh;
    fd->fd_charInfo         = AddToPtr(fh, fh->charWTableOffset);
    fd->fd_charData         = AddToPtr(fh, fh->charDataOffset);

    /* How many characters can we fit into the TRAM? */
    charCount = (TRAM_SIZE / (fd->fd_bytesPerRow * fd->fd_charHeight));
    charsWidth = (fd->fd_bytesPerRow * charCount * (8 / fd->fd_bitsPerPixel));
    DPRT(("Can fit %ld characters in TRAM, %ld pixels wide.\n", charCount, charsWidth));
    if (charsWidth > TRAM_WIDTH)
    {
        charCount = (TRAM_WIDTH / (fd->fd_bytesPerRow * (8 / fd->fd_bitsPerPixel)));
        DPRT(("reduce that to fit in %ld characters\n", charCount));
    }
    fd->fd_maxCharsLoad = charCount;
    fd->fd_rangeGap = 4;    /* Just a heuristic */

    DPRTF(("Font: BitsPerPixel = %ld\n", fd->fd_bitsPerPixel));
    DPRTF(("Font: Char range %d - %d (%c - %c)\n", fd->fd_firstChar, fd->fd_lastChar, fd->fd_firstChar, fd->fd_lastChar));
    DPRTF(("Font: ascent = %d, descent = %d, extra = %d, leading = %d\n", fd->fd_ascent, fd->fd_descent, fd->fd_charExtra, fd->fd_leading));
    DPRTF(("Font: BytesPerRow = %d\n", fd->fd_bytesPerRow));
    DPRTF(("Font: charWidth = %d, charHeight = %d\n", fd->fd_charWidth, fd->fd_charHeight));
    DPRTF(("Font: data @ 0x%lx\n", fd->fd_charInfo));

    return 0;
}


/****************************************************************************/


static const TagArg searchForFile[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

/* This runs in user mode, on the context of an item server thread spawned
 * by the operator.
 */
static Item LoadFont(const UserModeData *umd)
{
    Item            result;
    void           *fontImage;
    RawFile        *file;
    FileInfo        fontFileInfo;
    FontDescriptor  fd;
    Err             err;
    char           *fontName;

    result = ChangeDirectoryInDir(umd->umd_Client, "");
    if (result < 0)
        return result;

    fontName = umd->umd_FontName;
    DPRT(("Loading font %s\n", fontName));

    LockSemaphore(FontBase->ff_FontLock->s.n_Item, SEM_WAIT);

    result = OpenRawFile(&file, fontName, FILEOPEN_READ);
    if (result < 0)
    {
    char fullPath[90];
    char partialPath[60];

        sprintf(partialPath, "System.m2/Fonts/%.26s.font", fontName);

        result = FindFileAndIdentify(fullPath, sizeof(fullPath), partialPath, searchForFile);
        if (result >= 0)
            result = OpenRawFile(&file, fullPath, FILEOPEN_READ);
    }

    if (result >= 0)
    {
        DPRT(("Opened font %s\n", fontName));

        result = GetRawFileInfo(file, &fontFileInfo, sizeof(fontFileInfo));
        if (result >= 0)
        {
            fontImage = AllocMem(fontFileInfo.fi_ByteCount, MEMTYPE_TRACKSIZE);
            if (fontImage)
            {
                result = ReadRawFile(file, fontImage, fontFileInfo.fi_ByteCount);
		DPRT(("Read font %s\n", fontName));
                if (result == fontFileInfo.fi_ByteCount)
                {
                    result = ParseFont(&fd, fontImage);
                    if (result >= 0)
                    {
		        DPRT(("Parsed font %s\n", fontName));
                        result = CreateItemVA(MKNODEID(NST_FONT,FONT_FONT_NODE),
                                              TAG_ITEM_UNIQUE_NAME,    TRUE,
                                              TAG_ITEM_NAME,           fontName,
                                              TAG_ITEM_VERSION,        fd.fd.n_Version,
                                              TAG_ITEM_REVISION,       fd.fd.n_Revision,
                                              CREATEFONT_TAG_TEMPLATE, &fd,
                                              TAG_END);
                        if (result >= 0)
                        {
			    DPRT(("Created font item\n"));
                            err = SetItemOwner(result, FontBase->ff.fn.n_Owner);
                            if (err < 0)
                            {
                                DeleteItem(result);
                                result = err;
                            }
                        }

                    }
                }
                else if (result >= 0)
                {
                    result = FONT_ERR_BADFONTFILE;
                }

                if (result < 0)
                    FreeMem(fontImage, -1);
            }
            else
            {
                result = FONT_ERR_NOMEM;
            }
        }
        CloseRawFile(file);
    }

    UnlockSemaphore(FontBase->ff_FontLock->s.n_Item);
    DPRT(("Loaded font %s\n", fontName));

    return result;
}


/****************************************************************************/


static int32 TagCallBack(FontDescriptor *fd, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    switch (tag)
    {
        case CREATEFONT_TAG_TEMPLATE: fd->fd_fontHeader = (void *)arg;
                                      break;

        default                     : return FONT_ERR_BADTAG;
    }

    return 0;
}


/****************************************************************************/


Item CreateFontItem(FontDescriptor *fd, TagArg *args)
{
    Item            result;
    FontDescriptor *template;

    if (!IsPriv(CURRENTTASK))
        return FONT_ERR_BADPRIV;

    result = TagProcessor(fd, args, TagCallBack, 0);
    if (result >= 0)
    {
        template = (FontDescriptor *)fd->fd_fontHeader;
        template->fd = fd->fd;
        *fd = *template;

        SuperInternalLockSemaphore(FontBase->ff_FontLock, SEM_WAIT);
    	AddTail(&FontBase->ff_Fonts,(Node *)fd);
        SuperInternalUnlockSemaphore(FontBase->ff_FontLock);

    	result = fd->fd.n_Item;
    }

    return result;
}


/****************************************************************************/


Err DeleteFontItem(FontDescriptor *font)
{
    SuperInternalLockSemaphore(FontBase->ff_FontLock, SEM_WAIT);
    RemNode((Node *)font);
    SuperFreeUserMem(font->fd_fontHeader, -1, TASK(FontBase->ff.fn.n_Owner));
    SuperInternalUnlockSemaphore(FontBase->ff_FontLock);

    return 0;
}


/****************************************************************************/


Err CloseFontItem(FontDescriptor *font)
{
    if (font->fd.n_OpenCount == 0)
        SuperInternalDeleteItem(font->fd.n_Item);

    return 0;
}


/****************************************************************************/


Item FindFontItem(TagArg *tags)
{
    ItemNode  inode;
    ItemNode *it;
    Item      result;

    memset(&inode,0,sizeof(inode));

    result = TagProcessorNoAlloc(&inode,tags,NULL,0);
    if (result < 0)
        return result;

    if (inode.n_Name == NULL)
        return FONT_ERR_NOTFOUND;

    SuperInternalLockSemaphore(FontBase->ff_FontLock, SEM_WAIT);
    it = (ItemNode *)FindNamedNode(&FontBase->ff_Fonts, inode.n_Name);
    SuperInternalUnlockSemaphore(FontBase->ff_FontLock);

    if (it == NULL)
        return FONT_ERR_NOTFOUND;

    return it->n_Item;
}


/*****************************************************************************/


Item LoadFontItem(TagArg *tags)
{
    ItemNode inode;
    Err      result;
    uint8    oldPriv;
    UserModeData umd;

    memset(&inode,0,sizeof(inode));

    result = TagProcessorNoAlloc(&inode,tags,NULL,0);
    if (result < 0)
        return result;

    if (inode.n_Name == NULL)
        return FONT_ERR_NOTFOUND;

    umd.umd_FontName = inode.n_Name;
    umd.umd_Client   = CURRENTTASKITEM;

    oldPriv = PromotePriv(CURRENTTASK);
    result = (Item)CallAsItemServer(LoadFont, &umd, TRUE);
    DemotePriv(CURRENTTASK,oldPriv);

    return result;
}
