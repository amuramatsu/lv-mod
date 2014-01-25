/*
 * console.c
 *
 * All rights reserved. Copyright (C) 1996 by NARITA Tomio.
 * $Id: console.c,v 1.8 2004/01/05 07:27:46 nrt Exp $
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef MSDOS
#include <dos.h>
#endif /* MSDOS */

#ifdef WIN32NATIVE
#include <windows.h>
#include <fcntl.h>
#include "getch_msvc.h"
#endif /* WIN32NATIVE */

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif /* HAVE_TERMIOS_H */

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */
#endif /* UNIX */

#ifdef TERMINFO
#include <termio.h>
#include <curses.h>
#include <term.h>
#endif /* TERMINFO */

#include <import.h>
#include <ascii.h>
#include <attr.h>
#include <begin.h>
#include <console.h>

#define ANSI_ATTR_LENGTH	8

#ifdef WIN32NATIVE
private HANDLE hStdin;
private HANDLE hStdout;
private DWORD oldConsoleMode, newConsoleMode;
private CONSOLE_CURSOR_INFO oldCursorInfo;
private BOOL bStdinIsConsole;
typedef struct _console_buf_saved {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  PCHAR_INFO                 buffer;
  BOOL                       valid;
} CONSOLE_BUF_SAVED;
private CONSOLE_BUF_SAVED old_console_buf;
#endif

#if (defined( MSDOS ) || defined( WIN32 )) && !defined(WIN32NATIVE)
private char tbuf[ 16 ];

private char *clear_screen		= "\x1b[2J";
private char *clr_eol			= "\x1b[K";
private char *delete_line		= "\x1b[M";
private char *insert_line		= "\x1b[L";
private char *enter_standout_mode	= "\x1b[7m";
private char *exit_standout_mode	= "\x1b[m";
private char *enter_underline_mode	= "\x1b[4m";
private char *exit_underline_mode	= "\x1b[m";
private char *enter_bold_mode		= "\x1b[1m";
private char *exit_attribute_mode	= "\x1b[m";
private char *cursor_visible		= NULL;
private char *cursor_invisible		= NULL;
private char *enter_ca_mode		= NULL;
private char *exit_ca_mode		= NULL;
private char *keypad_local		= NULL;
private char *keypad_xmit		= NULL;

private void tputs( char *cp, int affcnt, int (*outc)(char) )
{
  while( *cp )
    outc( *cp++ );
}

private int (*putfunc)(char) = ConsolePrint;
#endif /* MSDOS */

#ifdef UNIX

#ifdef HAVE_TERMIOS_H
private struct termios ttyOld, ttyNew;
#else
private struct sgttyb ttyOld, ttyNew;
#endif /* HAVE_TERMIOS_H */

#ifdef putchar
private int putfunc( int ch )
{
  return putchar( ch );
}
#else
private int (*putfunc)(int) = putchar;
#endif

#endif /* UNIX */

#ifdef TERMCAP
private char entry[ 1024 ];
private char func[ 1024 ];

extern char *tgetstr(), *tgoto();
extern int tgetent(), tgetflag(), tgetnum(), tputs();

private char *cursor_address		= NULL;
private char *clear_screen		= NULL;
private char *clr_eol			= NULL;
private char *insert_line		= NULL;
private char *delete_line		= NULL;
private char *enter_standout_mode	= NULL;
private char *exit_standout_mode	= NULL;
private char *enter_underline_mode	= NULL;
private char *exit_underline_mode	= NULL;
private char *enter_bold_mode		= NULL;
private char *exit_attribute_mode	= NULL;
private char *cursor_visible		= NULL;
private char *cursor_invisible		= NULL;
private char *enter_ca_mode		= NULL;
private char *exit_ca_mode		= NULL;
private char *keypad_local		= NULL;
private char *keypad_xmit		= NULL;
#endif /* TERMCAP */

#ifdef WIN32NATIVE
private void
SaveConsoleBuffer(CONSOLE_BUF_SAVED *save) 
{
  int        size;
  COORD      pos;
  SMALL_RECT region;
 
  pos.X = pos.Y = 0;
  GetConsoleScreenBufferInfo(hStdout, &save->csbi);
  size = save->csbi.dwSize.X * save->csbi.dwSize.Y;
  save->buffer = (PCHAR_INFO)malloc(size * sizeof(CHAR_INFO));
  region.Top = 0;
  region.Bottom = save->csbi.srWindow.Bottom;
  region.Left = 0;
  region.Right = save->csbi.dwSize.X - 1;
  ReadConsoleOutput(hStdout, save->buffer, save->csbi.dwSize, pos, &region);
  save->valid = TRUE;
}

