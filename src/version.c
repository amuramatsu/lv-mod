/*
 * version.c
 *
 * All rights reserved. Copyright (C) 1996 by NARITA Tomio.
 * $Id: version.c,v 1.6 2004/01/05 07:21:26 nrt Exp $
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

#include <import.h>
#include <begin.h>
#include <version.h>

public void Banner()
{
  fprintf( stderr,
	  "# lv " VERSION "\n"
	  "# All rights reserved. Copyright (C) 1996-2005 by NARITA Tomio\n"
	  "# ABSOLUTELY NO WARRANTY; for details type `lv -h'\n"
	  );
}
