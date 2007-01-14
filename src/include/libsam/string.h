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

#ifndef LIBSAM_STRING_H
#define LIBSAM_STRING_H

/** Safe, dynamically allocating string type. */
typedef struct {
    size_t len;	    /**< Number of bytes allocated for the string (not
		     **	 including the trailing NUL inserted by
		     **	 #sam_string_ins()) */
    size_t alloc;   /**< Number of bytes allocated for the string
		     *	 including for the trailing NUL */
    char *data;	    /**< The C string character array. */
} sam_string;

extern void sam_string_init(/*@out@*/ sam_string *restrict s);
extern void sam_string_free(sam_string *restrict s);
/*@null@*/ extern char *sam_string_get(/*@in@*/ FILE *restrict in, /*@out@*/ sam_string *restrict s);
/*@null@*/ extern char *sam_string_read(/*@in@*/ FILE *restrict in, /*@out@*/ sam_string *restrict s);

#endif /* LIBSAM_STRING_H */
