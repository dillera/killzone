#include <cmoc.h>
#include <coco.h>
#include <hirestxt.h>
#include "conio_wrapper.h"

/*
 * cc65 conio like, text console functions
 * on coco
 *
 */
#define SCREEN_BUFFER (byte*) 0x6600 // PMODE graphics buffer address

byte colorset = 1;
BOOL closed = FALSE;
BOOL usecursor = FALSE;

void hirestxt_init(void)
{

  initCoCoSupport();
  closed = FALSE;

  if (isCoCo3)
  {
    width(40);
  }
  else
  {
    // Define a `HiResTextScreenInit` object:
    struct HiResTextScreenInit init =
        {
            42,                 /* characters per row */
            writeCharAt_42cols, /* must be consistent with previous field */
            SCREEN_BUFFER,      /* pointer to text screen buffer */
            TRUE,               /* redirects printf() to the 42x24 text screen */
            (word *)0x112,      /* pointer to a 60 Hz async counter (Color Basic's TIMER) */
            0,                  /* default cursor blinking rate */
            NULL,               /* use inkey(), i.e., Color Basic's INKEY$ */
            NULL,               /* no sound on '\a' */
        };

    initCoCoSupport();
    rgb();
    width(32);                               /* PMODE graphics will only appear from 32x16 (does nothing on CoCo 1&2) */
    pmode(4, (byte *)init.textScreenBuffer); /* hires text mode */
    pcls(255);
    screen(1, colorset);
    initHiResTextScreen(&init);
  }
}

void hirestxt_close(void)
{
  if (!isCoCo3  && !closed)
  {
    closed = TRUE;  
	  closeHiResTextScreen();
    pmode(0, 0);
    screen(0, 0);
    cls(255);
  }
}

void switch_colorset(void)
{
	if (colorset == 0)
	{
		colorset = 1;
	}
	else
	{
		colorset = 0;
	}

	screen(1, colorset);
}

void gotoxy(byte x, byte y)
{
  if (isCoCo3)
  {
    // CoCo 3 text mode
    locate(x,y);
  }
  else
  {
    moveCursor(x, y);
  }
}

void my_clrscr(void)
{
  if (isCoCo3)
  {
    cls(0);
  }
  else
  {
    clear();
    home();
  }
}

unsigned char wherex(void)
{
  if (isCoCo3)
  {
    return 0;
  }
  else
  {
    return getCursorColumn();
  }
}

unsigned char wherey(void)
{
  if (isCoCo3)
  {
    return 0;
  }
  else
  {
    return getCursorRow();
  }
}

unsigned char cursor(unsigned char onoff)
{
  usecursor = onoff;

  if (!isCoCo3)
  {
    if (onoff)
    {
      animateCursor();
    }
    else
    {
      removeCursor();
    }
  }

  return 0;
}

void cputcxy(unsigned char x, unsigned char y, char c)
{
	gotoxy(x, y);
	cputc(c);
}

void cputsxy(unsigned char x, unsigned char y, const char *s)
{
	gotoxy(x, y);
	cputs(s);
}

byte cgetc()
{
  byte shift = false;
  byte k;

  while (true)
  {
    if (usecursor)
    {
      k = waitKeyBlinkingCursor();
    }
    else
    {
      k = inkey();
    }

    if (isKeyPressed(KEY_PROBE_SHIFT, KEY_BIT_SHIFT))
    {
      shift = 0x00;
    }
    else
    {
      if (k > '@' && k < '[')
        shift = 0x20;
    }

    if (k)
      return k + shift;
  }
}

void get_line(char *buf, uint8_t max_len)
{
  char *line_buf = NULL;

  uint8_t c;
  uint8_t i = 0;
  uint8_t init_x = wherex();

  do
  {
    gotox(i + init_x);
    if (!isCoCo3)
    {
      cursor(1);
    }

    c = cgetc();

    if (isprint(c))
    {
      putchar(c);
      buf[i] = c;
      if (i < max_len - 1)
        i++;
    }
    else if (c == KEY_LEFT_ARROW)
    {
      if (i)
      {
        putchar(KEY_LEFT_ARROW);
        putchar(' ');
        putchar(KEY_LEFT_ARROW);
        --i;
      }
    }
  } while (c != KEY_ENTER);
  putchar('\n');
  buf[i] = '\0';

  cursor(0);
}

/* this is from cc65.h */
unsigned char doesclrscrafterexit(void)
{
	return 0;
}
