/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2006 Trevor Caira, Jimmy Hartzell
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * $Log$
 * Revision 1.4  2007/01/04 05:40:57  trevor
 * Use new io funcs.
 *
 * Revision 1.3  2006/12/27 18:47:21  trevor
 * Fixed types.
 *
 * Revision 1.2  2006/12/12 23:31:35  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef LIBJBR_H
#define LIBJBR_H

#include <libsam/io.h>

extern int   jbr_vfprintf(sam_io_stream ios,
			  const char *restrict fmt,
			  va_list ap);
extern int   jbr_vfscanf(sam_io_stream ios,
			 const char *restrict fmt,
			 va_list ap);
extern char *jbr_afgets(char **restrict s,
			sam_io_stream ios);

#endif /* LIBJBR_H */