private void
RestoreConsoleBuffer(CONSOLE_BUF_SAVED *save) 
{
  if (save->valid) {
    COORD      pos;
    SMALL_RECT region;
    pos.X = pos.Y = 0;
    region.Top = 0;
    region.Bottom = save->csbi.srWindow.Bottom;
    region.Left = 0;
    region.Right = save->csbi.dwSize.X - 1;
    SetConsoleScreenBufferSize(hStdout, save->csbi.dwSize);
    WriteConsoleOutput(hStdout, save->buffer, save->csbi.dwSize, pos, &region);
    SetConsoleCursorPosition(hStdout, save->csbi.dwCursorPosition);
    SetConsoleWindowInfo(hStdout, TRUE, &save->csbi.srWindow);
    SetConsoleTextAttribute(hStdout, save->csbi.wAttributes);
    free(save->buffer);
    save->valid = FALSE;
  }
}

#endif /* WIN32NATIVE */

public void ConsoleInit()
{
  allow_interrupt	= FALSE;
  kb_interrupted	= FALSE;
  window_changed	= FALSE;
}

public void ConsoleResetAnsiSequence()
{
  ansi_standout		= "7";
  ansi_reverse		= "7";
  ansi_blink		= "5";
  ansi_underline	= "4";
  ansi_hilight		= "1";
}

#if defined(MSDOS) || defined(WIN32NATIVE)
private void InterruptIgnoreHandler( int arg )
{
  signal( SIGINT, InterruptIgnoreHandler );
}
#endif /* MSDOS || WIN32NATIVE */

private RETSIGTYPE InterruptHandler( int arg )
{
  kb_interrupted = TRUE;

#ifndef HAVE_SIGVEC
  signal( SIGINT, InterruptHandler );
#endif /* HAVE_SIGVEC */
}

public void ConsoleEnableInterrupt()
{
#ifdef WIN32NATIVE
  DWORD dw;
  allow_interrupt = TRUE;
  signal( SIGINT, InterruptHandler );
  GetConsoleMode(hStdin, &dw);
  SetConsoleMode(hStdin, dw | ENABLE_PROCESSED_INPUT );
#endif /* WIN32NATIVE */

#ifdef MSDOS
  allow_interrupt = TRUE;
  signal( SIGINT, InterruptHandler );
#endif /* MSDOS */

#ifdef UNIX
  signal( SIGTSTP, SIG_IGN );
#ifdef HAVE_TERMIOS_H
  ttyNew.c_lflag |= ISIG;
  tcsetattr( 0, TCSADRAIN, &ttyNew );
#else /* HAVE_TERMIOS_H */
  ttyNew.sg_flags &= ~RAW;
  ioctl( 0, TIOCSETN, &ttyNew );
#endif /* HAVE_TERMIOS_H */
#endif /* UNIX */
}

public void ConsoleDisableInterrupt()
{
#ifdef WIN32NATIVE
  DWORD dw;
  GetConsoleMode(hStdin, &dw);
  SetConsoleMode(hStdin, dw & ~ENABLE_PROCESSED_INPUT );
  allow_interrupt = FALSE;
  signal( SIGINT, InterruptIgnoreHandler );
#endif /* WIN32NATIVE */

#ifdef MSDOS
  allow_interrupt = FALSE;
  signal( SIGINT, InterruptIgnoreHandler );
#endif /* MSDOS */

#ifdef UNIX
#ifdef HAVE_TERMIOS_H
  ttyNew.c_lflag &= ~ISIG;
  tcsetattr( 0, TCSADRAIN, &ttyNew );
#else /* HAVE_TERMIOS_H */
  ttyNew.sg_flags |= RAW;
  ioctl( 0, TIOCSETN, &ttyNew );
#endif /* HAVE_TERMIOS_H */
  signal( SIGTSTP, SIG_DFL );
#endif /* UNIX */
}

