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

#include "util.h"
#include "execute_types.h"

#include <string.h>

sam_ml *
sam_ml_new(sam_ml_value v,
	   sam_ml_type  t)
{
    sam_ml *m = sam_malloc(sizeof (sam_ml));

    /* entire width of m must be initialized */
    memset(m, 0, sizeof (sam_ml));

    m->type = t;
    m->value = v;
    return m;
}

/*@observer@*/ const char *
sam_ml_type_to_string(sam_ml_type t)
{
    switch (t) {
	case SAM_ML_TYPE_INT:	return _("integer");
	case SAM_ML_TYPE_FLOAT:	return _("float");
	case SAM_ML_TYPE_HA:	return _("heap address");
	case SAM_ML_TYPE_SA:	return _("stack address");
	case SAM_ML_TYPE_PA:	return _("program address");
	case SAM_ML_TYPE_NONE: /*@fallthrough@*/
	default:		return _("nonetype");
    }
}

char
sam_ml_type_to_char(sam_ml_type t)
{
    switch (t) {
	case SAM_ML_TYPE_INT:	return 'I';
	case SAM_ML_TYPE_FLOAT:	return 'F';
	case SAM_ML_TYPE_HA:	/*@fallthrough@*/
	case SAM_ML_TYPE_SA:	return 'M';
	case SAM_ML_TYPE_PA:	return 'P';
	case SAM_ML_TYPE_NONE:	/*@fallthrough@*/
	default:		return '?';
    }
}
