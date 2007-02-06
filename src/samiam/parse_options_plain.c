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
#include <string.h>
#include <sam.h>

#include "parse_options.h"

static bool
samiam_usage(void)
{
    puts("usage: samiam [options ...] [samfile]\n"
	 "Interpret and execute a SaM source file.\n\n"
	 "options:\n"
	 "    -q    suppress output\n");

    return false;
}

bool
samiam_parse_options(int argc,
		     char *const argv[restrict],
		     sam_options *restrict options,
		     char **restrict file)
{
    if (argc > 1 && (strcmp (argv[1], "-q") == 0)) {
	*options |= SAM_QUIET;
	++argv;
	--argc;
    }
    *file = argc == 1? NULL: argv[1];
    return argc > 2: samiam_usage(): true;
}
