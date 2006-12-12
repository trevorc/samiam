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
 * $Log$
 * Revision 1.2  2006/12/12 23:31:36  trevor
 * Added the $Id$ and $Log$ tags and copyright notice where they were missing.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define NUM 5000000

int main() {
    unsigned long i;
    struct timeval tv;
    double *arr = malloc(sizeof (double) * NUM);
    double acc;

    if (arr == NULL) {
	write(2, "no memory\n", 10);
	abort();
    }

    if (gettimeofday(&tv, NULL) < 0) {
	perror("gettimeofday");
    }
    srand((long)tv.tv_sec);

    for (i = 0; i < NUM; ++i) {
	arr[i] = rand() / (RAND_MAX + 1.0);
    }
    acc = arr[0];
    while (--i >= 1) {
	switch (rand() % 7) {
	    case 0:
	    case 1:
		acc += arr[i];
		break;
	    case 2:
	    case 3:
		acc *= arr[i];
		break;
	    case 4:
	    case 5:
		acc -= arr[i];
		break;
	    case 6:
		acc /= arr[i];
		break;
	}
    }

    printf("%f\n", acc);

    return round(acc);
}