public void ConsoleGetWindowSize()
{
#ifdef WIN32NATIVE
  CONSOLE_SCREEN_BUFFER_INFO info;
  WIDTH  = 80;
  HEIGHT = 24;
  if (GetConsoleScreenBufferInfo(hStdout, &info)) {
    WIDTH = info.dwSize.X;
    HEIGHT = info.srWindow.Bottom - info.srWindow.Top + 1;
  }
#endif /* WIN32NATIVE */

#ifdef UNIX
#ifdef WIN32
  WIDTH  = 80;
  HEIGHT = 24;
#else /* WIN32 */
  struct winsize winSize;

  ioctl( 0, TIOCGWINSZ, &winSize );
  WIDTH = winSize.ws_col;
  HEIGHT = winSize.ws_row;
  if( 0 >= WIDTH || 0 >= HEIGHT ){
#ifdef HAVE_TGETNUM
    WIDTH = tgetnum( "columns" );
    HEIGHT = tgetnum( "lines" );
#else
    WIDTH = tigetnum( "columns" );
    HEIGHT = tigetnum( "lines" );
#endif /* HAVE_TGETNUM */
    if( 0 >= WIDTH || 0 >= HEIGHT )
      WIDTH = 80, HEIGHT = 24;
  }
#endif /* WIN32 */
#endif /* UNIX */
}

#ifdef UNIX
private RETSIGTYPE WindowChangeHandler( int arg )
{
  window_changed = TRUE;

  ConsoleGetWindowSize();

#ifndef HAVE_SIGVEC
  signal( SIGWINCH, WindowChangeHandler );
#endif /* HAVE_SIGVEC */
}
#endif /* UNIX */

public void ConsoleTermInit()
{
  /*
   * 1. setup terminal capability
   * 2. retrieve window size
   * 3. initialize terminal status
   */

#ifdef WIN32NATIVE
  hStdin  = GetStdHandle(STD_INPUT_HANDLE);   /* 標準入力ハンドルの取得 */
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);  /* 標準出力ハンドルの取得 */

  ConsoleGetWindowSize();
  no_scroll = FALSE;
#endif

#if (defined( MSDOS ) || defined( WIN32 )) && !defined(WIN32NATIVE)
  byte *ptr;

#define ANSI		0
#define FMRCARD		1

  int term = ANSI;

  if( NULL != (ptr = getenv("TERM")) ){
    if( !strcmp( ptr, "fmr4020" ) ){
      term = FMRCARD;
      WIDTH = 40;
      HEIGHT = 19;
    } else if( !strcmp( ptr, "fmr8025" ) ){
      term = FMRCARD;
      WIDTH = 80;
      HEIGHT = 24;
    }
  }

  switch( term ){
  case FMRCARD:
    delete_line      = "\x1bR";
    insert_line      = "\x1b" "E";
    cursor_visible   = "\x1b[v";
    cursor_invisible = "\x1b[1v";
    break;
  default:
    WIDTH  = 80;
    HEIGHT = 24;
  }

  cur_left		= "\x1bK";
  cur_right		= "\x1bM";
  cur_up		= "\x1bH";
  cur_down		= "\x1bP";
  cur_ppage		= "\x1bI";
  cur_npage		= "\x1bQ";

#endif /* MSDOS */

#ifdef TERMCAP
  byte *term, *ptr;
#endif
#ifdef TERMINFO
  byte *term;
  int state;
#endif

#ifdef UNIX
  int fd = open("/dev/tty", O_RDONLY);
  dup2(fd, 0);
  close(fd);
#endif

