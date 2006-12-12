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
 * Revision 1.2  2006/12/12 23:31:36  trevor
 * Added the $Id$ and $Log$ tags and copyright notice where they were missing.
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <float.h>

#define HAVE_MMAN_H	1
#define HAVE_UNISTD_H	1
#define HAVE_LOCALE_H	1
#undef HAVE_DLFCN_H

#undef SAM_EXTENSIONS

#if !defined(__WORDSIZE)
# define __WORDSIZE 32
#endif /* !__WORDSIZE */

/* define to float on 32 bit architectures, double on 64 bit. */
#if __WORDSIZE == 64
typedef double sam_float;
# define SAM_EPSILON DBL_EPSILON
#else
typedef float sam_float;
# define SAM_EPSILON FLT_EPSILON
#endif /* __WORDSIZE == 64 */

#if defined(__GNUC__)
# define UNUSED /*@unused@*/ __attribute__((__unused__))
#else 
# define UNUSED /*@unused@*/
#endif /* __GNUC__ */

#endif /* CONFIG_H */
