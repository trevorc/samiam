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
 * Revision 1.13  2007/01/04 23:51:35  trevor
 * Default type for scanf.
 *
 * Revision 1.12  2007/01/04 06:19:13  trevor
 * Remove debugging statements.
 *
 * Revision 1.11  2007/01/04 05:40:56  trevor
 * Use new io funcs.
 *
 * Revision 1.10  2006/12/27 18:45:23  trevor
 * Rearrange headers.
 *
 * Revision 1.9  2006/12/27 18:43:23  trevor
 * Fixed types.
 *
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

#include <stdio.h>
#include <stdlib.h>

#include <libsam/config.h>
#include <libsam/types.h>
#include <libsam/string.h>

#include "libjbr.h"

int
jbr_vfprintf(sam_io_stream ios,
	     const char *restrict fmt,
	     va_list ap)
{
    if (ios == SAM_IOS_OUT) {
	printf("Processor Output: ");
    }
    int rv = vfprintf(sam_ios_to_file(ios), fmt, ap);
    fprintf(sam_ios_to_file(ios), "\n");
    fflush(sam_ios_to_file(ios));
    return rv;
}

int
jbr_vfscanf(sam_io_stream ios,
	    const char *restrict fmt,
	    va_list ap)
{
    int	       rv;
    const char *type;

    switch (fmt[1]) {
	case 'f':
	    type = "float";
	    break;
	case 'd':
	    type = "integer";
	    break;
	case 'c':
	    type = "character";
	    break;
	case 's':
	    type = "string";
	    break;
	default:
	    type = "nonetype";
	    break;
    }

    printf("Processor Input (enter %s): ", type);
    sam_string s;
    if (sam_string_read(sam_ios_to_file(ios), &s) == NULL) {
	return EOF;
    }
    rv = vsscanf(s.data, fmt, ap);
    sam_string_free(&s);
    return rv;
}

char *
jbr_afgets(char **restrict s,
	   sam_io_stream ios)
{
    sam_string str;
    return *s = sam_string_read(sam_ios_to_file(ios), &str);
}