#ifdef TERMCAP
  if( NULL == (term = getenv( "TERM" )) )
    fprintf( stderr, "lv: environment variable TERM is required\n" );
  if( 0 >= tgetent( entry, term ) )
    fprintf( stderr, "lv: %s not found in termcap\n", term );

  ConsoleGetWindowSize();

  ptr = func;

  cursor_address	= tgetstr( "cm", &ptr );
  clear_screen		= tgetstr( "cl", &ptr );
  clr_eol		= tgetstr( "ce", &ptr );
  insert_line		= tgetstr( "al", &ptr );
  delete_line		= tgetstr( "dl", &ptr );
  enter_standout_mode	= tgetstr( "so", &ptr );
  exit_standout_mode	= tgetstr( "se", &ptr );
  enter_underline_mode	= tgetstr( "us", &ptr );
  exit_underline_mode	= tgetstr( "ue", &ptr );
  enter_bold_mode	= tgetstr( "md", &ptr );
  exit_attribute_mode	= tgetstr( "me", &ptr );
  cursor_visible	= tgetstr( "ve", &ptr );
  cursor_invisible	= tgetstr( "vi", &ptr );
  enter_ca_mode		= tgetstr( "ti", &ptr );
  exit_ca_mode		= tgetstr( "te", &ptr );

  keypad_local		= tgetstr( "ke", &ptr );
  keypad_xmit		= tgetstr( "ks", &ptr );

  cur_left		= tgetstr( "kl", &ptr );
  cur_right		= tgetstr( "kr", &ptr );
  cur_up		= tgetstr( "ku", &ptr );
  cur_down		= tgetstr( "kd", &ptr );
  cur_ppage		= tgetstr( "kP", &ptr );
  cur_npage		= tgetstr( "kN", &ptr );

  if( NULL == cursor_address || NULL == clear_screen || NULL == clr_eol )
    fprintf( stderr, "lv: termcap cm, cl, ce are required\n" );

  if( NULL == insert_line || NULL == delete_line )
    no_scroll = TRUE;
  else
    no_scroll = FALSE;
#endif /* TERMCAP */

#ifdef TERMINFO

  if( NULL == (term = getenv( "TERM" )) )
    fprintf( stderr, "lv: environment variable TERM is required\n" );

  setupterm( term, 1, &state );
  if( 1 != state )
    fprintf( stderr, "lv: cannot initialize terminal\n" );

  ConsoleGetWindowSize();

  cur_left		= key_left;
  cur_right		= key_right;
  cur_up		= key_up;
  cur_down		= key_down;
  cur_ppage		= key_ppage;
  cur_npage		= key_npage;

  if( NULL == cursor_address || NULL == clear_screen || NULL == clr_eol )
    fprintf( stderr, "lv: terminfo cursor_address, clr_eol are required\n" );

  if( NULL == insert_line || NULL == delete_line )
    no_scroll = TRUE;
  else
    no_scroll = FALSE;
#endif /* TERMINFO */

#ifndef WIN32NATIVE
  if( enter_ca_mode )
    tputs( enter_ca_mode, 1, putfunc );
  if( keypad_xmit )
    tputs( keypad_xmit, 1, putfunc );
#endif /* WIN32NATIVE */
}

public void ConsoleSetUp()
{
#if defined(MSDOS) || defined(WIN32NATIVE)
  signal( SIGINT, InterruptIgnoreHandler );
#endif /* MSDOS || WIN32NATIVE */

#ifdef WIN32NATIVE
  /* stdin リダイレクト時の対処（stdinをunbuffered mode に）*/
  setvbuf(stdin, NULL, _IONBF, 0);
  setmode(fileno(stdin), O_BINARY);
  if (GetConsoleMode(hStdin, &oldConsoleMode)) {
    /* Win32コンソールの場合は念のためダイレクトモードに設定 */
    bStdinIsConsole = TRUE;
    newConsoleMode = oldConsoleMode;
    oldConsoleMode &= ~(ENABLE_PROCESSED_INPUT |
			ENABLE_LINE_INPUT |
			ENABLE_ECHO_INPUT |
			ENABLE_INSERT_MODE |
			ENABLE_QUICK_EDIT_MODE);
    SetConsoleMode(hStdin, newConsoleMode);
  }
  if (GetConsoleCursorInfo(hStdout, &oldCursorInfo)) {
    COORD size;
    size.X = WIDTH;
    size.Y = HEIGHT;
    SaveConsoleBuffer(&old_console_buf);
    SetConsoleScreenBufferSize(hStdout, size);
  }
#endif /* WIN32NATIVE */

#ifdef HAVE_SIGVEC
  struct sigvec sigVec;

  sigVec.sv_handler = WindowChangeHandler;
  sigVec.sv_mask = sigmask( SIGWINCH );
  sigVec.sv_flags = SV_INTERRUPT;
  sigvec( SIGWINCH, &sigVec, NULL );

  sigVec.sv_handler = InterruptHandler;
  sigVec.sv_mask = sigmask( SIGINT );
  sigVec.sv_flags = SV_INTERRUPT;
  sigvec( SIGINT, &sigVec, NULL );
#else
# ifdef SIGWINCH
  signal( SIGWINCH, WindowChangeHandler );
# endif 
  signal( SIGINT, InterruptHandler );
#endif /* HAVE_SIGVEC */

#ifdef UNIX
#ifdef HAVE_TERMIOS_H
  tcgetattr( 0, &ttyOld );
  ttyNew = ttyOld;
  ttyNew.c_iflag &= ~ISTRIP;
  ttyNew.c_iflag &= ~INLCR;
  ttyNew.c_iflag &= ~ICRNL;
  ttyNew.c_iflag &= ~IGNCR;
  ttyNew.c_lflag &= ~ISIG;
  ttyNew.c_lflag &= ~ICANON;
  ttyNew.c_lflag &= ~ECHO;
  ttyNew.c_lflag &= ~IEXTEN;
#ifdef VDISCRD /* IBM AIX */ 
#define VDISCARD VDISCRD 
#endif 
  ttyNew.c_cc[ VDISCARD ] = -1;
  ttyNew.c_cc[ VMIN ] = 1;
  ttyNew.c_cc[ VTIME ] = 0;
  tcsetattr( 0, TCSADRAIN, &ttyNew );
#else
  ioctl( 0, TIOCGETP, &ttyOld );
  ttyNew = ttyOld;
  ttyNew.sg_flags &= ~ECHO;
  ttyNew.sg_flags |= RAW;
  ttyNew.sg_flags |= CRMOD;
  ioctl( 0, TIOCSETN, &ttyNew );
#endif /* HAVE_TERMIOS_H */
#endif /* UNIX */
}

