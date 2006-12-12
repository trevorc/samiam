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
 * Revision 1.2  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sam_util.h"
#include "sam_main.h"
#include "sam_execute.h"
#include "sam_parse.h"

#if defined(HAVE_LOCALE_H)
# include <locale.h>
#endif /* HAVE_LOCALE_H */

sam_options options;

int
sam_main(sam_options user_options,
	 /*@null@*/ const char *file,
	 /*@null@*/ sam_io_funcs *user_io_funcs)
{
    sam_exit_code retval;
    sam_string	  input;
    sam_array	  instructions;
    sam_array	  labels;
    sam_io_funcs  io_funcs = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    options = user_options;
    if (user_io_funcs != NULL) {
	io_funcs = *user_io_funcs;
    }

#ifdef HAVE_LOCALE_H
    if (setlocale(LC_ALL, "") == NULL ||
	setlocale(LC_CTYPE, "C") == NULL) {
	fputs("warning: couldn't setlocale.\n", stderr);
    }
#endif /* HAVE_LOCALE_H */

    if (!sam_parse(&input, file, &instructions, &labels)) {
	retval = SAM_PARSE_ERROR;
    } else {
	retval = sam_execute(&instructions, &labels, &io_funcs);
	sam_file_free(&input);
    }

    return (int)retval;
}
