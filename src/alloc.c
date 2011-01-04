/*
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
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "xkballoc.h"
#include "xkbgeom.h"
#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBhurd-console.h>

int
XkbcAllocCompatMap(struct xkb_desc * xkb, unsigned which, unsigned nSI)
{
    struct xkb_compat_map * compat;
    struct xkb_sym_interpret *prev_interpret;

    if (!xkb)
        return BadMatch;

    if (xkb->compat) {
        if (xkb->compat->size_si >= nSI)
            return Success;

        compat = xkb->compat;
        compat->size_si = nSI;
        if (!compat->sym_interpret)
            compat->num_si = 0;

        prev_interpret = compat->sym_interpret;
        compat->sym_interpret = _XkbTypedRealloc(compat->sym_interpret,
                                                 nSI, struct xkb_sym_interpret);
        if (!compat->sym_interpret) {
            free(prev_interpret);
            compat->size_si = compat->num_si = 0;
            return BadAlloc;
        }

        if (compat->num_si != 0)
            _XkbClearElems(compat->sym_interpret, compat->num_si,
                           compat->size_si - 1, struct xkb_sym_interpret);

        return Success;
    }

    compat = _XkbTypedCalloc(1, struct xkb_compat_map);
    if (!compat)
        return BadAlloc;

    if (nSI > 0) {
        compat->sym_interpret = _XkbTypedCalloc(nSI, struct xkb_sym_interpret);
        if (!compat->sym_interpret) {
            free(compat);
            return BadAlloc;
        }
    }
    compat->size_si = nSI;
    compat->num_si = 0;
    bzero(&compat->groups[0], XkbNumKbdGroups * sizeof(struct xkb_mods));
    xkb->compat = compat;

    return Success;
}


static void
XkbcFreeCompatMap(struct xkb_desc * xkb, unsigned which, Bool freeMap)
{
    struct xkb_compat_map * compat;

    if (!xkb || !xkb->compat)
        return;

    compat = xkb->compat;
    if (freeMap)
        which = XkbAllCompatMask;

    if (which & XkbGroupCompatMask)
        bzero(&compat->groups[0], XkbNumKbdGroups * sizeof(struct xkb_mods));

    if (which & XkbSymInterpMask) {
        if (compat->sym_interpret && (compat->size_si > 0))
            free(compat->sym_interpret);
        compat->size_si = compat->num_si = 0;
        compat->sym_interpret = NULL;
    }

    if (freeMap) {
        free(compat);
        xkb->compat = NULL;
    }
}

int
XkbcAllocNames(struct xkb_desc * xkb, unsigned which, int nTotalRG, int nTotalAliases)
{
    struct xkb_names * names;

    if (!xkb)
        return BadMatch;

    if (!xkb->names) {
        xkb->names = _XkbTypedCalloc(1, struct xkb_names);
        if (!xkb->names)
            return BadAlloc;
    }
    names = xkb->names;

    if ((which & XkbKTLevelNamesMask) && xkb->map && xkb->map->types) {
        int i;
        struct xkb_key_type * type;

        type = xkb->map->types;
        for (i = 0; i < xkb->map->num_types; i++, type++) {
            if (!type->level_names) {
                type->level_names = _XkbTypedCalloc(type->num_levels, uint32_t);
                if (!type->level_names)
                    return BadAlloc;
            }
        }
    }

    if ((which & XkbKeyNamesMask) && !names->keys) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadValue;

        names->keys = _XkbTypedCalloc(xkb->max_key_code + 1, struct xkb_key_name);
        if (!names->keys)
            return BadAlloc;
    }

    if ((which & XkbKeyAliasesMask) && (nTotalAliases > 0)) {
        if (!names->key_aliases)
            names->key_aliases = _XkbTypedCalloc(nTotalAliases,
                                                 struct xkb_key_alias);
        else if (nTotalAliases > names->num_key_aliases) {
            struct xkb_key_alias *prev_aliases = names->key_aliases;

            names->key_aliases = _XkbTypedRealloc(names->key_aliases,
                                                  nTotalAliases,
                                                  struct xkb_key_alias);
            if (names->key_aliases)
                _XkbClearElems(names->key_aliases, names->num_key_aliases,
                               nTotalAliases - 1, struct xkb_key_alias);
            else
                free(prev_aliases);
        }

        if (!names->key_aliases) {
            names->num_key_aliases = 0;
            return BadAlloc;
        }

        names->num_key_aliases = nTotalAliases;
    }

    if ((which & XkbRGNamesMask) && (nTotalRG > 0)) {
        if (!names->radio_groups)
            names->radio_groups = _XkbTypedCalloc(nTotalRG, uint32_t);
        else if (nTotalRG > names->num_rg) {
            uint32_t *prev_radio_groups = names->radio_groups;

            names->radio_groups = _XkbTypedRealloc(names->radio_groups,
                                                   nTotalRG, uint32_t);
            if (names->radio_groups)
                _XkbClearElems(names->radio_groups, names->num_rg,
                               nTotalRG - 1, uint32_t);
            else
                free(prev_radio_groups);
        }

        if (!names->radio_groups)
            return BadAlloc;

        names->num_rg = nTotalRG;
    }

    return Success;
}

static void
XkbcFreeNames(struct xkb_desc * xkb, unsigned which, Bool freeMap)
{
    struct xkb_names * names;

    if (!xkb || !xkb->names)
        return;

    names = xkb->names;
    if (freeMap)
        which = XkbAllNamesMask;

    if (which & XkbKTLevelNamesMask) {
        struct xkb_client_map * map = xkb->map;

        if (map && map->types) {
            int i;
            struct xkb_key_type * type = map->types;

            for (i = 0; i < map->num_types; i++, type++) {
                if (type->level_names) {
                    free(type->level_names);
                    type->level_names = NULL;
                }
            }
        }
    }

    if ((which & XkbKeyNamesMask) && names->keys) {
        free(names->keys);
        names->keys = NULL;
        names->num_keys = 0;
    }

    if ((which & XkbKeyAliasesMask) && names->key_aliases) {
        free(names->key_aliases);
        names->key_aliases = NULL;
        names->num_key_aliases = 0;
    }

    if ((which & XkbRGNamesMask) && names->radio_groups) {
        free(names->radio_groups);
        names->radio_groups = NULL;
        names->num_rg = 0;
    }

    if (freeMap) {
        free(names);
        xkb->names = NULL;
    }
}

int
XkbcAllocControls(struct xkb_desc * xkb, unsigned which)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->ctrls) {
        xkb->ctrls = _XkbTypedCalloc(1, struct xkb_controls);
        if (!xkb->ctrls)
            return BadAlloc;
    }

    return Success;
}

static void
XkbcFreeControls(struct xkb_desc * xkb, unsigned which, Bool freeMap)
{
    if (freeMap && xkb && xkb->ctrls) {
        free(xkb->ctrls);
        xkb->ctrls = NULL;
    }
}

int
XkbcAllocIndicatorMaps(struct xkb_desc * xkb)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->indicators) {
        xkb->indicators = _XkbTypedCalloc(1, struct xkb_indicator);
        if (!xkb->indicators)
            return BadAlloc;
    }

    return Success;
}

static void
XkbcFreeIndicatorMaps(struct xkb_desc * xkb)
{
    if (xkb && xkb->indicators) {
        free(xkb->indicators);
        xkb->indicators = NULL;
    }
}

struct xkb_desc *
XkbcAllocKeyboard(void)
{
    struct xkb_desc *xkb;

    xkb = _XkbTypedCalloc(1, struct xkb_desc);
    if (xkb)
        xkb->device_spec = XkbUseCoreKbd;
    return xkb;
}

void
XkbcFreeKeyboard(struct xkb_desc * xkb, unsigned which, Bool freeAll)
{
    if (!xkb)
        return;

    if (freeAll)
        which = XkbAllComponentsMask;

    if (which & XkbClientMapMask)
        XkbcFreeClientMap(xkb, XkbAllClientInfoMask, True);
    if (which & XkbServerMapMask)
        XkbcFreeServerMap(xkb, XkbAllServerInfoMask, True);
    if (which & XkbCompatMapMask)
        XkbcFreeCompatMap(xkb, XkbAllCompatMask, True);
    if (which & XkbIndicatorMapMask)
        XkbcFreeIndicatorMaps(xkb);
    if (which & XkbNamesMask)
        XkbcFreeNames(xkb, XkbAllNamesMask, True);
    if ((which & XkbGeometryMask) && xkb->geom)
        XkbcFreeGeometry(xkb->geom, XkbGeomAllMask, True);
    if (which & XkbControlsMask)
        XkbcFreeControls(xkb, XkbAllControlsMask, True);
    if (freeAll)
        free(xkb);
}
