/*
 * screen.c
 *
 * All rights reserved. Copyright (C) 1996 by NARITA Tomio.
 * $Id: screen.c,v 1.4 2004/01/05 07:36:02 nrt Exp $
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

#include <stdlib.h>

#include <import.h>
#include <position.h>
#include <fetch.h>
#include <console.h>
#include <begin.h>
#include <screen.h>

#define ScreenSetBoth( f, seg, blk, off, phy )				\
{									\
  PositionSet( (f)->screen.top, (seg), (blk), (off), (phy) );		\
  PositionSet( (f)->screen.bot, (seg), (blk), (off), (phy) );		\
}

#define ScreenDecTop( f )						\
  PositionDec( (f), (f)->screen.top.seg, (f)->screen.top.blk, (f)->screen.top.off, (f)->screen.top.phy )

#define ScreenDecBot( f )						\
  PositionDec( (f), (f)->screen.bot.seg, (f)->screen.bot.blk, (f)->screen.bot.off, (f)->screen.bot.phy )

#define ScreenIncTop( f )						\
  PositionInc( (f), (f)->screen.top.seg, (f)->screen.top.blk, (f)->screen.top.off, (f)->screen.top.phy )

#define ScreenIncBot( f )						\
  PositionInc( (f), (f)->screen.bot.seg, (f)->screen.bot.blk, (f)->screen.bot.off, (f)->screen.bot.phy )

/*
 * $B2hLL$N0LCV$N@)8f(B
 * $BA0$N%Z!<%8(B, $B<!$N%Z!<%8(B, $BO@M}9T$N2?9TL\(B, $BJ*M}9T$N2?9TL\(B, $B$H$$$C$?CM$G(B
 * $B%Z!<%8$r%a%b%j$X%m!<%I$9$k(B.
 */

public unsigned int ScreenPrev( file_t *f, int physical )
{
  int i;

  for( i = 0 ; i < physical ; i++ ){
    ScreenDecTop( f );
    f->screen.lines++;
    if( f->screen.lines > f->height ){
      ScreenDecBot( f );
      f->screen.lines--;
    }
  }

  if( i < physical )
    f->top = TRUE;
  else
    f->top = FALSE;

  if( f->screen.lines < f->height )
    f->bottom = TRUE;
  else
    f->bottom = FALSE;

  f->find.pos = f->screen.top;

  return i;
}

public unsigned int ScreenNext( file_t *f, int physical )
{
  int i;

  for( i = 0 ; i < physical ; i++ ){
    ScreenIncBot( f );
    f->screen.lines++;
    if( f->screen.lines > f->height ){
      ScreenIncTop( f );
      f->screen.lines--;
    }
  }

  if( i < physical ){
    f->top = FALSE;
    f->bottom = TRUE;
  } else {
    f->top = FALSE;
    f->bottom = FALSE;
  }

  f->find.pos = f->screen.top;

  return i;
}

public boolean_t ScreenBot( file_t *f )
{
  int i;
  unsigned int seg;
  int blk, off, phy;

  if( FALSE == f->done ){
    ConsoleEnableInterrupt();
    FileStretch( f, LV_PAGE_SIZE * SLOT_SIZE * FRAME_SIZE - 1 );
    ConsoleDisableInterrupt();
  }

  if( FALSE == FetchLine( f, f->lastSegment - 1, 0 ) ){
    /*
     * null file
     */
    f->top = FALSE;
    f->bottom = TRUE;
    f->screen.lines = 0;
    ScreenSetBoth( f, 0, 0, 0, 0 );
    f->find.pos = f->screen.top;
    return FALSE;
  }

  seg = f->lastSegment - 1;
  blk = Block( seg );
  off = f->page[ blk ].lines - 1;
  phy = f->page[ blk ].line[ off ].heads - 1;

  ScreenSetBoth( f, seg, blk, off, phy );

  for( i = 0 ; i < ( f->height - 1 ) ; i++ )
    ScreenDecTop( f );

  f->top = FALSE;
  f->bottom = TRUE;
  f->screen.lines = i + 1;
  f->find.pos = f->screen.top;

  if( TRUE == kb_interrupted )
    return FALSE;

  return TRUE;
}

public boolean_t ScreenTop( file_t *f, unsigned int logical )
{
  int i;
  unsigned int seg;
  int blk, off, phy;
  boolean_t res;

  if( logical > 0 )
    logical--;

  ConsoleEnableInterrupt();
  res = FetchLine( f, Segment( logical ), Offset( logical ) );
  ConsoleDisableInterrupt();

  if( FALSE == res ){
    return ScreenBot( f );
  }

  seg = Segment( logical );
  blk = Block( seg );
  off = Offset( logical );
  phy = 0;

  ScreenSetBoth( f, seg, blk, off, phy );

  for( i = 0 ; i < ( f->height - 1 ) ; i++ )
    ScreenIncBot( f );

  if( 0 == logical )
    f->top = TRUE;
  else
    f->top = FALSE;

  if( i < f->height - 1 )
    f->bottom = TRUE;
  else
    f->bottom = FALSE;

  f->screen.lines = i + 1;
  f->find.pos = f->screen.top;

  if( TRUE == kb_interrupted )
    return FALSE;

  return TRUE;
}

public boolean_t ScreenTopPhysical( file_t *f, position_t *pos )
{
  int i;

  if( FALSE == FetchLine( f, pos->seg, pos->off ) )
    return ScreenBot( f );

  f->screen.top.seg = f->screen.bot.seg = pos->seg;
  f->screen.top.blk = f->screen.bot.blk = pos->blk;
  f->screen.top.off = f->screen.bot.off = pos->off;
  f->screen.top.phy = f->screen.bot.phy = pos->phy;

  for( i = 0 ; i < ( f->height - 1 ) ; i++ )
    ScreenIncBot( f );

  if( f->screen.top.seg == 0 && f->screen.top.off == 0 && f->screen.top.phy == 0 )
    f->top = TRUE;
  else
    f->top = FALSE;

  if( i < f->height - 1 )
    f->bottom = TRUE;
  else
    f->bottom = FALSE;

  f->screen.lines = i + 1;
  f->find.pos = f->screen.top;

  return TRUE;
}

/*
 * $B2hLL%5%$%:$r<hF@$7$F(B file_t $B9=B$BN$X3JG<$9$k(B.
 * $B$=$N2hLL$N%j%5%$%:$KJ*M}9T4IM}$r@09g$5$;$k$?$a$K=>Mh$N@hF,9T$r(B
 * $BO@M}9T$G;XDj$7(B, $B%U%!%$%k$r%a%b%j$X:F%m!<%I$9$k(B.
 *
 * $B"((B $B%Z!<%8!&%-%c%C%7%e$r=i4|2=$7$F$+$i8F$V$3$H(B.
 */

public void ScreenRefresh( file_t *f )
{
  unsigned int logical;

  ConsoleGetWindowSize();

  if( HEIGHT > LV_PAGE_SIZE * ( BLOCK_SIZE - 1 ) )
    HEIGHT = LV_PAGE_SIZE * ( BLOCK_SIZE - 1 );

  f->width = WIDTH;
  f->height = HEIGHT - 1;

  logical = 1 + f->screen.top.seg * LV_PAGE_SIZE + f->screen.top.off;
  ScreenTop( f, logical );
}
