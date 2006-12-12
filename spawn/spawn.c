/*
 * spawn.c          launch many instances of samiam in parallel
 *		    (deprecated in favor of tester.pl)
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
 * Revision 1.3  2006/12/12 23:31:36  trevor
 * Added the $Id$ and $Log$ tags and copyright notice where they were missing.
 *
 */

#define _XOPEN_SOURCE 600

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sam.h>

int
main(const int		argc,
     const char *const *argv)
{
    int i;

    for (i = 1; i < argc; ++i) {
	pid_t p = fork();
	if (p == 0) {
	    printf("%s exited with status %d.\n", argv[i],
		   sam_main(1, argv[i], NULL));
	    break;
	} else if (p == -1) {
	    perror("fork");
	    return 1;
	}
    }

    return 0;
}
