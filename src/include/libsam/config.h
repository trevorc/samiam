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
 */

#ifndef LIBSAM_CONFIG_H
#define LIBSAM_CONFIG_H

#include <float.h>
#include <limits.h>

#define SAM_EXTENSIONS	1

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
