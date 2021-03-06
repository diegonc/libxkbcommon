/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xkballoc.h"
#include "xkbmisc.h"
#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"
#include <X11/keysym.h>

#define mapSize(m) (sizeof(m) / sizeof(struct xkb_kt_map_entry))
static struct xkb_kt_map_entry map2Level[]= {
    { True, ShiftMask, {1, ShiftMask, 0} }
};

static struct xkb_kt_map_entry mapAlpha[]= {
    { True, ShiftMask, { 1, ShiftMask, 0 } },
    { True, LockMask,  { 0, LockMask,  0 } }
};

static struct xkb_mods preAlpha[]= {
    { 0,        0,        0 },
    { LockMask, LockMask, 0 }
};

#define NL_VMOD_MASK 0
static  struct xkb_kt_map_entry mapKeypad[]= {
    { True,  ShiftMask, { 1, ShiftMask, 0 } },
    { False, 0,         { 1, 0, NL_VMOD_MASK } }
};

static struct xkb_key_type canonicalTypes[XkbNumRequiredTypes] = {
    { { 0, 0, 0 },
      1,        /* num_levels */
      0,        /* map_count */
      NULL, NULL,
      None, NULL
    },
    { { ShiftMask, ShiftMask, 0 },
      2,        /* num_levels */
      mapSize(map2Level),   /* map_count */
      map2Level, NULL,
      None,      NULL
    },
    { { ShiftMask|LockMask, ShiftMask|LockMask, 0 },
      2,        /* num_levels */
      mapSize(mapAlpha),    /* map_count */
      mapAlpha, preAlpha,
      None,     NULL
    },
    { { ShiftMask, ShiftMask, NL_VMOD_MASK },
      2,        /* num_levels */
      mapSize(mapKeypad),   /* map_count */
      mapKeypad, NULL,
      None,      NULL
    }
};

int
XkbcInitCanonicalKeyTypes(struct xkb_desc * xkb, unsigned which, int keypadVMod)
{
    struct xkb_client_map * map;
    struct xkb_key_type *from, *to;
    int rtrn;

    if (!xkb)
        return BadMatch;

    rtrn= XkbcAllocClientMap(xkb, XkbKeyTypesMask, XkbNumRequiredTypes);
    if (rtrn != Success)
        return rtrn;

    map= xkb->map;
    if ((which & XkbAllRequiredTypes) == 0)
        return Success;

    rtrn = Success;
    from = canonicalTypes;
    to = map->types;

    if (which & XkbOneLevelMask)
        rtrn = XkbcCopyKeyType(&from[XkbOneLevelIndex], &to[XkbOneLevelIndex]);

    if ((which & XkbTwoLevelMask) && (rtrn == Success))
        rtrn = XkbcCopyKeyType(&from[XkbTwoLevelIndex], &to[XkbTwoLevelIndex]);

    if ((which & XkbAlphabeticMask) && (rtrn == Success))
        rtrn = XkbcCopyKeyType(&from[XkbAlphabeticIndex],
                               &to[XkbAlphabeticIndex]);

    if ((which & XkbKeypadMask) && (rtrn == Success)) {
        struct xkb_key_type * type;

        rtrn = XkbcCopyKeyType(&from[XkbKeypadIndex], &to[XkbKeypadIndex]);
        type = &to[XkbKeypadIndex];

        if ((keypadVMod >= 0) && (keypadVMod < XkbNumVirtualMods) &&
            (rtrn == Success)) {
            type->mods.vmods = (1 << keypadVMod);
            type->map[0].active = True;
            type->map[0].mods.mask = ShiftMask;
            type->map[0].mods.real_mods = ShiftMask;
            type->map[0].mods.vmods = 0;
            type->map[0].level = 1;
            type->map[1].active = False;
            type->map[1].mods.mask = 0;
            type->map[1].mods.real_mods = 0;
            type->map[1].mods.vmods = (1 << keypadVMod);
            type->map[1].level = 1;
        }
    }

    return Success;
}

Bool
XkbcVirtualModsToReal(struct xkb_desc * xkb, unsigned virtual_mask,
                      unsigned *mask_rtrn)
{
    int i, bit;
    unsigned mask;

    if (!xkb)
        return False;
    if (virtual_mask == 0) {
        *mask_rtrn = 0;
        return True;
    }
    if (!xkb->server)
        return False;

    for (i = mask = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (virtual_mask & bit)
            mask |= xkb->server->vmods[i];
    }

    *mask_rtrn = mask;
    return True;
}