public void ConsoleSetDown()
{
#ifdef UNIX
#ifdef HAVE_TERMIOS_H
  tcsetattr( 0, TCSADRAIN, &ttyOld );
#else
  ioctl( 0, TIOCSETN, &ttyOld );
#endif /* HAVE_TERMIOS_H */
#endif /* UNIX */

#ifdef WIN32NATIVE
  SetConsoleMode(hStdin, oldConsoleMode);
  SetConsoleCursorInfo(hStdout, &oldCursorInfo);
  RestoreConsoleBuffer(&old_console_buf);
#else /* !WIN32NATIVE */
  if( keypad_local )
    tputs( keypad_local, 1, putfunc );
  if( exit_ca_mode )
    tputs( exit_ca_mode, 1, putfunc );
  else {
    ConsoleSetCur( 0, HEIGHT - 1 );
    ConsolePrint( CR );
    ConsolePrint( LF );
  }
#endif /* WIN32NATIVE */
}

public void ConsoleShellEscape()
{
#ifdef UNIX
#ifdef HAVE_TERMIOS_H
  tcsetattr( 0, TCSADRAIN, &ttyOld );
#else
  ioctl( 0, TIOCSETN, &ttyOld );
#endif /* HAVE_TERMIOS_H */
#endif /* UNIX */

#ifdef WIN32NATIVE
  SetConsoleMode(hStdin, oldConsoleMode);
  SetConsoleCursorInfo(hStdout, &oldCursorInfo);
  ConsoleSetCur( 0, HEIGHT - 1 );
#else /* !WIN32NATIVE */
  if( keypad_local )
    tputs( keypad_local, 1, putfunc );
  if( exit_ca_mode )
    tputs( exit_ca_mode, 1, putfunc );
  else
    ConsoleSetCur( 0, HEIGHT - 1 );
#endif /* WIN32NATIVE */

  ConsoleFlush();
}

public void ConsoleReturnToProgram()
{
#ifdef WIN32NATIVE
  SetConsoleMode(hStdin, newConsoleMode);
#else /* !WIN32NATIVE */
  if( keypad_xmit )
    tputs( keypad_xmit, 1, putfunc );
  if( enter_ca_mode )
    tputs( enter_ca_mode, 1, putfunc );
#endif /* WIN32NATIVE */

#ifdef UNIX
#ifdef HAVE_TERMIOS_H 
  tcsetattr( 0, TCSADRAIN, &ttyNew );
#else
  ioctl( 0, TIOCSETN, &ttyNew );
#endif /* HAVE_TERMIOS_H */
#endif /* UNIX */
}

