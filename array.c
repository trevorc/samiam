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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libsam.h"

#include "array.h"
#include "util.h"

/** Initial size for array allocations. */
static const size_t SAM_INIT_ALLOC = 4;

void
sam_array_init(sam_array *restrict a)
{
    a->alloc = SAM_INIT_ALLOC;
    a->len = 0;
    a->arr = sam_malloc(sizeof (*a->arr) * SAM_INIT_ALLOC);
}

void
sam_array_ins(/*@in@*/	 sam_array *restrict a,
	      /*@only@*/ void *restrict m)
{
    ++a->len;
    if (a->alloc < a->len) {
	while (a->alloc < a->len) {
	    a->alloc *= 2;
	}
	a->arr = sam_realloc(a->arr, sizeof (*a->arr) * a->alloc);
    }
    a->arr[a->len - 1] = m;
}

/*@null@*/ inline void *
sam_array_rem(/*@in@*/ sam_array *restrict a)
{
    if (a->len < a->alloc / 4) {
	a->alloc /= 2;
	a->arr = sam_realloc(a->arr, sizeof (*a->arr) * a->alloc);
    }

    return a->len == 0? NULL: a->arr[--a->len];
}

void
sam_array_free(sam_array *restrict a)
{
    for (size_t i = 0; i < a->len; ++i) {
	free(a->arr[i]);
    }
    free(a->arr);
}
