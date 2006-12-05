/*
 * libjbr.c         the java bali runtime library
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
#include <stdlib.h>
#include "libjbr.h"

static char buffer[36];

int
readInt(int *n)
{
    printf("Processor Input (enter integer): ");
    *n = atoi(fgets(buffer, 36, stdin));
    return 0;
}

int
readFloat(double *f)
{
    printf("Processor Input (enter float): ");
    *f = atof(fgets(buffer, 36, stdin));
    return 0;
}

int
readChar(char *c)
{
    printf("Processor Input (enter character): ");
    *c = getchar();
    getchar();
    return 0;
}

int
readString(char **s)
{
    *s = NULL;
    return 0;
}

int
printChar(char c)
{
    printf("Processor Output: %c\n",c);
    return 0;
}

int
printFloat(double f)
{
    printf("Processor Output: %f\n",f);
    return 0;
}

int
printInt(int i)
{
    printf("Processor Output: %i\n",i);
    return 0;
}

int
printString(char *s)
{
    printf("Processor Output: %s\n",s);
    return 0;
}
