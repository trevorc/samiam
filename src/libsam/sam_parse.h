/*
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
 * Revision 1.4  2007/01/04 06:10:35  trevor
 * Accept pointer to a free_func to be called by sam_main() to deallocate input.
 *
 * Revision 1.3  2006/12/25 00:29:54  trevor
 * Update for new hash table labels.
 *
 * Revision 1.2  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef SAM_PARSE_H
#define SAM_PARSE_H

#include <stdbool.h>

typedef void (*sam_input_free_func)(sam_string *restrict s);

extern bool    sam_parse	   (sam_es *restrict es,
				    sam_string *restrict s,
				    sam_input_free_func *restrict free_func,
				    /*@null@*/ const char *restrict file);

#endif /* SAM_PARSE_H */
