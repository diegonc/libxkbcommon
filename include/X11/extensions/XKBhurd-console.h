#ifndef _XKBHURD_CONSOLE_H_
#define _XKBHURD_CONSOLE_H_

#include <X11/extensions/XKB.h>

/*
 * New actions for the Hurd console.
 *
 * The id for new actions must be consecutive numbers starting from
 * XkbSA_LastAction as defined by the include above. Keep them in sync.
 *
 * XXX: note, however, that they cannot be expressed in terms of
 * XkbSA_LastAction because the macro has to be redefined here.
 */
#define XkbSA_ConsScroll       0x15

/* Redefine XkbSA_LastAction to take into account new actions. */
#undef XkbSA_LastAction
#define XkbSA_LastAction        XkbSA_ConsScroll

/* Flags for ConsScroll action. */
#define XkbSA_ScreenAbsolute    4
#define XkbSA_LineAbsolute      2
#define XkbSA_UsePercent        8

#endif
