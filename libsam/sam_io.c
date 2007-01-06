/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2006 Trevor Caira, Jimmy Hartzell, Daniel Perelman
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
 * Revision 1.3  2007/01/06 01:07:42  trevor
 * Added format attribute to sam_io_vf*f().
 *
 * Revision 1.2  2007/01/05 06:25:21  trevor
 * if --> ?:
 *
 * Revision 1.1  2007/01/04 06:15:03  trevor
 * Input/output routines.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include <libsam/error.h>
#include <libsam/string.h>
#include <libsam/es.h>
#include <libsam/io.h>

static int sam_io_vfprintf(sam_io_stream ios,
			   const char *restrict fmt,
			   va_list ap)
__attribute__((format(printf, 2, 0)));

static int sam_io_vfscanf(sam_io_stream ios,
			  const char *restrict fmt,
			  va_list ap)
__attribute__((format(scanf, 2, 0)));

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
sam_io_vfscanf(sam_io_stream ios,
	       const char *restrict fmt,
	       va_list ap)
{
    sam_string s;
    int	       rv;

    if (fmt == NULL || sam_string_get(sam_ios_to_file(ios), &s) == NULL) {
	return EOF;
    }
    rv = vsscanf(s.data, fmt, ap);
    sam_string_free(&s);
    return rv;
}

static int
sam_sprint_char(char	 *s,
		sam_char  c)
{
    char *start = s;

    if (isprint(c)) {
	sprintf(s, "'%c'", (int)c);
	return 1;
    }
    *s++ = '\'';
    *s++ = '\\';
    switch (c) {
	case '\a':
	    *s++ = 'a';
	    break;
	case '\b':
	    *s++ = 'b';
	    break;
	case '\f':
	    *s++ = 'f';
	    break;
	case '\n':
	    *s++ = 'n';
	    break;
	case '\r':
	    *s++ = 'r';
	    break;
	case '\t':
	    *s++ = 't';
	    break;
	case '\v':
	    *s++ = 'v';
	    break;
	default:
	    *s++ = c;
    }
    *s++ = '\'';
    *s++ = '\0';

    return s - start;
}

static void
sam_io_ml_value_print(const sam_es *restrict es,
		      sam_ml_value v,
		      sam_ml_type t)
{
    switch (t) {
	case SAM_ML_TYPE_INT:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%ld", v.i);
	    break;
	case SAM_ML_TYPE_FLOAT:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%.5g", v.f);
	    break;
	case SAM_ML_TYPE_HA:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%luH", (unsigned long)v.ha);
	    break;
	case SAM_ML_TYPE_SA:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%luS", (unsigned long)v.sa);
	    break;
	case SAM_ML_TYPE_PA:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%lu", (unsigned long)v.pa);
	    break;
	case SAM_ML_TYPE_NONE: /*@fallthrough@*/
	default:
	    sam_io_fprintf(es, SAM_IOS_ERR, "?");
	    break;
    }
}

static void
sam_io_op_value_print(const sam_es *restrict es,
		      sam_op_value v,
		      sam_op_type t)
{
    char buf[8];

    switch (t) {
	case SAM_OP_TYPE_INT:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%ld", v.i);
	    break;
	case SAM_OP_TYPE_FLOAT:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%.5g", v.f);
	    break;
	case SAM_OP_TYPE_CHAR:
	    sam_sprint_char(buf, v.c);
	    sam_io_fprintf(es, SAM_IOS_ERR, "%s", buf);
	    break;
	case SAM_OP_TYPE_STR: /*@fallthrough@*/
	case SAM_OP_TYPE_LABEL:
	    sam_io_fprintf(es, SAM_IOS_ERR, "\"%s\"", v.s);
	    break;
	case SAM_OP_TYPE_NONE: /*@fallthrough@*/
	default:
	    sam_io_fprintf(es, SAM_IOS_ERR, "?");
	    break;
    }
}

