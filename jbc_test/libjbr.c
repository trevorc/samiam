/*
 * libjbr.c         the java bali runtime library
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
 * Revision 1.8  2006/12/25 00:23:50  trevor
 * Use the new SDK headers.
 *
 * Revision 1.7  2006/12/17 14:00:33  trevor
 * cleaned up log message.
 *
 * Revision 1.6  2006/12/17 01:54:01  trevor
 * UNUSED -> *@unused@*
 *
 * Revision 1.5  2006/12/12 23:31:35  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#include <libsam/config.h>

#include <stdio.h>
#include <stdlib.h>

#include "libjbr.h"

static char buffer[36];

int
readInt(int *n)
{
    long l;
    char *endptr;

    printf("Processor Input (enter integer): ");
    if (fgets(buffer, 36, stdin) == NULL) {
	return -1;
    }
    l = strtol(buffer, &endptr, 0);
    *n = (int)l;

    return 0;
}

int
readFloat(double *f)
{
    char *endptr;

    printf("Processor Input (enter float): ");
    if (fgets(buffer, 36, stdin) == NULL) {
	return -1;
    }
    *f = strtod(buffer, &endptr);
    return 0;
}

int
readChar(char *c)
{
    int n;

    printf("Processor Input (enter character): ");
    if ((n = getchar()) == EOF) {
	return -1;
    }
    *c = (char)n;
    if ((n = getchar()) == EOF ||
	(n != (int)'\n' && ungetc(n, stdin) == EOF)) {
	return -1;
    }

    return 0;
}

int
readString(/*@unused@*/ __attribute__((unused)) char **s)
{
    printf("Processor Input (enter string): ");
    return 0;
}

int
printChar(char c)
{
    printf("Processor Output: %c\n", c);
    return 0;
}

int
printFloat(double f)
{
    printf("Processor Output: %f\n", f);
    return 0;
}

int
printInt(int i)
{
    printf("Processor Output: %i\n", i);
    return 0;
}

int
printString(char *s)
{
    printf("Processor Output: %s\n", s);
    return 0;
}
