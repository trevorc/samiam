/*
 * sam_main.c       the main entrance point to libsam
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
 * Revision 1.7  2007/01/08 03:15:25  trevor
 * Added const to io_funcs param.
 *
 * Revision 1.6  2007/01/04 06:09:05  trevor
 * New sam_es architecture.
 *
 * Revision 1.5  2006/12/25 00:28:44  trevor
 * Update for new hash table labels.
 *
 * Revision 1.4  2006/12/19 18:17:53  trevor
 * Moved parts of sam_util.h which were library-generic into libsam.h.
 *
 * Revision 1.3  2006/12/19 09:29:34  trevor
 * Removed implicit casts.
 *
 * Revision 1.2  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libsam/es.h>
#include <libsam/main.h>
#include <libsam/string.h>
#include <libsam/array.h>
#include <libsam/hash_table.h>

#include "sam_execute.h"
#include "sam_parse.h"

#if defined(HAVE_LOCALE_H)
# include <locale.h>
#endif /* HAVE_LOCALE_H */

int
sam_main(sam_options options,
	 /*@null@*/ const char *restrict file,
	 /*@null@*/ const sam_io_funcs *restrict io_funcs)
{
    sam_es *restrict es = sam_es_new(options, io_funcs);

#ifdef HAVE_LOCALE_H
    if (setlocale(LC_ALL, "") == NULL ||
	setlocale(LC_CTYPE, "C") == NULL) {
	fputs("warning: couldn't setlocale.\n", stderr);
    }
#endif /* HAVE_LOCALE_H */

    sam_string input;
    sam_input_free_func free_func;
    if (!sam_parse(es, &input, &free_func, file)) {
	sam_es_free(es);
	return SAM_PARSE_ERROR;
    }

    sam_exit_code retval = sam_execute(es);
    sam_es_free(es);
    free_func(&input);

    return retval;
}
