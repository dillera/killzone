#ifndef KEYDEFS_H
#define KEYDEFS_H

#ifdef __APPLE2ENH__
#include <apple2enh.h>
#else
#include <apple2.h>
#endif

/* apple2 target does not always expose up/down cursor defines. */
#ifndef CH_CURS_UP
#define CH_CURS_UP 0x0B
#endif

#ifndef CH_CURS_DOWN
#define CH_CURS_DOWN 0x0A
#endif

#ifndef CH_CURS_LEFT
#define CH_CURS_LEFT 0x08
#endif

#ifndef CH_CURS_RIGHT
#define CH_CURS_RIGHT 0x15
#endif

#define KEY_LEFT_ARROW       CH_CURS_LEFT
#define KEY_RIGHT_ARROW      CH_CURS_RIGHT
#define KEY_UP_ARROW         CH_CURS_UP
#define KEY_DOWN_ARROW       CH_CURS_DOWN

#endif /* KEYDEFS_H */
