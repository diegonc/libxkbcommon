/*
Copyright 2009  Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

#ifndef _XKBALLOC_H_
#define _XKBALLOC_H_

#include <X11/X.h>
#include <X11/Xdefs.h>
#include "X11/extensions/XKBcommon.h"

extern int
XkbcAllocCompatMap(struct xkb_desc * xkb, unsigned which, unsigned nSI);

extern int
XkbcAllocNames(struct xkb_desc * xkb, unsigned which, int nTotalRG,
               int nTotalAliases);

extern int
XkbcAllocControls(struct xkb_desc * xkb, unsigned which);

extern int
XkbcAllocIndicatorMaps(struct xkb_desc * xkb);

extern struct xkb_desc *
XkbcAllocKeyboard(void);

extern void
XkbcFreeKeyboard(struct xkb_desc * xkb, unsigned which, Bool freeAll);

/***====================================================================***/

extern int
XkbcAllocClientMap(struct xkb_desc * xkb, unsigned which, unsigned nTotalTypes);

extern int
XkbcAllocServerMap(struct xkb_desc * xkb, unsigned which, unsigned nNewActions);

extern int
XkbcCopyKeyType(struct xkb_key_type * from, struct xkb_key_type *into);

extern uint32_t *
XkbcResizeKeySyms(struct xkb_desc * xkb, int key, int needed);

extern union xkb_action *
XkbcResizeKeyActions(struct xkb_desc * xkb, int key, int needed);

extern void
XkbcFreeClientMap(struct xkb_desc * xkb, unsigned what, Bool freeMap);

extern void
XkbcFreeServerMap(struct xkb_desc * xkb, unsigned what, Bool freeMap);

#endif /* _XKBALLOC_H_ */