public void ConsoleSuspend()
{
#if !(defined(MSDOS) || defined(WIN32NATIVE)) /* if NOT defind */
  kill(0, SIGSTOP);	/*to pgrp*/
#endif
}

public int ConsoleGetChar()
{
#ifdef WIN32NATIVE
  int c;
  if (bStdinIsConsole) {
      c = getch_replacement_for_msvc();
  } else {
      /* stdin がリダイレクトされている場合（わりと適当） */
      c = fgetc(stdin);
      if (c == '\n') c = '\r';
      if (c == EOF) return EOF;
  }
  return c & 255;
#endif /* WIN32NATIVE */

#ifdef MSDOS
  return getch();
#endif /* MSDOS */

#ifdef UNIX
  byte buf;

  if( 0 > read( 0, &buf, 1 ) )
    return EOF;
  else
    return (int)buf;
#endif /* UNIX */
}

#ifdef MSDOS
union REGS regs;
#endif /* MSDOS */

public int ConsolePrint( byte c )
{
#ifdef MSDOS
  /*
   * fast console output, but remember that you cannot use lv
   * through remote terminals, for example AUX (RS232C).
   * I changed this code for FreeDOS. Because function code No.6
   * with Int 21h on FreeDOS (Alpha 5) doesn't seem to work correctly.
   */
  regs.h.al = c;
  return int86( 0x29, &regs, &regs );
/*
  return (int)bdos( 0x06, 0xff != c ? c : 0, 0 );
*/
#endif /* MSDOS */

#ifdef WIN32NATIVE
  DWORD wsize;
  WriteFile(hStdout, &c, 1, &wsize, NULL);
  return wsize == 1;
#endif /* WIN32NATIVE */

#ifdef UNIX
  return putchar( c );
#endif /* UNIX */
}

public void ConsolePrints( byte *str )
{
#ifdef WIN32NATIVE
  DWORD size;
  size = strlen(str);
  WriteFile(hStdout, str, size, &size, NULL);
#else /* !WIN32NATIVE */
  while( *str )
    ConsolePrint( *str++ );
#endif
}

public void ConsolePrintsStr( str_t *str, int length )
{
  int i;
  byte attr, lastAttr;

  attr = lastAttr = ATTR_NULL;
  for( i = 0 ; i < length ; i++ ){
    attr = ( 0xff00 & str[ i ] ) >> 8;
    if( lastAttr != attr )
      ConsoleSetAttribute( attr );
    lastAttr = attr;
    ConsolePrint( 0xff & str[ i ] );
  }
  if( 0 != attr )
    ConsoleSetAttribute( 0 );
}

public void ConsoleFlush()
{
#ifdef UNIX
  fflush( stdout );
#endif /* UNIX || WIN32NATIVE */
}

public void ConsoleSetCur( int x, int y )
{
#ifdef WIN32NATIVE
  COORD pos;
  pos.X = x;
  pos.Y = y;
  SetConsoleCursorPosition(hStdout, pos);
#endif /* WIN32NATIVE */

#if (defined( MSDOS ) || defined( WIN32 )) && !defined(WIN32NATIVE)
  sprintf( tbuf, "\x1b[%d;%dH", y + 1, x + 1 );
  ConsolePrints( tbuf );
#endif /* MSDOS */

#ifdef TERMCAP
  tputs( tgoto( cursor_address, x, y ), 1, putfunc );
#endif /* TERMCAP */

#ifdef TERMINFO
  tputs( tparm( cursor_address, y, x ), 1, putfunc );
#endif /* TERMINFO */
}

public void ConsoleOnCur()
{
#ifdef WIN32NATIVE
  CONSOLE_CURSOR_INFO info;
  if (GetConsoleCursorInfo(hStdout, &info)) {
    info.bVisible = TRUE;
    SetConsoleCursorInfo(hStdout, &info);
  }
#else
  if( cursor_visible )
    tputs( cursor_visible, 1, putfunc );
#endif
}

public void ConsoleOffCur()
{
#ifdef WIN32NATIVE
  CONSOLE_CURSOR_INFO info;
  if (GetConsoleCursorInfo(hStdout, &info)) {
    info.bVisible = FALSE;
    SetConsoleCursorInfo(hStdout, &info);
  }
#else
  if( cursor_invisible )
    tputs( cursor_invisible, 1, putfunc );
#endif
}

