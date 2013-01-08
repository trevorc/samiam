/*
 * sam_util.c       arrays, strings, and a safe malloc(3).
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

#include "libsam.h"

#include <stdlib.h>
#include "util.h"

#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

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

    if ((p = malloc(size)) == NULL) {
#if defined(HAVE_UNISTD_H)
	write(STDERR_FILENO, "no memory", 10);
#endif
	abort();
    }

    return p;
}

/*@only@*/ /*@notnull@*/ void *
sam_realloc(/*@only@*/ void *p,
	    size_t size)
{
    if ((p = realloc(p, size)) == NULL) {
	free(p);
#if defined(HAVE_UNISTD_H)
	write(STDERR_FILENO, "no memory", 10);
#endif
	abort();
    }
    return p;
}
