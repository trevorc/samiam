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

#include "libsam.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libsam/util.h>
#include <libsam/hash_table.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

/** Initial size for hash table allocations. */
static const size_t SAM_INIT_ALLOC = 64;

/** Big prime for computing hashes. */
static const unsigned long SAM_BIG_PRIME = 6291469UL;

/** Maximum load value for the hash table. */
static const float SAM_MAX_LOAD = 0.5;

/*@only@*/ /*@notnull@*/ static void *
sam_calloc(size_t nmemb,
	   size_t size)
{
    void *restrict p = calloc(nmemb, size);

    if (p == NULL) {
#if defined(HAVE_UNISTD_H)
	write(STDERR_FILENO, "no memory\n", 10);
#endif
	abort();
    }

    return p;
}

void
sam_hash_table_init(sam_hash_table *restrict h)
{
    h->alloc = SAM_INIT_ALLOC;
    h->nmemb = 0;
    h->arr = sam_calloc(sizeof (*h->arr), SAM_INIT_ALLOC);

    for (size_t i = 0; i < h->alloc; ++i) {
	h->arr[i].value = NULL;
    }
}

static size_t
sam_hash(const char *restrict key,
	 size_t alloc)
{
    size_t hash = 0;

    while (*key != '\0') {
	hash += (hash << 5) + *key++;
    }
    hash *= SAM_BIG_PRIME;

    return hash % alloc;
}

static void
sam_hash_table_double(sam_hash_table *restrict h)
{
    sam_hash_table tmp;
    size_t i;

    tmp.alloc = h->alloc * 2;
    tmp.nmemb = h->nmemb;
    tmp.arr = sam_calloc(tmp.alloc, sizeof (*tmp.arr));

    for (size_t j = 0; j < h->alloc; ++j) {
	tmp.arr[j].value = NULL;
    }

    for (i = 0; i < h->alloc; ++i) {
	if (h->arr[i].key != NULL) {
	    sam_hash_table_ins(&tmp, h->arr[i].key, h->arr[i].value);
	}
    }

    free(h->arr);
    h->arr = tmp.arr;
    h->alloc = tmp.alloc;
}

bool
sam_hash_table_ins(/*@in@*/ sam_hash_table *restrict h,
		   /*@dependent@*/ const char *restrict key,
		   void *value)
{
    if (h == NULL) {
	return false;
    }

    ++h->nmemb;
    while (((float)h->nmemb) / h->alloc > SAM_MAX_LOAD) {
	sam_hash_table_double(h);
    }

    size_t hash = sam_hash(key, h->alloc);
    for (;;) {
	if (h->arr[hash].key == NULL) {
	    h->arr[hash].key = key;
	    h->arr[hash].value = value;
	    return true;
	} else if (strcmp(key, h->arr[hash].key) == 0) {
	    return false;
	} else if (hash == h->alloc - 1) {
	    hash = 0;
	} else {
	    ++hash;
	}
    }
}

void *
sam_hash_table_get(/*@in@*/ const sam_hash_table *restrict h,
		   /*@dependent@*/ const char *restrict key)
{
    size_t hash, start;

    if (h == NULL) {
	return NULL;
    }

    start = hash = sam_hash(key, h->alloc);
    for (;;) {
	if (h->arr[hash].key == NULL) {
	    return NULL;
	} else if (strcmp(h->arr[hash].key, key) == 0) {
	    return h->arr[hash].value;
	} else if (hash == h->alloc - 1) {
	    hash = 0;
	} else {
	    ++hash;
	}
    }
}

inline void
sam_hash_table_free(sam_hash_table *restrict h)
{
    for (size_t i = 0; i < h->alloc; ++i) {
	free(h->arr[i].value);
    }

    free(h->arr);
}
