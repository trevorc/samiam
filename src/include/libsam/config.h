/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2006 Trevor Caira, Jimmy Hartzell, Daniel Perelman
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
 * Revision 1.2  2007/01/04 05:37:02  trevor
 * Reformat.
 *
 * Revision 1.1  2006/12/25 00:21:39  trevor
 * New SDK suite of headers.
 *
 * Revision 1.6  2006/12/21 04:07:12  trevor
 * Even though sam_ha and sam_sa's are size_t, they're not allowed to
 * hold more than a long can store.
 *
 * Revision 1.5  2006/12/19 09:29:34  trevor
 * Removed implicit casts.
 *
 * Revision 1.4  2006/12/19 08:33:56  anyoneeb
 * Some splint warning fixes.
 *
 * Revision 1.3  2006/12/17 02:53:26  trevor
 * UNUSED was unused. so i removd it.
 *
 * Revision 1.2  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef LIBSAM_CONFIG_H
#define LIBSAM_CONFIG_H

#include <float.h>
#include <limits.h>

#define HAVE_MMAN_H	1
#define HAVE_UNISTD_H	1
#define HAVE_LOCALE_H	1
#define SAM_EXTENSIONS	1
#define HAVE_DLFCN_H	1

#if !defined(__WORDSIZE)
# define __WORDSIZE 32
#endif /* !__WORDSIZE */

#if __WORDSIZE == 64
typedef double sam_float;
# define SAM_EPSILON DBL_EPSILON
#else
typedef float sam_float;
# define SAM_EPSILON FLT_EPSILON
#endif /* __WORDSIZE == 64 */

/** Greatest value a sam_heap_address can hold. */
#define SAM_HEAP_PTR_MAX LONG_MAX

/** Greatest value a sam_stack_address can hold. */
#define SAM_STACK_PTR_MAX LONG_MAX

#endif /* LIBSAM_CONFIG_H */
