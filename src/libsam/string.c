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

#include <libsam/util.h>
#include <libsam/string.h>

/** Initial size for string allocations. */
static const size_t SAM_INIT_ALLOC = 10;

/*
 * sam_string: safe, dynamically reallocating string
 */

void
sam_string_init(/*@out@*/ sam_string *s)
{
    s->data = sam_malloc(SAM_INIT_ALLOC);
    *s->data = '\0';
    s->alloc = SAM_INIT_ALLOC;
    s->len = 0;

#if defined(HAVE_MMAN_H)
    s->mmapped = false;
#endif /* HAVE_MMAN_H */
}

static void
sam_string_ins(sam_string *restrict s,
	       char *restrict src,
	       size_t n)
{
    s->len += n;
    if (s->alloc < s->len + 1) {
	while (s->alloc < s->len + 1) {
	    s->alloc *= 2;
	}
	s->data = sam_realloc(s->data, s->alloc * sizeof (*s->data));
    }
    memcpy(s->data + (s->len - n), src, n);
    s->data[s->len] = '\0';
}

void
sam_string_free(sam_string *restrict s)
{
    free(s->data);
}

/*@null@*/ char *
sam_string_get(/*@in@*/  FILE *restrict in,
	       /*@out@*/ sam_string *restrict s)
{
    sam_string_init(s);
    for (;;) {
	int c = fgetc(in);

	if (c == EOF) {
	    if (ferror(in)) {
		perror("fgetc");
		sam_string_free(s);
		return NULL;
	    }
	    break;
	}
	c = c == '\n'? '\0': c;
	sam_string_ins(s, (char *)&c, 1);
	if (c == '\0') {
	    break;
	}
    }

    return s->data;
}

/*@null@*/ char *
sam_string_read(/*@in@*/  FILE *restrict in,
		/*@out@*/ sam_string *restrict s)
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
	}
	sam_string_ins(s, buf, n);
	if (n < 16) {
	    break;
	}
    }

    return s->data;
}
