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

#include "libsam.h"

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

/*TODO: do we want or need this function*/
#if 0
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
#endif

/*TODO: do we want or need this function?*/
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
    /*sam_warning_leaks(es);*/
    if (err == SAM_OK) {
	sam_warning_forgot_stop(es);
    }
    if (sam_es_bt_get(es)) {
	sam_io_bt(es);
    }

    sam_ml *restrict m = sam_es_stack_get(es, 0);
    return m == NULL?
	sam_warning_empty_stack(es):
	sam_convert_to_int(es, m);
}
