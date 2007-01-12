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
 * Revision 1.3  2007/01/06 01:09:05  trevor
 * Remove free(3) decl.
 *
 * Revision 1.2  2007/01/04 05:39:33  trevor
 * Split off array, string, and hash table code.
 *
 * Revision 1.1  2006/12/25 00:21:39  trevor
 * New SDK suite of headers.
 *
 * Revision 1.9  2006/12/19 18:17:53  trevor
 * Moved parts of sam_util.h which were library-generic into libsam.h.
 *
 * Revision 1.8  2006/12/19 18:01:22  trevor
 * Moved sam_exit_code to sam.h.
 *
 * Revision 1.7  2006/12/19 07:28:09  anyoneeb
 * Split sam_value into sam_op_value and sam_ml_value.
 *
 * Revision 1.6  2006/12/19 05:41:15  anyoneeb
 * Sepated operand types from memory types.
 *
 * Revision 1.5  2006/12/19 03:19:19  trevor
 * fixed doc for TYPE_STR
 *
 * Revision 1.4  2006/12/17 00:15:42  trevor
 * Added two new types: TYPE_HA and TYPE_SA. Removed TYPE_MA. Shifted values
 * of types down. Removed dynamic loading defs.
 *
 * Revision 1.3  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef LIBSAM_UTIL_H
#define LIBSAM_UTIL_H

#include <stdlib.h>

/**
 *  Wrap malloc(3). Calls abort(3) if there is no memory available.
 *
 *  @param size The amount to allocate.
 *
 *  @return A pointer to the start of the allocated block.
 */
/*@out@*/ /*@only@*/ /*@notnull@*/ extern void *sam_malloc(size_t size);
/*@only@*/ /*@notnull@*/ extern void *sam_realloc(/*@only@*/ void *restrict p, size_t size);

#endif /* LIBSAM_UTIL_H */
