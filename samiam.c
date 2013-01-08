/*
 * samiam.c         a simple frontend to libsam
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

#include "samiam.h"

#include <stdio.h>

#if defined(HAVE_LIBINTL_H)
# include <locale.h>
#endif /* HAVE_LIBINTL_H */

#include "es.h"

#include "execute.h"

#include "parse_options.h"

#if defined(HAVE_LIBINTL_H)
static const char *
samiam_get_localedir(void)
{
    const char *restrict envvar = getenv("LOCALEDIR");
    return envvar == NULL? LOCALEDIR: envvar;
}
#endif

int
main(int argc,
     char *const argv[])
{
    sam_options options = 0;
    char *file = NULL;

#if defined(HAVE_LIBINTL_H)
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, samiam_get_localedir());
    textdomain(PACKAGE);
#endif /* HAVE_LIBINTL_H */

    if (samiam_parse_options(argc, argv, &options, &file)) {
        sam_es *restrict es = sam_es_new(file, options, NULL, NULL);

        if (es == NULL) {
            return SAM_PARSE_ERROR;
        }

        sam_exit_code retval = sam_execute(es);
        sam_es_free(es);

        return retval;
    } else {
        return SAM_USAGE;
    }
}
