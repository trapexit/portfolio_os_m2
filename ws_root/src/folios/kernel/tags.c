/* @(#) tags.c 96/08/27 1.23
/* $Id: tags.c,v 1.21 1994/11/16 19:44:27 sdas Exp $ */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/tags.h>
#include <kernel/internalf.h>
#include <stdio.h>          /* printf() */


/*****************************************************************************/


typedef int32 (*CallBackFunc)(void *, void *, uint32, void *);


static int32 internalTagProcessor(void *np, TagArg *tagList,
                                  CallBackFunc callback, void *dataP, bool cloneName)
{
ItemNode *n;
int       numJumps;
int32     result;
uint32    arg;

    if (tagList == NULL)
        return 0;

    if (!IsMemReadable(tagList,4))
        return BADPTR;

    n        = (ItemNode *)np;
    result   = 0;
    numJumps = 0;
    while (tagList->ta_Tag != TAG_END)
    {
        arg = (uint32)tagList->ta_Arg;
        switch (tagList->ta_Tag)
        {
            case TAG_ITEM_PRI        : n->n_Priority = (uint8)arg;
                                       break;

            case TAG_ITEM_NAME       : if (!IsLegalName((char *)arg))
                                       {
                                           result = BADNAME;
                                       }
                                       else if (cloneName)
                                       {
                                           n->n_Name = AllocateString((char *)arg);
                                           if (!n->n_Name)
                                           {
                                               result = NOMEM;
                                           }
                                       }
                                       else
                                       {
                                           n->n_Name = (char *)arg;
                                       }
                                       break;

            case TAG_ITEM_UNIQUE_NAME: n->n_ItemFlags |= ITEMNODE_UNIQUE_NAME;
                                       break;

            case TAG_ITEM_VERSION    : n->n_Version = (uint8)arg;
                                       break;

            case TAG_ITEM_REVISION   : n->n_Revision = (uint8)arg;
                                       break;

            case TAG_JUMP            : /* simple loop prevention */
                                       if (numJumps++ <= 20)
                                       {
                                           tagList = (TagArg *)arg;
                                           if (!tagList)
                                           {
                                               goto done;
                                           }
                                           else if (IsMemReadable(tagList,4))
                                           {
                                               continue;
                                           }
                                       }
                                       result = BADPTR;
                                       break;

            case TAG_NOP             : break;

            default                  : if (callback)
                                           result = (*callback)(n,dataP,tagList->ta_Tag,tagList->ta_Arg);
                                       else
                                           result = BADTAG;
                                       break;
        }

        if (result < 0)
            break;

        tagList++;
    }

done:

    if (result < 0)
    {
        if (cloneName)
            FreeString(n->n_Name);

        n->n_Name = NULL;
    }

    return result;
}


/*****************************************************************************/


int32 TagProcessor(void *np, TagArg *tagpt, int32 (*callback)(),void *dataP)
{
    return internalTagProcessor(np,tagpt,(CallBackFunc)callback,dataP,TRUE);
}


/*****************************************************************************/


int32 TagProcessorNoAlloc(void *np, TagArg *tagpt, int32 (*callback)(), void *dataP)
{
    return internalTagProcessor(np,tagpt,(CallBackFunc)callback,dataP,FALSE);
}


/*****************************************************************************/


int32 TagProcessorSearch(TagArg *tagReturn, TagArg *tagList, uint32 targetTag)
{
int   numJumps;
int32 result;

    if (tagList == NULL)
        return 0;

    if (!IsMemReadable(tagList,4))
        return BADPTR;

    result   = 0;
    numJumps = 0;
    while (tagList->ta_Tag != TAG_END)
    {
        if (tagList->ta_Tag == targetTag)
        {
            result            = 1;
            tagReturn->ta_Tag = tagList->ta_Tag;
            tagReturn->ta_Arg = tagList->ta_Arg;

            /* Note that we don't return immediately since we want to
             * find the last instance of this tag in the tag list.
             */
        }
        else if (tagList->ta_Tag == TAG_JUMP)
        {
            /* simple loop prevention */
            if (numJumps++ <= 20)
            {
                tagList = (TagArg *)tagList->ta_Arg;
                if (!tagList)
                {
                    goto done;
                }
                else if (IsMemReadable(tagList,4))
                {
                    continue;
                }
            }

            result = BADPTR;
            break;
        }

        tagList++;
    }

done:

    return result;
}


/*****************************************************************************/

static int32 SafeScanNextTagArg (const TagArg **tagp, const TagArg *t);

