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
#include <unistd.h>

#include "parse_options.h"

static bool
samiam_usage(void)
{
    puts("usage: samiam [-q] [samfile]");
    return false;
}

bool
samiam_parse_options(int argc,
		     char *const argv[restrict],
		     sam_options *restrict options,
		     char **restrict file)
{
    int opt;

    while ((opt = getopt(argc, argv, "q")) > -1) {
	switch (opt) {
	    case 'q':
		*options |= SAM_QUIET;
		break;
	    case '?':
		return samiam_usage();
	}
    }
    if (argc - optind == 1) {
	*file = argv[optind];
    }
    return argc - optind > 1? samiam_usage(): true;
}
