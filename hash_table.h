/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2007 Trevor Caira, Jimmy Hartzell, Daniel Perelman
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

#ifndef LIBSAM_HASH_TABLE_H
#define LIBSAM_HASH_TABLE_H

#include "config.h"
#include "types.h"

#include <stdbool.h>

/** Hash table of strings to void *. */
typedef struct {
    size_t   nmemb;
    size_t   alloc;
    struct {
	const char *restrict key;
	void *value;
    } *restrict arr;
} sam_hash_table;

extern void sam_hash_table_init(sam_hash_table *restrict h);
extern bool sam_hash_table_ins(/*@in@*/ sam_hash_table *restrict h,
			       /*@dependent@*/ const char *restrict key,
			       void *value);
extern void *sam_hash_table_get(/*@in@*/ const sam_hash_table *restrict h,
				/*@dependent@*/ const char *restrict key);
extern void sam_hash_table_free(sam_hash_table *restrict h);

#endif /* LIBSAM_HASH_TABLE_H */
