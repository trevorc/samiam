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
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sam.h>
#include <getopt.h>

#include "parse_options.h"

__attribute__((noreturn))
static int
samiam_copyright(void)
{
    puts("samiam 0.1\n"
	 "Copyright (c) 2006 Trevor Caira, Jimmy Hartzell\n");

    puts("Permission is hereby granted, free of charge, to any person "
	 "obtaining\na copy of this software and associated documentation "
	 "files (the\n\"Software\"), to deal in the Software without "
	 "restriction, including\nwithout limitation the rights to use, "
	 "copy, modify, merge, publish,\ndistribute, sublicense, and/or "
	 "sell copies of the Software, and to\npermit persons to whom the "
	 "Software is furnished to do so, subject to\nthe following "
	 "conditions:\n");

    puts("The above copyright notice and this permission notice shall be\n"
	 "in all copies or substantial portions of the Software.\n");

    puts("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY "
	 "KIND,\nEXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE "
	 "WARRANTIES OF\nMERCHANTABILITY, FITNESS FOR A PARTICULAR "
	 "PURPOSE AND NONINFRINGEMENT.\nIN NO EVENT SHALL THE AUTHORS "
	 "OR COPYRIGHT HOLDERS BE LIABLE FOR ANY\nCLAIM, DAMAGES OR OTHER "
	 "LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\nTORT OR "
	 "OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE\n" 
	 "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n");

    exit(0);
}

static int
samiam_usage(const char *restrict name)
{
    printf("Usage: %s [OPTION]... [FILE]\n"
	   "Interpret and execute a SaM source file.\n\n"
	   "Mandatory arguments to long options are mandatory for "
	   "short options too.\n"
	   "  -q, --quiet    suppress verbose output\n"
	   "      --help     display this help and exit\n"
	   "      --version  output version information and exit\n\n",
	   name);
    return false;
}

bool
samiam_parse_options(int argc,
		     char *const argv[restrict],
		     sam_options *restrict options,
		     char **restrict file)
{
    int opt;
    static struct option long_options[] = {
	{"quiet", 0, NULL, 'q'},
	{"help", 0, NULL, 'h'},
	{"version", 0, NULL, 'v'},
	{0, 0, NULL, 0},
    };

    while ((opt = getopt_long(argc, argv, "q", long_options, NULL)) > -1) {
	switch (opt) {
	    case 'q':
		*options |= SAM_QUIET;
		break;
	    case 'v':
		samiam_copyright();
	    case 'h':
		samiam_usage(argv[0]);
		exit(0);
	    case '?':
		return samiam_usage(argv[0]);
	}
    }
    if (argc - optind == 1) {
	*file = argv[optind];
    }
    return argc - optind > 1? samiam_usage(argv[0]): true;
}
