/***********************************************************
Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/************************************************************
 Copyright 1994 by Silicon Graphics Computer Systems, Inc.

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
#include "xkbmisc.h"
#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"

#define InitialTableSize 100

typedef struct _Node {
    struct _Node    *left, *right;
    uint32_t          a;
    unsigned int    fingerPrint;
    char            *string;
} NodeRec, *NodePtr;

#define BAD_RESOURCE 0xe0000000

static uint32_t lastAtom = None;
static NodePtr atomRoot = NULL;
static unsigned long tableLength;
static NodePtr *nodeTable = NULL;
static InternAtomFuncPtr do_intern_atom = NULL;
static GetAtomValueFuncPtr do_get_atom_value = NULL;

void
xkb_init_atoms(InternAtomFuncPtr intern, GetAtomValueFuncPtr get_atom_value)
{
    if (intern && get_atom_value) {
        if (do_intern_atom && do_get_atom_value)
            return;
        do_intern_atom = intern;
        do_get_atom_value = get_atom_value;
        return;
    }
}

const char *
XkbcAtomText(uint32_t atom)
{
    NodePtr node;

    if (do_get_atom_value)
        return do_get_atom_value(atom);

    if ((atom == None) || (atom > lastAtom))
        return NULL;
    if (!(node = nodeTable[atom]))
        return NULL;
    return node->string;
}

char *
XkbcAtomGetString(uint32_t atom)
{
    const char *ret = XkbcAtomText(atom);
    return ret ? strdup(ret) : NULL;
}

static uint32_t
_XkbcMakeAtom(const char *string, unsigned len, Bool makeit)
{
    NodePtr *np;
    unsigned i;
    int comp;
    unsigned int fp = 0;

    np = &atomRoot;
    for (i = 0; i < (len + 1) / 2; i++) {
        fp = fp * 27 + string[i];
        fp = fp * 27 + string[len - 1 - i];
    }

    while (*np) {
        if (fp < (*np)->fingerPrint)
            np = &((*np)->left);
        else if (fp > (*np)->fingerPrint)
            np = &((*np)->right);
        else {
            /* now start testing the strings */
            comp = strncmp(string, (*np)->string, (int)len);
            if ((comp < 0) || ((comp == 0) && (len < strlen((*np)->string))))
                np = &((*np)->left);
            else if (comp > 0)
                np = &((*np)->right);
            else
                return(*np)->a;
            }
    }

    if (makeit) {
        NodePtr nd;

        nd = (NodePtr)malloc(sizeof(NodeRec));
        if (!nd)
            return BAD_RESOURCE;

        nd->string = (char *)malloc(len + 1);
        if (!nd->string) {
            free(nd);
            return BAD_RESOURCE;
        }
        strncpy(nd->string, string, (int)len);
        nd->string[len] = 0;

        if ((lastAtom + 1) >= tableLength) {
            NodePtr *table;
	    int newLength;

	    if (tableLength == 0)
		newLength = InitialTableSize;
	    else
		newLength = tableLength * 2;

            table = realloc(nodeTable, newLength * sizeof(NodePtr));
            if (!table) {
                if (nd->string != string)
                    free(nd->string);
                free(nd);
                return BAD_RESOURCE;
            }
            tableLength = newLength;
	    table[None] = NULL;

            nodeTable = table;
        }

        *np = nd;
        nd->left = nd->right = NULL;
        nd->fingerPrint = fp;
        nd->a = (++lastAtom);
        *(nodeTable + lastAtom) = nd;

        return nd->a;
    }
    else
        return None;
}

uint32_t
XkbcInternAtom(const char *name, Bool onlyIfExists)
{
    if (!name)
        return None;
    if (do_intern_atom)
        return do_intern_atom(name);
    return _XkbcMakeAtom(name, strlen(name), !onlyIfExists);
}
