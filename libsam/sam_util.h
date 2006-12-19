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

#ifndef SAM_UTIL_H
#define SAM_UTIL_H

/** Safe, dynamically allocating array type. */
typedef struct {
    size_t len;	    /**< Number of elements used in the array. */
    size_t alloc;   /**< Allocated size (in number of elements) of the
		     *	 array. */
    void **arr;	    /**< The array itself. */
} sam_array;

/** Safe, dynamically allocating string type. */
typedef struct {
    size_t len;	    /**< Number of bytes allocated for the string (not
		     **	 including the trailing NUL inserted by
		     **	 #sam_string_ins()) */
    size_t alloc;   /**< Number of bytes allocated for the string
		     *	 including for the trailing NUL */
    char *data;	    /**< The C string character array. */
} sam_string;

/**
 *  Wrap malloc(3). Calls abort(3) if there is no memory available.
 *
 *  @param size The amount to allocate.
 *
 *  @return A pointer to the start of the allocated block.
 */
/*@out@*/ /*@only@*/ /*@notnull@*/ extern void *sam_malloc(size_t size);

extern void free(/*@only@*/ /*@out@*/ /*@null@*/ void *p);

/*@only@*/ /*@notnull@*/ extern void *sam_realloc(/*@only@*/ void *p, size_t size);
extern void sam_array_init(/*@out@*/ sam_array *a);
extern void sam_array_ins(/*@in@*/ sam_array *a, /*@only@*/ void *m);
/*@only@*/ /*@null@*/ extern void *sam_array_rem(sam_array *a);
extern void sam_array_free(sam_array *a);
extern void sam_string_init(/*@out@*/ sam_string *s);
extern void sam_string_ins(/*@in@*/ sam_string *s, char *src, size_t n);
extern void sam_string_free(sam_string *s);
/*@null@*/ extern char *sam_string_read(/*@in@*/ FILE *in, /*@out@*/ sam_string *s);

#endif /* SAM_UTIL_H */
