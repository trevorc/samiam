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

#include <libsam/error.h>
#include <libsam/string.h>
#include <libsam/es.h>
#include <libsam/io.h>

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

/*TODO: do we really need this function?*/
#if 0
static int
sam_sprint_char(char	 *s,
		sam_char  c)
{
    char *start = s;

    if (isprint(c)) {
	sprintf(s, "'%c'", c);
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
	    s += sprintf(s, "%d", c);
    }
    *s++ = '\'';
    *s++ = '\0';

    return s - start;
}
#endif

/*TODO: do we really need this function*/
#if 0
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
	    sam_io_fprintf(es,
			   SAM_IOS_ERR,
			   "%u:%uH",
			   v.ha.alloc,
			   v.ha.index);
	    break;
	case SAM_ML_TYPE_SA:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%luS", (unsigned long)v.sa);
	    break;
	case SAM_ML_TYPE_PA:
	    sam_io_fprintf(es, SAM_IOS_ERR, "%hu:%hu", v.pa.m, v.pa.l);
	    break;
	case SAM_ML_TYPE_NONE: /*@fallthrough@*/
	default:
	    sam_io_fprintf(es, SAM_IOS_ERR, "?");
	    break;
    }
}
#endif

/* TODO: do we really need this function*/
#if 0
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
#endif

/*TODO: shouldn't this be part of the UI now?
 * GSaM has no need to use this - it does it well enough just by what
 * it is. Therefore, it's not UI-universal. What's it doing in libsam?*/
#if 1
static void
sam_io_bt_default(const sam_es *restrict es) {
    sam_io_fprintf(es,
		   SAM_IOS_ERR,
		   "default backtrace - not implemented");
}
#else
/* Die... with style! */

/*
 * TODO:
 *  - module specific
 *  - print labels
 *  - i18n friendly
 */
static void
sam_io_bt_default(const sam_es *restrict es)
{
    size_t i;

    sam_io_fprintf(es,
		   SAM_IOS_ERR,
		   _("\nstate of execution:\n"
		     "PC:\t%hu:%hu\n"
		     "FBR:\t%lu\n"
		     "SP:\t%lu\n\n"),
		   sam_es_pc_get(es).m,
		   sam_es_pc_get(es).l,
		   (unsigned long)sam_es_fbr_get(es),
		   (unsigned long)sam_es_stack_len(es));

    sam_io_fprintf(es,
		   SAM_IOS_ERR,
		   _("Heap\t    Stack\t    Program\n"));
    for (i = 0;
	 i <= sam_es_instructions_len_cur(es) ||
	 i <= sam_es_stack_len(es) ||
	 i < sam_es_heap_len(es);
	 ++i) {
	if (i < sam_es_heap_len(es)) {
	    sam_ml *m = sam_es_heap_get(es, i);
	    if (m == NULL) {
		sam_io_fprintf(es, SAM_IOS_ERR, _("NULL"));
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
	if (i <= sam_es_instructions_len_cur(es)) {
	    if (i == sam_es_pc_get(es).l) {
		sam_io_fprintf(es, SAM_IOS_ERR, "==> ");
	    } else {
		sam_io_fprintf(es, SAM_IOS_ERR, "    ");
	    }
	    if (i < sam_es_instructions_len_cur(es)) {
		sam_instruction *restrict inst =
		    sam_es_instructions_get_cur(es, i);
		sam_io_fprintf(es, SAM_IOS_ERR, "%s", inst->name);
		if (inst->optype != SAM_OP_TYPE_NONE) {
		    sam_io_fprintf(es, SAM_IOS_ERR, " ");
		    sam_io_op_value_print(es, inst->operand, inst->optype);
		}
#if 0
		char *restrict label = sam_es_labels_get(es, );
		sam_io_fprintf(es, SAM_IOS_ERR, " [%s]", );
#endif
	    }
	}
	sam_io_fprintf(es, SAM_IOS_ERR, "\n");
    }
    sam_io_fprintf(es, SAM_IOS_ERR, "\n");
}
#endif

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

void
sam_io_bt(const sam_es *restrict es)
{
    sam_es_io_func_bt(es) == NULL?
	sam_io_bt_default(es):
	sam_es_io_func_bt(es)(es, sam_es_io_data_get(es));
}
