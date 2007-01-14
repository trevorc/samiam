/*
 * samiam.c         a simple frontend to libsam
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

#include <stdio.h>
#include <string.h>
#include <sam.h>

static int
usage(void)
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

    puts("usage: samiam [options]... [samfile]\n"
	 "Interpret and execute a SaM source file.\n\n"
	 "options:\n"
	 "    -q    suppress output\n");

    return -1;
}

int
main(int argc,
     char *restrict const argv[restrict])
{
    int quiet = 0;

    if (argc > 1 && (strcmp (argv[1], "-q") == 0)) {
	quiet = 1;
	++argv;
	--argc;
    }

    return argc > 2?
	usage():
	sam_main(quiet, argc == 1? NULL: argv[1], NULL);
}
