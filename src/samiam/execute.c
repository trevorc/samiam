/*
 * sam_execute.c    run a parsed sam program
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

#include "samiam.h"

#include <libsam/es.h>
#include <libsam/io.h>

#include "execute.h"

/**
 * The stop instruction was not found when there were no more
 * instructions left to execute.
 */
static inline void
sam_warning_forgot_stop(sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       _("warning: final instruction must be STOP.\n"));
	sam_es_bt_set(es, true);
    }
}

static inline sam_exit_code
sam_warning_empty_stack(sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		      _("warning: program terminated with an empty stack.\n"));
	sam_es_bt_set(es, true);
    }

    return SAM_EMPTY_STACK;
}

/**
 * The last item on the stack at program termination was not of type
 * integer.
 *
 *  @param es The current state of execution.
 */
static inline void
sam_warning_retval_type(/*@in@*/ sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_ml *restrict m = sam_es_stack_get(es, 0);
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       _("warning: expected bottom of stack to contain an "
			 "integer (found: %s).\n"),
		       sam_ml_type_to_string(m->type));
	sam_es_bt_set(es, true);
    }
}

static inline void
sam_warning_leaks(/*@in@*/ const sam_es *restrict es)
{
    unsigned long block_count = 0;
    unsigned long leak_size = 0;

    if (!sam_es_options_get(es, SAM_QUIET) &&
	sam_es_heap_leak_check(es, &block_count, &leak_size)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       _("warning: your program leaks %lu byte%s in %lu "
			 "block%s.\n"),
		       leak_size, leak_size == 1? "": "s",
		       block_count, block_count == 1? "": "s");
    }
}

static inline int
sam_convert_to_int(sam_es *restrict es,
		   /*@in@*/ sam_ml *restrict m)
{
    if (m->type == SAM_ML_TYPE_INT) {
	return m->value.i;
    }

    sam_warning_retval_type(es);
    switch (m->type) {
	case SAM_ML_TYPE_FLOAT:
	    return m->value.f;
	case SAM_ML_TYPE_PA:
	    return m->value.pa.l;
	case SAM_ML_TYPE_HA:
	    return m->value.ha.index;
	case SAM_ML_TYPE_SA:
	    return m->value.sa;
	case SAM_ML_TYPE_NONE: /*@fallthrough@*/
	default:
	    return 0;
    }
}

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

/* Die... with style! */
/*
 * TODO:
 *  - module specific
 *  - print labels
 *  - i18n friendly
 */
static void
sam_bt(const sam_es *restrict es)
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
		   _("    Stack\t    Program\n"));
    for (i = 0;
	 i <= sam_es_instructions_len_cur(es) ||
	 i <= sam_es_stack_len(es);
	 ++i) {
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

sam_exit_code
sam_execute(/*@in@*/ sam_es *restrict es)
{
    sam_error err = SAM_OK;

    for (; sam_es_pc_get(es).l < sam_es_instructions_len_cur(es) &&
	    err == SAM_OK;
	 sam_es_pc_pp(es)) {
	err = sam_es_instructions_cur(es)->handler(es);
    }

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_es_dlhandles_close(es);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
    sam_warning_leaks(es);
    if (err == SAM_OK) {
	sam_warning_forgot_stop(es);
    }
    if (sam_es_bt_get(es)) {
	sam_bt(es);
    }

    sam_ml *restrict m = sam_es_stack_get(es, 0);
    return m == NULL?
	sam_warning_empty_stack(es):
	sam_convert_to_int(es, m);
}
