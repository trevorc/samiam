/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2007 Trevor Caira, Jimmy Hartzell, Daniel Perelman
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

#include "libsam.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "error.h"
#include "string.h"
#include "es.h"
#include "io.h"

static int sam_io_vfprintf(sam_io_stream ios,
			   const char *restrict fmt,
			   va_list ap)
__attribute__((format(printf, 2, 0)));

static int sam_io_vfscanf(const sam_es *restrict es,
			  sam_io_stream ios,
			  const char *restrict fmt,
			  va_list ap)
__attribute__((format(scanf, 3, 0)));

static int
sam_io_vfprintf(sam_io_stream ios,
		const char *restrict fmt,
		va_list ap)
{
    int rv = vfprintf(sam_ios_to_file(ios), fmt, ap);
    fflush(sam_ios_to_file(ios));
    return rv;
}

static int
sam_io_vfscanf(const sam_es *restrict es,
	       sam_io_stream ios,
	       const char *restrict fmt,
	       va_list ap)
{
    char *s;
    if (sam_io_afgets(es, &s, ios) == NULL) {
	return 0;
    }

    int rv = vsscanf(s, fmt, ap);
    free(s);
    return rv;
}

FILE *
sam_ios_to_file(sam_io_stream ios)
{
    switch (ios) {
	case SAM_IOS_IN:
	    return stdin;
	case SAM_IOS_OUT:
	    return stdout;
	case SAM_IOS_ERR:
	    return stderr;
	default:
	    return NULL;
    }
}

int
sam_io_printf(const sam_es *restrict es,
	      const char *restrict fmt,
	      ...)
{
    va_list ap;

    va_start(ap, fmt);
    int len = sam_es_io_func_vfprintf(es) == NULL?
	sam_io_vfprintf(SAM_IOS_OUT, fmt, ap):
	sam_es_io_func_vfprintf(es)(SAM_IOS_OUT,
				    sam_es_io_data_get(es),
				    fmt,
				    ap);
    va_end(ap);

    return len;
}

int
sam_io_fprintf(const sam_es *restrict es,
	       sam_io_stream ios,
	       const char *restrict fmt,
	       ...)
{
    va_list ap;

    va_start(ap, fmt);
    int len = sam_es_io_func_vfprintf(es) == NULL?
	sam_io_vfprintf(ios, fmt, ap):
	sam_es_io_func_vfprintf(es)(ios,
				    sam_es_io_data_get(es),
				    fmt,
				    ap);
    va_end(ap);

    return len;
}

int
sam_io_scanf(const sam_es *restrict es,
	     const char *restrict fmt,
	     ...)
{
    va_list ap;

    va_start(ap, fmt);
    int len = sam_es_io_func_vfscanf(es) == NULL?
	sam_io_vfscanf(es, SAM_IOS_IN, fmt, ap):
	sam_es_io_func_vfscanf(es)(SAM_IOS_IN,
				   sam_es_io_data_get(es),
				   fmt,
				   ap);
    va_end(ap);

    return len;
}

char *
sam_io_afgets(const sam_es *restrict es,
	      char **restrict s,
	      sam_io_stream ios)
{
    if (s == NULL) {
	return NULL;
    }
    if (sam_es_io_func_afgets(es) == NULL) {
	sam_string str;
	return *s = sam_string_get(sam_ios_to_file(ios), &str);
    }
    return *s = sam_es_io_func_afgets(es)(s,
					  ios,
					  sam_es_io_data_get(es));
}