/* Die... with style! */
static void
sam_io_bt_default(const sam_es *restrict es)
{
    size_t i;

    sam_io_fprintf(es,
		   SAM_IOS_ERR,
		   "\nstate of execution:\n"
		   "PC:\t%lu\n"
		   "FBR:\t%lu\n"
		   "SP:\t%lu\n\n",
		   (unsigned long)sam_es_pc_get(es),
		   (unsigned long)sam_es_fbr_get(es),
		   (unsigned long)sam_es_stack_len(es));

    sam_io_fprintf(es,
		   SAM_IOS_ERR,
		   "Heap\t    Stack\t    Program\n");
    for (i = 0;
	 i <= sam_es_instructions_len(es) ||
	 i <= sam_es_stack_len(es) ||
	 i < sam_es_heap_len(es);
	 ++i) {
	if (i < sam_es_heap_len(es)) {
	    sam_ml *m = sam_es_heap_get(es, i);
	    if (m == NULL) {
		sam_io_fprintf(es, SAM_IOS_ERR, "NULL");
	    } else {
		sam_io_fprintf(es,
			       SAM_IOS_ERR,
			       "%c: ",
			       sam_ml_type_to_char(m->type));
		sam_io_ml_value_print(es, m->value, m->type);
	    }
	    sam_io_fprintf(es, SAM_IOS_ERR, "\t");
	} else {
	    sam_io_fprintf(es, SAM_IOS_ERR, "\t");
	}
	if (i < sam_es_stack_len(es)) {
	    sam_ml *m = sam_es_stack_get(es, i);
	    if (i == sam_es_fbr_get(es)) {
		sam_io_fprintf(es, SAM_IOS_ERR, "==> ");
	    } else {
		sam_io_fprintf(es, SAM_IOS_ERR, "    ");
	    }
	    sam_io_fprintf(es,
			   SAM_IOS_ERR,
			   "%c: ",
			   sam_ml_type_to_char(m->type));
	    sam_io_ml_value_print(es, m->value, m->type);
	    sam_io_fprintf(es, SAM_IOS_ERR, "\t");
	} else {
	    sam_io_fprintf(es, SAM_IOS_ERR, "    \t\t");
	}
	if (i <= sam_es_instructions_len(es)) {
	    if (i == sam_es_pc_get(es)) {
		sam_io_fprintf(es, SAM_IOS_ERR, "==> ");
	    } else {
		sam_io_fprintf(es, SAM_IOS_ERR, "    ");
	    }
	    if (i < sam_es_instructions_len(es)) {
		sam_instruction *inst = sam_es_instructions_get(es, i);
		fprintf(stderr, "%s ", inst->name);
		if (inst->optype != SAM_OP_TYPE_NONE) {
		    sam_io_op_value_print(es, inst->operand, inst->optype);
		}
	    }
	}
	sam_io_fprintf(es, SAM_IOS_ERR, "\n");
    }
    sam_io_fprintf(es, SAM_IOS_ERR, "\n");
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
    int len = sam_es_io_funcs_vfprintf(es) == NULL?
	sam_io_vfprintf(SAM_IOS_OUT, fmt, ap):
	sam_es_io_funcs_vfprintf(es)(SAM_IOS_OUT, fmt, ap);
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
    int len = sam_es_io_funcs_vfprintf(es) == NULL?
	sam_io_vfprintf(ios, fmt, ap):
	sam_es_io_funcs_vfprintf(es)(ios, fmt, ap);
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
    int len = sam_es_io_funcs_vfscanf(es) == NULL?
	sam_io_vfscanf(SAM_IOS_IN, fmt, ap):
	sam_es_io_funcs_vfscanf(es)(SAM_IOS_IN, fmt, ap);
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
    if (sam_es_io_funcs_afgets(es) == NULL) {
	sam_string str;
	return *s = sam_string_get(sam_ios_to_file(ios), &str);
    }
    return *s = sam_es_io_funcs_afgets(es)(s, ios);
}

void
sam_io_bt(const sam_es *restrict es)
{
    sam_es_io_funcs_bt(es) == NULL?
	sam_io_bt_default(es):
	sam_es_io_funcs_bt(es)(es);
}
