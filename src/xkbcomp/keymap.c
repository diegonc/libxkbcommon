/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include "xkbcomp.h"
#include "xkbmisc.h"
#include "tokens.h"
#include "expr.h"
#include "vmod.h"
#include "action.h"
#include "misc.h"
#include "indicators.h"

#define	KEYCODES	0
#define	GEOMETRY	1
#define	TYPES		2
#define	COMPAT		3
#define	SYMBOLS		4
#define	MAX_SECTIONS	5

static XkbFile *sections[MAX_SECTIONS];

/**
 * Compile the given file and store the output in xkb.
 * @param file A list of XkbFiles, each denoting one type (e.g.
 * XkmKeyNamesIdx, etc.)
 */
Bool
CompileKeymap(XkbFile *file, struct xkb_desc * xkb, unsigned merge)
{
    unsigned have;
    Bool ok;
    unsigned required, legal;
    unsigned mainType;
    char *mainName;
    LEDInfo *unbound = NULL;

    bzero(sections, MAX_SECTIONS * sizeof(XkbFile *));
    mainType = file->type;
    mainName = file->name;
    switch (mainType)
    {
    case XkmSemanticsFile:
        required = XkmSemanticsRequired;
        legal = XkmSemanticsLegal;
        break;
    case XkmLayoutFile:        /* standard type  if setxkbmap -print */
        required = XkmLayoutRequired;
        legal = XkmKeymapLegal;
        break;
    case XkmKeymapFile:
        required = XkmKeymapRequired;
        legal = XkmKeymapLegal;
        break;
    default:
        ERROR("Cannot compile %s alone into an XKM file\n",
               XkbcConfigText(mainType));
        return False;
    }
    have = 0;
    ok = 1;
    file = (XkbFile *) file->defs;
    /* Check for duplicate entries in the input file */
    while ((file) && (ok))
    {
        file->topName = mainName;
        if ((have & (1 << file->type)) != 0)
        {
            ERROR("More than one %s section in a %s file\n",
                   XkbcConfigText(file->type), XkbcConfigText(mainType));
            ACTION("All sections after the first ignored\n");
            ok = False;
        }
        else if ((1 << file->type) & (~legal))
        {
            ERROR("Cannot define %s in a %s file\n",
                   XkbcConfigText(file->type), XkbcConfigText(mainType));
            ok = False;
        }
        else
            switch (file->type)
            {
            case XkmSemanticsFile:
            case XkmLayoutFile:
            case XkmKeymapFile:
                WSGO("Illegal %s configuration in a %s file\n",
                      XkbcConfigText(file->type), XkbcConfigText(mainType));
                ACTION("Ignored\n");
                ok = False;
                break;
            case XkmKeyNamesIndex:
                sections[KEYCODES] = file;
                break;
            case XkmTypesIndex:
                sections[TYPES] = file;
                break;
            case XkmSymbolsIndex:
                sections[SYMBOLS] = file;
                break;
            case XkmCompatMapIndex:
                sections[COMPAT] = file;
                break;
            case XkmGeometryIndex:
            case XkmGeometryFile:
                sections[GEOMETRY] = file;
                break;
            case XkmVirtualModsIndex:
            case XkmIndicatorsIndex:
                WSGO("Found an isolated %s section\n",
                      XkbcConfigText(file->type));
                break;
            default:
                WSGO("Unknown file type %d\n", file->type);
                break;
            }
        if (ok)
            have |= (1 << file->type);
        file = (XkbFile *) file->common.next;
    }
    /* compile the sections we have in the file one-by-one, or fail. */
    if (ok)
    {
        if (ok && (sections[KEYCODES] != NULL))
            ok = CompileKeycodes(sections[KEYCODES], xkb, MergeOverride);
        if (ok && (sections[GEOMETRY] != NULL))
            ok = CompileGeometry(sections[GEOMETRY], xkb, MergeOverride);
        if (ok && (sections[TYPES] != NULL))
            ok = CompileKeyTypes(sections[TYPES], xkb, MergeOverride);
        if (ok && (sections[COMPAT] != NULL))
            ok = CompileCompatMap(sections[COMPAT], xkb, MergeOverride,
                                  &unbound);
        if (ok && (sections[SYMBOLS] != NULL))
            ok = CompileSymbols(sections[SYMBOLS], xkb, MergeOverride);
    }
    if (!ok)
        return False;
    xkb->defined = have;
    if (required & (~have))
    {
        register int i, bit;
        unsigned missing;
        missing = required & (~have);
        for (i = 0, bit = 1; missing != 0; i++, bit <<= 1)
        {
            if (missing & bit)
            {
                ERROR("Missing %s section in a %s file\n",
                       XkbcConfigText(i), XkbcConfigText(mainType));
                missing &= ~bit;
            }
        }
        ACTION("Description of %s not compiled\n",
                XkbcConfigText(mainType));
        ok = False;
    }
    ok = BindIndicators(xkb, True, unbound, NULL);
    return ok;
}