/**
|||	AUTODOC -private -class Kernel -group Tags -name SafeFirstTagArg
|||	Finds the first TagArg in a tag list with memory protection.
|||
|||	  Description
|||
|||	    This function finds the first TagArg in a tag list, skipping and chaining as
|||	    dictated by control tags. There are three control tags:
|||
|||	    TAG_NOP
|||	        Ignores that single entry and moves to the next one.
|||
|||	    TAG_JUMP
|||	        Has a pointer to another array of TagArgs.
|||
|||	    TAG_END
|||	        Marks the end of the tag list.
|||
|||	    This function only returns TagArgs which are not control tags. It either
|||	    returns a positive value and stores a pointer to the first non-control
|||	    TagArg in *tagp, zero to indicate an empty tag list, or a negative error
|||	    code.
|||
|||	  Arguments
|||
|||	    tagp
|||	        Pointer to variable to write pointer of first TagArg. tagp is assumed
|||	        to be a valid pointer.
|||
|||	    tagList
|||	        Tag list to begin scanning. May be NULL or empty. This pointer
|||	        and TAG_JUMP pointers are validated by this function.
|||
|||	  Return Value
|||
|||	    > 0
|||	        Found first TagArg in tag list. Pointer to that TagArg is stored in
|||	        *tagp.
|||
|||	    0
|||	        Tag list is empty. *tagp is not written to.
|||
|||	    < 0
|||	        An error occurred (e.g., invalid pointer). *tagp is not written to.
|||
|||	  Example
|||
|||	    // Note: SafeFirstTagArg() will catch tagList==NULL or an invalid pointer,
|||	    // so no need to validate tagList beforehand.
|||
|||	    void WalkTagList(const TagArg *tagList)
|||	    {
|||	    const TagArg *tag;
|||	    int32 result;
|||
|||	        // Loop until result <= 0 (end of list or error).
|||	        for ( result = SafeFirstTagArg (&tag, tagList);
|||	              result > 0;
|||	              result = SafeNextTagArg (&tag) )
|||	        {
|||	            switch (tag->ta_Tag)
|||	            {
|||	                case TAG1: // process this tag
|||	                           break;
|||
|||	                case TAG2: // process this tag
|||	                           break;
|||
|||	                default  : // unknown tag, return an error
|||	                           break;
|||	            }
|||	        }
|||	        if (result < 0) {
|||	            // Handle error returned from SafeFirstTagArg() or SafeNextTagArg(). The
|||	            // error code is stored in result.
|||	        }
|||	    }
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>
|||
|||	  See Also
|||
|||	    SafeNextTagArg(), TagProcessor(), NextTagArg()
**/

int32 SafeFirstTagArg (const TagArg **tagp, const TagArg *tagList)
{
        /* Since we have to check for non-NULL before calling IsMemReadable()
        ** anyway, might as well have a fast exit for NULL */
    if (!tagList) return 0;

        /* Validate first TagArg in tagList */
    if (!IsMemReadable (tagList, sizeof (TagArg))) return BADPTR;

        /* Scan starting with first tag for a non-control tag */
    return SafeScanNextTagArg (tagp, tagList);
}


/**
|||	AUTODOC -private -class Kernel -group Tags -name SafeNextTagArg
|||	Finds the next TagArg in a tag list with memory protection.
|||
|||	  Synposis
|||
|||	    int32 SafeNextTagArg(const TagArg **tagp);
|||
|||	  Description
|||
|||	    This function finds the next TagArg in a tag list, skipping and chaining as
|||	    dictated by control tags. Use it to iterate through the remaining TagArgs
|||	    after calling SafeFirstTagArg().
|||
|||	    This function only returns TagArgs which are not control tags. It either
|||	    returns a positive value and stores a pointer to the next non-control
|||	    TagArg in *tagp, zero to indicate no more TagArgs in the tag list, or a
|||	    negative error code.
|||
|||	  Arguments
|||
|||	    tagp
|||	        Pointer to TagArg variable previously passed to SafeFirstTagArg() or
|||	        SafeNextTagArg(). *tagp should contain a pointer to the last
|||	        non-control TagArg encountered. The pointer to the next non-control
|||	        TagArg will be written to *tagp.
|||
|||	        tagp is assumed to be a valid pointer. TAG_JUMP pointers are validated
|||	        by this function.
|||
|||	  Return Value
|||
|||	    > 0
|||	        Found next TagArg. Pointer to that TagArg is stored in *tagp.
|||
|||	    0
|||	        No more TagArgs. *tagp is not written to.
|||
|||	    < 0
|||	        An error occurred (e.g., invalid pointer). *tagp is not written to.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>
|||
|||	  See Also
|||
|||	    SafeFirstTagArg(), NextTagArg()
**/

int32 SafeNextTagArg (const TagArg **tagp)
{
        /* scan starting with next tag for a non-control tag */
    return SafeScanNextTagArg (tagp, *tagp+1);
}


