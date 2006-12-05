/*
 * jbc_tester.c     the jbc tester app frontend
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
 */

#include <stdio.h>
#include <string.h>
#include <sam.h>

#include "libjbr.h"

static void
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

    puts("usage: jbc_tester samfile\n"
	 "Interpret and execute a SaM source file.\n\n");
}

int
main(int	 argc,
     char *const argv[])
{
    sam_io_funcs io_funcs = {
	readInt, readFloat, readChar, readString,
	printInt, printFloat, printChar, printString,
    };
    if (argc != 2) {
	usage();
	return 1;
    }
    return sam_main(1, argv[1], &io_funcs);
}
