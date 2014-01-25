/*
 * version.h
 *
 * All rights reserved. Copyright (C) 1996 by NARITA Tomio
 * $Id: version.h,v 1.24 2004/01/16 12:25:57 nrt Exp $
 */

#ifndef __VERSION_H__
#define __VERSION_H__

#define VERSION		"v.4.51 (Jan.16th,2004)"
#if defined(WIN32NATIVE) || defined(USE_UTF16)
#define PATCH_VERSION	"+002 (Nov.3rd,2011)"
#endif

public void Banner();

#endif /* __VERSION_H__ */