public void ConsoleClearScreen()
{
#ifdef WIN32NATIVE
  COORD                       coordScreen;
  DWORD                       dwCharsWritten;
  DWORD                       dwConsoleSize;
  CONSOLE_SCREEN_BUFFER_INFO  csbi;

  coordScreen.X = 0;
  coordScreen.Y = 0;
  
  /* コンソールのキャラクタバッファ情報を取得 */
  if (GetConsoleScreenBufferInfo(hStdout, &csbi) == FALSE)
    return;

  /* キャラクタバッファサイズを計算 */
  dwConsoleSize = csbi.dwSize.X * csbi.dwSize.Y;

  /* キャラクタバッファを空白で埋める */
  FillConsoleOutputCharacter(
    hStdout, ' ', dwConsoleSize, coordScreen, &dwCharsWritten);

  /* 現在のテキスト属性の取得 */
  if (GetConsoleScreenBufferInfo(hStdout, &csbi) == FALSE)
    return;

  /* すべての文字に対して取得したテキスト属性を適用する */
  FillConsoleOutputAttribute(
    hStdout, csbi.wAttributes, dwConsoleSize, coordScreen, &dwCharsWritten);

  /* カーソル位置を左上角に移動 */
  SetConsoleCursorPosition(hStdout, coordScreen);
#else
  tputs( clear_screen, 1, putfunc );
#endif
}

public void ConsoleClearRight()
{
#ifdef WIN32NATIVE
  COORD                       coordScreen;
  DWORD                       dwCharsWritten;
  DWORD                       dwConsoleSize;
  CONSOLE_SCREEN_BUFFER_INFO  csbi;

  /* コンソールのキャラクタバッファ情報を取得 */
  if (GetConsoleScreenBufferInfo(hStdout, &csbi) == FALSE)
    return;
  coordScreen = csbi.dwCursorPosition;

  /* キャラクタバッファサイズを計算 */
  dwConsoleSize = csbi.dwSize.X - coordScreen.X;

  /* キャラクタバッファを空白で埋める */
  FillConsoleOutputCharacter(
    hStdout, ' ', dwConsoleSize, coordScreen, &dwCharsWritten);

  /* 現在のテキスト属性の取得 */
  if (GetConsoleScreenBufferInfo(hStdout, &csbi) == FALSE)
    return;

  /* すべての文字に対して取得したテキスト属性を適用する */
  FillConsoleOutputAttribute(
    hStdout, csbi.wAttributes, dwConsoleSize, coordScreen, &dwCharsWritten);

  /* カーソル位置をもとに戻す */
  SetConsoleCursorPosition(hStdout, coordScreen);
#else
  tputs( clr_eol, 1, putfunc );
#endif
}

public void ConsoleGoAhead()
{
  ConsolePrint( 0x0d );
}

public void ConsoleScrollUp()
{
#ifdef WIN32NATIVE
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  SMALL_RECT                 region;
  COORD                      dest;
  CHAR_INFO                  fill;
  GetConsoleScreenBufferInfo(hStdout, &csbi);
  fill.Char.AsciiChar = ' ';
  fill.Attributes = csbi.wAttributes;
  region = csbi.srWindow;
  dest = csbi.dwCursorPosition;
  region.Top = dest.Y + 1;
  ScrollConsoleScreenBuffer(hStdout, &region, NULL, dest, &fill);
#else
  if( delete_line )
    tputs( delete_line, 1, putfunc );
#endif
}

public void ConsoleScrollDown()
{
#ifdef WIN32NATIVE
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  SMALL_RECT                 region;
  COORD                      dest;
  CHAR_INFO                  fill;
  GetConsoleScreenBufferInfo(hStdout, &csbi);
  fill.Char.AsciiChar = ' ';
  fill.Attributes = csbi.wAttributes;
  region = csbi.srWindow;
  dest = csbi.dwCursorPosition;
  dest.Y += 1;
  ScrollConsoleScreenBuffer(hStdout, &region, NULL, dest, &fill);
#else
  if( insert_line )
    tputs( insert_line, 1, putfunc );
#endif
}

private byte prevAttr = 0;

