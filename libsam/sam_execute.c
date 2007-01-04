/*
 * sam_execute.c    run a parsed sam program
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
 * Revision 1.30  2007/01/04 06:07:55  trevor
 * Trim down to sam_execute() and a minimal set of helper functions.
 *
 * Revision 1.29  2006/12/26 22:54:07  trevor
 * Logical right shift added.
 *
 * Revision 1.28  2006/12/25 18:56:43  trevor
 * fix #27
 *
 * Revision 1.27  2006/12/25 00:27:16  trevor
 * Use new SDK headers. Add dynamic loading. Export necessary functions for SDK. Update for new label hash table.
 *
 * Revision 1.26  2006/12/23 02:12:58  trevor
 * comparing unlike-type things now prints a sensible error message.
 *
 * Revision 1.25  2006/12/23 01:32:42  anyoneeb
 * All tests pass (except shortStruct and shortArray pass half the time on ia32). samiam's LESS, GREATER, and CMP instruction now require both operands of the same type instead of both operands integers.
 *
 * Revision 1.24  2006/12/21 04:05:59  trevor
 * You can't add pointer types.
 *
 * Revision 1.23  2006/12/21 01:45:55  trevor
 * Allow addition and subtraction of like-type memory addresses.
 *
 * Revision 1.22  2006/12/20 05:30:00  trevor
 * Reject bit shifts by negative values.
 *
 * Revision 1.21  2006/12/19 23:42:28  trevor
 * Pretty-print floating points with %g.
 *
 * Revision 1.20  2006/12/19 18:36:25  trevor
 * Misc fixes (docs, inappropriate use of size_t).
 *
 * Revision 1.19  2006/12/19 18:17:53  trevor
 * Moved parts of sam_util.h which were library-generic into libsam.h.
 *
 * Revision 1.18  2006/12/19 10:41:26  anyoneeb
 * Last splint warnings fix for now.
 *
 * Revision 1.17  2006/12/19 09:47:39  trevor
 * Should be the rest of the fallthroughs.
 *
 * Revision 1.16  2006/12/19 09:36:47  anyoneeb
 * More splint warning fixes.
 *
 * Revision 1.15  2006/12/19 09:29:34  trevor
 * Removed implicit casts.
 *
 * Revision 1.14  2006/12/19 08:33:56  anyoneeb
 * Some splint warning fixes.
 *
 * Revision 1.13  2006/12/19 07:28:09  anyoneeb
 * Split sam_value into sam_op_value and sam_ml_value.
 *
 * Revision 1.12  2006/12/19 05:52:06  trevor
 * Added some *@fallthrough@*s.
 *
 * Revision 1.11  2006/12/19 05:41:15  anyoneeb
 * Sepated operand types from memory types.
 *
 * Revision 1.10  2006/12/17 15:29:15  trevor
 * Fixed #11.
 *
 * Revision 1.9  2006/12/17 13:54:42  trevor
 * Warn about use of uninitialized variables, and initialize them to (int)0.
 *
 * Revision 1.8  2006/12/17 00:39:29  trevor
 * Removed dynamic loading code (Closes: #18). Check return values of sam_push()
 * for overflow, added heap overflow checks (Closes: #20).
 *
 * Revision 1.7  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <libsam/assert.h>
#include <libsam/es.h>
#include <libsam/io.h>

#include "sam_main.h"
#include "sam_execute.h"

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
		       "warning: final instruction must be STOP.\n");
	sam_es_bt_set(es, true);
    }
}

static inline sam_exit_code
sam_warning_empty_stack(sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "warning: program terminated with an empty stack.\n");
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
		       "warning: expected bottom of stack to contain an "
		       "integer (found: %s).\n",
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
		       "warning: your program leaks %lu byte%s in %lu "
		       "block%s.\n",
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
	    return m->value.pa;
	case SAM_ML_TYPE_HA:
	    return m->value.ha;
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

    for (; sam_es_pc_get(es) < sam_es_instructions_len(es) && err == SAM_OK;
	 sam_es_pc_pp(es)) {
	err = sam_es_instructions_cur(es)->handler(es);
    }
    sam_es_dlhandles_close(es);
    sam_warning_leaks(es);
    if (err == SAM_OK) {
	sam_warning_forgot_stop(es);
    }
    if (sam_es_bt_get(es)) {
	sam_io_bt(es);
    }

    sam_ml *m = sam_es_stack_get(es, 0);
    return m == NULL?
	sam_warning_empty_stack(es):
	sam_convert_to_int(es, m);
}