/**
|||	AUTODOC -public -class Kernel -group Tags -name NextTagArg
|||	Finds the next TagArg in a tag list.
|||
|||	  Synopsis
|||
|||	    TagArg *NextTagArg(const TagArg **tagList);
|||
|||	  Description
|||
|||	    This function iterates through a tag list, skipping and chaining as
|||	    dictated by control tags. There are three control tags:
|||
|||	    TAG_NOP
|||	        Ignores that single entry and moves to the next one.
|||
|||	    TAG_JUMP
|||	        Has a pointer to another array of tags.
|||
|||	    TAG_END
|||	        Marks the end of the tag list.
|||
|||	    This function only returns TagArgs which are not system tags. Each
|||	    call returns either the next TagArg you should examine, or NULL
|||	    when the end of the list has been reached.
|||
|||	  Arguments
|||
|||	    tagList
|||	        This is a pointer to a storage location used by the iterator
|||	        to keep track of its current location in the tag list. The
|||	        variable that this parameter points to should be initialized
|||	        to point to the first TagArg in the tag list, and should not be
|||	        changed thereafter.
|||
|||	  Return Value
|||
|||	    Returns a pointer to a TagArg structure, or NULL if all the tags
|||	    have been visited. None of the control tags are ever returned to
|||	    you, they are handled transparently by this function.
|||
|||	  Example
|||
|||	    void WalkTagList(const TagArg *tags)
|||	    {
|||	    TagArg *state;
|||	    TagArg *tag;
|||
|||	        state = tags;
|||	        while ((tag = NextTagArg(&state)) != NULL)
|||	        {
|||	            switch (tag->ta_Tag)
|||	            {
|||	                case TAG1: // process this tag
|||	                           break;
|||
|||	                case TAG2: // process this tag
|||	                           break;
|||
|||	                default  : // unknown tag, return an error
|||	                           break;
|||	            }
|||	        }
|||	    }
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>, libc.a
|||
|||	  See Also
|||
|||	    FindTagArg()
|||
**/

TagArg *NextTagArg (const TagArg **tagp)
{
    const TagArg *t;
    int32 result;

        /* Mimic original behavior of NextTagArg(): scan for non-control tag
        ** starting with supplied tag. Return pointer to found tag or NULL.
        ** If we found a tag, set *tagp to point to next tag. */
    if ((result = SafeScanNextTagArg (&t, *tagp)) > 0) {
        *tagp = t+1;
        return t;
    }
#ifdef BUILD_STRINGS
    else if (result < 0) {
        printf ("WARNING: NextTagArg() unable to walk TagArg list near 0x%x\n", *tagp);
        printf ("         "); PrintfSysErr (result);
    }
#else
    TOUCH(result);
#endif

    return NULL;
}


/*
    Iterates through TagArg list with memory validation at TAG_JUMPs. Returns
    pointer to next non-control TagArg.

    Arguments
        tagp
            Pointer to variable to write pointer of next TagArg.

        t
            First tag to consider in iteration. May be a control tag,
            a non-control tag, or NULL.

    Results
        >0
            Found next TagArg. Pointer to that TagArg is stored in *tagp.

        0
            No more TagArgs. *tagp is not written to.

        < 0
            An error occurred (e.g., invalid pointer). *tagp is not written to.
*/
static int32 SafeScanNextTagArg (const TagArg **tagp, const TagArg *t)
{
    while (t) {
        switch (t->ta_Tag) {
            case TAG_END:
                t = NULL;
                break;

            case TAG_NOP:
                t++;
                break;

            case TAG_JUMP:      /* Note: { TAG_JUMP, NULL } == TAG_END */
                t = (TagArg *)t->ta_Arg;
                if (t && !IsMemReadable (t, sizeof (TagArg))) return BADPTR;
                break;

            default:
                *tagp = t;
                return 1;
        }
    }
    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Tags -name FindTagArg
|||	Looks through a tag list for a specific tag.
|||
|||	  Synopsis
|||
|||	    TagArg *FindTagArg(const TagArg *tagList, uint32 tag);
|||
|||	  Description
|||
|||	    This function scans a tag list looking for a TagArg structure with
|||	    a ta_Tag field equal to the tag parameter. The function always
|||	    returns the last TagArg structure in the list which matches the tag.
|||	    Finally, this function deals with the various control tags such as
|||	    TAG_JUMP and TAG_NOP.
|||
|||	  Arguments
|||
|||	    tagList
|||	        The list of tags to scan.
|||
|||	    tag
|||	        The value to look for.
|||
|||	  Return Value
|||
|||	    Returns a pointer to a TagArg structure with a value of ta_Tag that
|||	    matches the tag parameter, or NULL if no match can be found. The
|||	    function always returns the last tag in the list which matches.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>, libc.a
|||
|||	  See Also
|||
|||	    NextTagArg()
|||
**/

TagArg *FindTagArg(const TagArg *tagList, uint32 tag)
{
TagArg *ta;
TagArg *result;

    result = NULL;
    while ((ta = NextTagArg(&tagList)) != NULL)
    {
        if (ta->ta_Tag == tag)
            result = ta;
    }

    return result;
}