/*
 * All latin-1 alphanumerics, plus parens, slash, minus, underscore and
 * wildcards.
 */
static unsigned char componentSpecLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

void
XkbcEnsureSafeMapName(char *name)
{
    if (!name)
        return;

    while (*name!='\0') {
        if ((componentSpecLegal[(*name) / 8] & (1 << ((*name) % 8))) == 0)
            *name= '_';
        name++;
    }
}

unsigned
_XkbcKSCheckCase(uint32_t ks)
{
    unsigned set = (ks & (~0xff)) >> 8;
    unsigned rtrn = 0;

    switch (set) {
    case 0: /* latin 1 */
        if ((ks >= XK_A && ks <= XK_Z) ||
            (ks >= XK_Agrave && ks <= XK_THORN && ks != XK_multiply))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_a && ks <= XK_z) ||
            (ks >= XK_agrave && ks <= XK_ydiaeresis))
            rtrn |= _XkbKSLower;
        break;
    case 1: /* latin 2 */
        if ((ks >= XK_Aogonek && ks <= XK_Zabovedot && ks != XK_breve) ||
            (ks >= XK_Racute && ks<=XK_Tcedilla))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_aogonek && ks <= XK_zabovedot && ks != XK_caron) ||
            (ks >= XK_racute && ks <= XK_tcedilla))
            rtrn |= _XkbKSLower;
        break;
    case 2: /* latin 3 */
        if ((ks >= XK_Hstroke && ks <= XK_Jcircumflex) ||
            (ks >= XK_Cabovedot && ks <= XK_Scircumflex))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_hstroke && ks <= XK_jcircumflex) ||
            (ks >= XK_cabovedot && ks <= XK_scircumflex))
            rtrn |= _XkbKSLower;
        break;
    case 3: /* latin 4 */
        if ((ks >= XK_Rcedilla && ks <= XK_Tslash) ||
            (ks == XK_ENG) ||
            (ks >= XK_Amacron && ks <= XK_Umacron))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_rcedilla && ks <= XK_tslash) ||
            (ks == XK_eng) ||
            (ks >= XK_amacron && ks <= XK_umacron))
            rtrn |= _XkbKSLower;
        break;
    case 18: /* latin 8 */
        if ((ks == XK_Babovedot) ||
            (ks >= XK_Dabovedot && ks <= XK_Wacute) ||
            (ks >= XK_Ygrave && ks <= XK_Fabovedot) ||
            (ks == XK_Mabovedot) ||
            (ks == XK_Pabovedot) ||
            (ks == XK_Sabovedot) ||
            (ks == XK_Wdiaeresis) ||
            (ks >= XK_Wcircumflex && ks <= XK_Ycircumflex))
            rtrn |= _XkbKSUpper;
        if ((ks == XK_babovedot) ||
            (ks == XK_dabovedot) ||
            (ks == XK_fabovedot) ||
            (ks == XK_mabovedot) ||
            (ks >= XK_wgrave && ks <= XK_wacute) ||
            (ks == XK_ygrave) ||
            (ks >= XK_wdiaeresis && ks <= XK_ycircumflex))
            rtrn |= _XkbKSLower;
        break;
    case 19: /* latin 9 */
        if (ks == XK_OE || ks == XK_Ydiaeresis)
            rtrn |= _XkbKSUpper;
        if (ks == XK_oe)
            rtrn |= _XkbKSLower;
        break;
    }

    return rtrn;
}

#define UNMATCHABLE(c) ((c) == '(' || (c) == ')' || (c) == '/')

Bool
XkbcNameMatchesPattern(char *name, char *ptrn)
{
    while (ptrn[0] != '\0') {
        if (name[0] == '\0') {
            if (ptrn[0] == '*') {
                ptrn++;
                continue;
            }
            return False;
        }

        if (ptrn[0] == '?') {
            if (UNMATCHABLE(name[0]))
                return False;
        }
        else if (ptrn[0] == '*') {
            if (!UNMATCHABLE(name[0]) &&
                XkbcNameMatchesPattern(name + 1, ptrn))
                return True;
            return XkbcNameMatchesPattern(name, ptrn + 1);
        }
        else if (ptrn[0] != name[0])
            return False;

        name++;
        ptrn++;
    }

    /* if we get here, the pattern is exhausted (-:just like me:-) */
    return (name[0] == '\0');
}
