/*
 * file.h
 *
 * All rights reserved. Copyright (C) 1996 by NARITA Tomio
 * $Id: file.h,v 1.8 2004/01/05 07:30:15 nrt Exp $
 */

#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <stdlib.h>

#include <itable.h>
#include <ctable.h>
#include <str.h>
#include <stream.h>

#define LV_PAGE_SIZE	32U		/* lines per page */

#ifdef MSDOS
#define BLOCK_SIZE	2		/* segments on memory */
#define SLOT_SIZE	1024U		/* file location pointers */
#define FRAME_SIZE	2U
#else
#define BLOCK_SIZE	4		/* segments on memory */
#define SLOT_SIZE	16384U		/* file location pointers */
#define FRAME_SIZE	4096U
#endif /* MSDOS */

#ifdef USE_INTERNAL_IOBUF
# define IOBUF_DEFAULT_SIZE	256
#endif

typedef struct {
  int		ptr;
  int		width;
  boolean_t	hit;
} head_t;

typedef struct {
  i_str_t	*istr;
  int		heads;			/* physical line number */
  head_t	*head;
} line_t;

typedef struct {
  unsigned int	segment;
  int		lines;			/* logical line number */
  line_t	line[ LV_PAGE_SIZE ];
} page_t;

typedef struct {
  unsigned int	seg;
  int		blk;
  int		off;
  int		phy;
} position_t;

typedef struct {
  position_t	top;
  position_t	bot;
  int		lines;
} screen_t;

typedef struct {
  position_t	pos;
  i_str_t	*pattern;
  boolean_t	first;
  boolean_t	displayed;
  boolean_t	before_direction;
} find_t;

typedef struct {
  FILE		*iop;
#ifdef USE_INTERNAL_IOBUF
  int		ungetc;
  byte		buf[ IOBUF_DEFAULT_SIZE ];
  size_t	cur;
  size_t	last;
#endif
} iobuf_t;

#if defined(WIN32NATIVE)
typedef __int64	offset_t;
#elif defined(HAVE_FSEEKO)
typedef off_t	offset_t;
#else
typedef long	offset_t;
#endif

typedef struct {
  byte		*fileName;
  i_str_t	*fileNameI18N;
  int		fileNameLength;
  iobuf_t	fp;
  iobuf_t	sp;
  int		pid;
  byte		inputCodingSystem;
  byte		outputCodingSystem;
  byte		keyboardCodingSystem;
  byte		pathnameCodingSystem;
  byte		defaultCodingSystem;
  byte		dummy;
  int		width;
  int		height;
  unsigned int	lastSegment;
  unsigned int	lastFrame;
  unsigned long	totalLines;
  offset_t	lastPtr;
  boolean_t	done;
  boolean_t	eof;
  boolean_t	top;
  boolean_t	bottom;
  boolean_t	truncated;
  boolean_t	dirty;
  find_t	find;
  screen_t	screen;
  boolean_t	used[ BLOCK_SIZE ];
  page_t	page[ BLOCK_SIZE ];
  offset_t	*slot[ FRAME_SIZE ];
} file_t;

#ifdef MSDOS
#define line_t line_t far
#define page_t page_t far
#define file_t file_t far
#else
#ifndef far
#define far
#endif /* far */
#endif /* MSDOS */

#define Segment( line )		( (line) / LV_PAGE_SIZE )
#define Offset( line )		( (line) % LV_PAGE_SIZE )
#define Block( segment )	( (segment) % BLOCK_SIZE )
#define Slot( segment )		( (segment) % SLOT_SIZE )
#define Frame( segment )	( (segment) / SLOT_SIZE )

public void FileFreeLine( file_t *f );
public byte *FileLoadLine( file_t *f, int *length, boolean_t *simple );

public file_t *FileAttach( byte *fileName, stream_t *st,
			  int width, int height,
			  byte inputCodingSystem,
			  byte outputCodingSystem,
			  byte keyboardCodingSystem,
			  byte pathnameCodingSystem,
			  byte defaultCodingSystem );
public void FilePreload( file_t *f );

public boolean_t FileFree( file_t *f );
public boolean_t FileDetach( file_t *f );

public boolean_t FileStretch( file_t *f, unsigned int target );
public boolean_t FileSeek( file_t *f, unsigned int segment );

public void FileRefresh( file_t *f );
public byte *FileStatus( file_t *f );
public byte *FileName( file_t *f );

public void FileInit();

#ifndef USE_INTERNAL_IOBUF
# if defined(WIN32NATIVE)
#  define IobufFtell( a )	_ftelli64( (a)->iop )
#  define IobufFseek( a, b, c )	_fseeki64( (a)->iop, b, c)
# elif defined(HAVE_FSEEKO)
#  define IobufFtell( a )	ftello( (a)->iop )
#  define IobufFseek( a, b, c )	fseeko( (a)->iop, b, c)
# else
#  define IobufFtell( a )	ftell( (a)->iop )
#  define IobufFseek( a, b, c )	fseek( (a)->iop, b, c)
# endif
#else
public offset_t IobufFtell( iobuf_t *iobuf );
public int IobufFseek( iobuf_t *iobuf, offset_t off, int mode );
#endif

#endif /* __FILE_H__ */
