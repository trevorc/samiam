/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2007 Trevor Caira, Jimmy Hartzell
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

#ifndef LIBSAM_ARRAY_H
#define LIBSAM_ARRAY_H

/** Safe, dynamically allocating array type. */
typedef struct {
    size_t len;	    /**< Number of elements used in the array. */
    size_t alloc;   /**< Allocated size (in number of elements) of the
		     *	 array. */
    void **restrict arr;    /**< The array itself. */
} sam_array;

extern void sam_array_init(/*@out@*/ sam_array *restrict a);
extern void sam_array_ins(/*@in@*/ sam_array *restrict a, /*@only@*/ void *restrict m);
/*@only@*/ /*@null@*/ extern inline void *sam_array_rem(sam_array *restrict a);
extern void sam_array_free(sam_array *restrict a);

#endif /* LIBSAM_ARRAY_H */
