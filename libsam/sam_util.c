/*
 * sam_util.c       arrays, strings, and a safe malloc(3).
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
 * Revision 1.5  2006/12/19 18:17:53  trevor
 * Moved parts of sam_util.h which were library-generic into libsam.h.
 *
 * Revision 1.4  2006/12/19 09:29:34  trevor
 * Removed implicit casts.
 *
 * Revision 1.3  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_UNISTD_H) || defined(HAVE_MMAN_H)
# include <unistd.h>
#endif /* HAVE_UNISTD_H || HAVE_MMAN_H */

#include "sam_util.h"

/** Initial size for array and string allocations. */
static const size_t INIT_ALLOC = 4;

/**
 * A wrapper around malloc(3) guaranteed to be safe. Calls abort()
 * when memory cannot be allocated.
 *
 *  @param size The number of bytes to allocate.
 *
 *  @return A pointer to the allocated space on the heap.
 */
/*@out@*/ /*@only@*/ /*@notnull@*/ void *
sam_malloc(size_t size)
{
    /*@out@*/ void *p;

    if (size == 0) { 
	fputs("Do you realize what you have done?", stderr);
    }
    if ((p = malloc(size)) == NULL) {
#if defined(HAVE_UNISTD_H)
	write(STDERR_FILENO, "no memory\n", 10);
#endif
	abort();
    }

    return p;
}

/*@only@*/ /*@notnull@*/ void *
sam_realloc(/*@only@*/ void   *p,
		       size_t  size)
{
    if ((p = realloc(p, size)) == NULL) {
	free(p);
#if defined(HAVE_UNISTD_H)
	write(STDERR_FILENO, "no memory\n", 10);
#endif
	abort();
    }
    return p;
}

void
sam_array_init(sam_array *a)
{
    a->alloc = INIT_ALLOC;
    a->len = 0;
    a->arr = sam_malloc(sizeof(*a->arr) * INIT_ALLOC);
}

void
sam_array_ins(/*@in@*/	 sam_array *a,
	      /*@only@*/ void *m)
{
    ++a->len;
    if (a->alloc < a->len) {
	while(a->alloc < a->len) {
	    a->alloc *= 2;
	}
	a->arr = sam_realloc(a->arr, sizeof(*a->arr) * a->alloc);
    }
    a->arr[a->len - 1] = m;
}

/*@null@*/ void *
sam_array_rem(/*@in@*/ sam_array *a)
{
    if (a->len == 0) {
	return NULL;
    }
    return a->arr[--a->len];
}

void
sam_array_free(sam_array *a)
{
    size_t i;

    for (i = 0; i < a->len; ++i) {
	free(a->arr[i]);
    }
    free(a->arr);
}

/*
 * sam_string: safe, dynamically reallocating string
 */

void
sam_string_init(/*@out@*/ sam_string *s)
{
    s->data = sam_malloc(INIT_ALLOC);
    s->alloc = INIT_ALLOC;
    s->len = 0;
}

void
sam_string_ins(sam_string *s,
	       char	  *src,
	       size_t	   n)
{
    s->len += n;
    if (s->alloc < s->len + 1) {
	while(s->alloc < s->len + 1) {
	    s->alloc *= 2;
	}
	s->data = sam_realloc(s->data, s->alloc * sizeof(*s->data));
    }
    memcpy(s->data + (s->len - n), src, n);
    s->data[s->len] = '\0';
}

void
sam_string_free(sam_string *s)
{
    free(s->data);
}

/*@null@*/ char *
sam_string_read(/*@in@*/  FILE *in,
		/*@out@*/ sam_string *s)
{
    sam_string_init(s);

    for (;;) {
	char buf[16];
	size_t n = fread(buf, sizeof (char), 16, in);

	if (n == 0) {
	    if (ferror(in)) {
		perror("fread");
		sam_string_free(s);
		return NULL;
	    }
	    break;
	} else {
	    sam_string_ins(s, buf, n);
	    if (n < 16) {
		break;
	    }
	}
    }

    return s->data;
}