public void ConsoleSetAttribute( byte attr )
{
#ifdef WIN32NATIVE
  if( 0 != attr ) {
    WORD w = 0;
    if (ATTR_STANDOUT & attr) {
      w = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
    else if (ATTR_REVERSE & attr) {
      if (ATTR_COLOR_R & attr)
	w |= BACKGROUND_RED;
      if (ATTR_COLOR_G & attr)
	w |= BACKGROUND_GREEN;
      if (ATTR_COLOR_B & attr)
	w |= BACKGROUND_BLUE;
      if (ATTR_BLINK & attr)
	w |= FOREGROUND_RED | BACKGROUND_INTENSITY;
      if (ATTR_UNDERLINE & attr)
	w |= FOREGROUND_BLUE | BACKGROUND_INTENSITY;
      if (ATTR_HILIGHT & attr)
	w |= BACKGROUND_INTENSITY;
    }
    else {
      if (ATTR_COLOR_R & attr)
	w |= FOREGROUND_RED;
      if (ATTR_COLOR_G & attr)
	w |= FOREGROUND_GREEN;
      if (ATTR_COLOR_B & attr)
	w |= FOREGROUND_BLUE;
      if (ATTR_BLINK & attr)
	w |= BACKGROUND_RED | FOREGROUND_INTENSITY;
      if (ATTR_UNDERLINE & attr)
	w |= BACKGROUND_BLUE | FOREGROUND_INTENSITY;
      if (ATTR_HILIGHT & attr)
	w |= FOREGROUND_INTENSITY;
    }
    SetConsoleTextAttribute(hStdout, w);
  }
  prevAttr = attr;
#else /* !WIN32NATIVE */
#ifndef MSDOS /* IF NOT DEFINED */
  if( TRUE == allow_ansi_esc ){
#endif /* MSDOS */
    ConsolePrints( "\x1b[0" );
    if( 0 != attr ){
      if( ATTR_STANDOUT & attr ){
	ConsolePrint( ';' );
	ConsolePrints( ansi_standout );
      } else if( ATTR_COLOR & attr ){
	if( ATTR_REVERSE & attr ){
	  if( ATTR_COLOR_B & attr ){
	    ConsolePrints( ";30;4" );
	    ConsolePrint( ( ATTR_COLOR & attr ) + '0' );
	  } else {
	    ConsolePrints( ";37;4" );
	    ConsolePrint( ( ATTR_COLOR & attr ) + '0' );
	  }
	} else {
	  ConsolePrints( ";3" );
	  ConsolePrint( ( ATTR_COLOR & attr ) + '0' );
	}
      } else if( ATTR_REVERSE & attr ){
	ConsolePrint( ';' );
	ConsolePrints( ansi_reverse );
      }
      if( ATTR_BLINK & attr ){
	ConsolePrint( ';' );
	ConsolePrints( ansi_blink );
      }
      if( ATTR_UNDERLINE & attr ){
	ConsolePrint( ';' );
	ConsolePrints( ansi_underline );
      }
      if( ATTR_HILIGHT & attr ){
	ConsolePrint( ';' );
	ConsolePrints( ansi_hilight );
      }
    }
    ConsolePrint( 'm' );
#ifndef MSDOS /* IF NOT DEFINED */
  } else {
    /*
     * non ansi sequence
     */
    if( ( ATTR_HILIGHT & prevAttr ) && 0 == ( ATTR_HILIGHT & attr ) )
      if( exit_attribute_mode )
	tputs( exit_attribute_mode, 1, putfunc );
    if( ( ATTR_UNDERLINE & prevAttr ) && 0 == ( ATTR_UNDERLINE & attr ) )
      if( exit_underline_mode )
	tputs( exit_underline_mode, 1, putfunc );
    if( ( ATTR_STANDOUT & prevAttr ) && 0 == ( ATTR_STANDOUT & attr ) )
      if( exit_standout_mode )
	tputs( exit_standout_mode, 1, putfunc );

    if( ATTR_HILIGHT & attr )
      if( enter_bold_mode )
	tputs( enter_bold_mode, 1, putfunc );
    if( ATTR_UNDERLINE & attr )
      if( enter_underline_mode )
	tputs( enter_underline_mode, 1, putfunc );
    if( ATTR_STANDOUT & attr )
      if( enter_standout_mode )
	tputs( enter_standout_mode, 1, putfunc );
  }
  prevAttr = attr;
#endif /* MSDOS */
#endif /* WIN32NATIVE */
}
