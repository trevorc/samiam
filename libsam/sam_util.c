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
 * Revision 1.6  2006/12/25 00:29:54  trevor
 * Update for new hash table labels.
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libsam/config.h>
#include <libsam/util.h>

#if defined(HAVE_UNISTD_H) || defined(HAVE_MMAN_H)
# include <unistd.h>
#endif /* HAVE_UNISTD_H || HAVE_MMAN_H */

/** Initial size for array and string allocations. */
static const size_t SAM_INIT_ALLOC = 4;

/** Initial size for hash tables. */
static const size_t SAM_INIT_HASH_ALLOC = 64;

/** Big prime for computing hashes. */
static const unsigned long SAM_BIG_PRIME = 6291469UL;

/** Maximum load value for the hash table. */
static const float SAM_MAX_LOAD = 0.5;

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

void *
sam_calloc(size_t nmemb,
	   size_t size)
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL) {
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
    a->alloc = SAM_INIT_ALLOC;
    a->len = 0;
    a->arr = sam_malloc(sizeof(*a->arr) * SAM_INIT_ALLOC);
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
    s->data = sam_malloc(SAM_INIT_ALLOC);
    s->alloc = SAM_INIT_ALLOC;
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

void
sam_hash_table_init(sam_hash_table *h)
{
    h->alloc = SAM_INIT_HASH_ALLOC;
    h->nmemb = 0;
    h->arr = sam_calloc(sizeof (*h->arr), SAM_INIT_HASH_ALLOC);
}

static size_t
sam_hash(const char *key,
	 size_t      alloc)
{
    size_t hash = 0;

    while (*key != '\0') {
	hash += (hash << 5) + *key++;
    }
    hash *= SAM_BIG_PRIME;

    return hash % alloc;
}

static void 
sam_hash_table_double(sam_hash_table *h)
{
    sam_hash_table tmp;
    size_t	   i;

    tmp.alloc = h->alloc * 2;
    tmp.nmemb = h->nmemb;
    tmp.arr = sam_calloc(tmp.alloc, sizeof (*tmp.arr));

    for (i = 0; i < h->alloc; ++i) {
	if (h->arr[i].key != NULL) {
	    sam_hash_table_ins(&tmp, h->arr[i].key, h->arr[i].value);
	}
    }

    free(h->arr);
    h->arr = tmp.arr;
    h->alloc = tmp.alloc;
}

sam_bool
sam_hash_table_ins(sam_hash_table *h,
		   char		  *key,
		   size_t	   value)
{
    size_t hash;

    ++h->nmemb;
    while (((float)h->nmemb) / h->alloc > SAM_MAX_LOAD) {
	sam_hash_table_double(h);
    }

    hash = sam_hash(key, h->alloc);
    for (;;) {
	if (h->arr[hash].key == NULL) {
	    h->arr[hash].key = key;
	    h->arr[hash].value = value;
	    return TRUE;
	} else if (strcmp(key, h->arr[hash].key) == 0) {
	    return FALSE;
	} else if (hash == h->alloc - 1) {
	    hash = 0;
	} else {
	    ++hash;
	}
    }
}

sam_bool
sam_hash_table_get(sam_hash_table *h,
		   const char	  *key,
		   size_t	  *value)
{
    size_t hash = sam_hash(key, h->alloc);
    size_t start = hash;
    sam_bool just_started = TRUE;

    *value = -1;
    for (;;) {
	if (h->arr[hash].key == NULL) {
	    return FALSE;
	} else if (start == hash && !just_started) {
	    return FALSE;
	} else if (strcmp(h->arr[hash].key, key) == 0) {
	    *value = h->arr[hash].value;
	    return TRUE;
	} else if (hash == h->alloc - 1) {
	    hash = 0;
	} else {
	    just_started = FALSE;
	    ++hash;
	}
    }
}

void
sam_hash_table_free(sam_hash_table *h)
{
    free(h->arr);
}
