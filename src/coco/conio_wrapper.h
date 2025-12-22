#ifndef COCO_CONIO_WRAPPER_H
#define COCO_CONIO_WRAPPER_H

#include <cmoc.h>
#include <coco.h>
#include <hirestxt.h>

#define KEY_LEFT_ARROW       0x08
#define KEY_RIGHT_ARROW      0x09
#define KEY_UP_ARROW         0x5E
#define KEY_DOWN_ARROW       0x0A
#define KEY_SHIFT_UP_ARROW   0x5F
#define KEY_SHIFT_DOWN_ARROW 0x5B
#define KEY_ENTER            0x0D
#define KEY_BREAK            0x03
#define KEY_CLEAR            0x0C

#define true 1
#define false 0

#undef gotoxy
#undef gotox
#undef gotoy
#undef wherex
#undef wherey
#undef revers
#undef cursor
#undef cputc
#undef cputs
#undef cputsxy
#undef isprint
#undef kbhit
#undef cgetc

unsigned char wherex(void);
unsigned char wherey(void);

#define gotox(x) moveCursor(x, wherey())
#define gotoy(y) moveCursor(wherex(), y)
 
#define cputc(c) putchar(c)
#define cputs(s) putstr(s, strlen(s))

#define isprint(c) (c>=0x20 && c<=0x8E)

#define kbhit() inkey()

void gotoxy(byte x, byte y);

byte cgetc(void);

byte cgetc_cursor(void);

unsigned char revers(unsigned char onoff);
unsigned char cursor(unsigned char onoff);

void cputcxy(unsigned char x, unsigned char y, char c);
void cputsxy(unsigned char x, unsigned char y, const char* s);

void chlinexy(unsigned char x, unsigned char y, unsigned char length);

unsigned char doesclrscrafterexit (void);

void hirestxt_init(void);
void hirestxt_close(void);

void switch_colorset(void);

void get_line(char *buf, uint8_t max_len);

#endif // COCO_CONIO_WRAPPER_H
