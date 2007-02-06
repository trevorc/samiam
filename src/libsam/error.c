/*
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

#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include <libsam/error.h>
#include <libsam/es.h>
#include <libsam/io.h>
#include <libsam/util.h>

/*@observer@*/ static const char *
sam_op_type_to_string(sam_op_type t)
{
    switch (t) {
	case SAM_OP_TYPE_INT:	return "integer";
	case SAM_OP_TYPE_FLOAT:	return "float";
	case SAM_OP_TYPE_CHAR:	return "character";
	case SAM_OP_TYPE_LABEL:	return "label";
	case SAM_OP_TYPE_STR:	return "string";
	case SAM_OP_TYPE_NONE: /*@fallthrough@*/
	default:		return "nonetype";
    }
}

sam_error
sam_error_optype(/*@in@*/ sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: bad operand type given: %s.\n",
		       sam_op_type_to_string(
			   sam_es_instructions_cur(es)->optype));
	sam_es_bt_set(es, true);
    }
    return SAM_EOPTYPE;
}

sam_error
sam_error_segmentation_fault(sam_es *restrict es,
			     sam_ma ma)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: segmentation fault. attempt to access illegal "
		       "memory at %s address %lu.\n",
		       ma.stack? "stack": "heap",
		       ma.stack?
		       (unsigned long)ma.index.sa:
		       (unsigned long)ma.index.ha);
	sam_es_bt_set(es, true);
    }
    return SAM_ESEGFAULT;
}

sam_error
sam_error_free(sam_es *restrict es,
	       sam_ha  ha)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: attempt to free nonexistant or unused heap "
		       "address %luH\n",
		       (unsigned long)ha);
	sam_es_bt_set(es, true);
    }
    return SAM_EFREE;
}

sam_error
sam_error_stack_underflow(sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es, SAM_IOS_ERR, "error: stack underflow.\n");
	sam_es_bt_set(es, true);
    }
    return SAM_ESTACK_UNDRFLW;
}

sam_error
sam_error_stack_overflow(sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es, SAM_IOS_ERR, "error: stack overflow.\n");
	sam_es_bt_set(es, true);
    }
    return SAM_ESTACK_OVERFLW;
}

sam_error
sam_error_no_memory(sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es, SAM_IOS_ERR, "error: out of memory.\n");
	sam_es_bt_set(es, true);
    }
    return SAM_ENOMEM;
}

sam_error
sam_error_type_conversion(sam_es *es,
			  sam_ml_type to,
			  sam_ml_type found,
			  sam_ml_type expected)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "warning: invalid type conversion to %s.\n"
		       "\texpected: %s.\n"
		       "\tfound: %s.\n",
		       sam_ml_type_to_string(to),
		       sam_ml_type_to_string(found),
		       sam_ml_type_to_string(expected));
	sam_es_bt_set(es, true);
    }
    return SAM_ETYPE_CONVERT;
}

sam_error
sam_error_unknown_identifier(sam_es *es,
			     const char *name)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: unknown identifier found: %s\n",
		       name);
	sam_es_bt_set(es, true);
    }
    return SAM_EUNKNOWN_IDENT;
}

sam_error
sam_error_stack_input(/*@in@*/ sam_es *es,
		      const char  *which,
		      sam_ml_type  found,
		      sam_ml_type  expected)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: invalid type of %s stack argument to %s.\n"
		       "found: %s\n"
		       "expected: %s\n",
		       which,
		       sam_es_instructions_cur(es)->name,
		       sam_ml_type_to_string(found),
		       sam_ml_type_to_string(expected));
	sam_es_bt_set(es, true);
    }

    return SAM_ESTACK_INPUT;
}

sam_error
sam_error_negative_shift(/*@in@*/ sam_es *es,
			 sam_int i)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: attempt to shift %ld by illegal negative value "
		       "%ld.\n",
		       i,
		       sam_es_instructions_cur(es)->operand.i);
    }

    return SAM_ESHIFT;
}

sam_error
sam_error_division_by_zero(sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es, SAM_IOS_ERR, "error: division by zero.\n");
	sam_es_bt_set(es, true);
    }

    return SAM_EDIVISION;
}

sam_error
sam_error_io(sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es, SAM_IOS_ERR, "error: input/output error.\n");
	sam_es_bt_set(es, true);
    }

    errno = 0;
    return SAM_EIO;
}

void
sam_error_uninitialized(/*@in@*/ sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "warning: use of uninitialized memory at program address "
		       "%lu.\n", (unsigned long)sam_es_pc_get(es));
    }
}

void
sam_error_number_format(sam_es *es,
			const char *buf)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "warning: \"%s\" could not be parsed as an integer.\n",
		       buf);
	sam_es_bt_set(es, true);
    }
}

sam_error
sam_error_final_stack_state(sam_es *es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "warning: more than one element left on stack.\n");
	sam_es_bt_set(es, true);
    }
    return SAM_EFINAL_STACK;
}

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
sam_error
sam_error_dlopen(sam_es *es,
		 const char *filename,
		 const char *reason)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: couldn't import library %s (%s).\n",
		       filename, reason);
	sam_es_bt_set(es, true);
    }

    return SAM_EDLOPEN;
}

sam_error
sam_error_dlsym(sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: couldn't call %s (not found).\n",
		       sam_es_instructions_cur(es)->operand.s);
	sam_es_bt_set(es, true);
    }

    return SAM_EDLSYM;
}
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
