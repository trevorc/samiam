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

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libsam/es.h>
#include <libsam/error.h>
#include <libsam/opcode.h>
#include <libsam/util.h>

typedef enum {
    SAM_OP_TIMES,
    SAM_OP_DIV,
    SAM_OP_MOD,
    SAM_OP_AND,
    SAM_OP_OR,
    SAM_OP_NAND,
    SAM_OP_NOR,
    SAM_OP_XOR,
    SAM_OP_BITAND,
    SAM_OP_BITOR,
    SAM_OP_BITNAND,
    SAM_OP_BITNOR,
    SAM_OP_BITXOR,
    SAM_OP_CMP,
    SAM_OP_LESS,
    SAM_OP_GREATER
} sam_integer_arithmetic_operation;

typedef enum {
    SAM_OP_ADDF,
    SAM_OP_SUBF,
    SAM_OP_TIMESF,
    SAM_OP_DIVF,
    SAM_OP_CMPF
} sam_float_arithmetic_operation;

typedef enum {
    SAM_OP_NOT,
    SAM_OP_BITNOT,
    SAM_OP_ISPOS,
    SAM_OP_ISNEG,
    SAM_OP_ISNIL
} sam_unary_arithmetic_operation;

typedef enum {
    SAM_SHIFT_ARITH_LEFT,
    SAM_SHIFT_ARITH_RIGHT,
    SAM_SHIFT_LOGIC_RIGHT
} sam_bitshift_type;

static sam_error
sam_get_jump_target(/*@in@*/ sam_es *restrict es,
		    /*@out@*/ sam_pa *restrict p)
{
    sam_instruction *cur = sam_es_instructions_cur(es);

    if (cur->optype == SAM_OP_TYPE_INT) {
	/* subtract 1 when we change the pa because it will be
	 * incremented by the loop in sam_execute */
	*p = cur->operand.pa - 1;
    } else if (cur->optype == SAM_OP_TYPE_LABEL) {
	if (sam_es_labels_get(es, p, cur->operand.s)) {
	    /* as above */
	    --*p;
	} else {
	    *p = 0;
	    return sam_error_unknown_identifier(es, cur->operand.s);
	}
    } else {
	*p = 0;
	return sam_error_optype(es);
    }

    return SAM_OK;
}

static sam_error
sam_sp_shift(sam_es *restrict es,
	     sam_ha sp)
{
    while (sp < sam_es_stack_len(es)) {
	sam_ml *m;
	if ((m = sam_es_stack_pop(es)) == NULL) {
	    return sam_error_stack_underflow(es);
	}
	free(m);
    }
    while (sp > sam_es_stack_len(es)) {
	if (!sam_es_stack_push(es, sam_ml_new((sam_ml_value){.i = 0},
					      SAM_ML_TYPE_NONE))) {
	    return sam_error_stack_overflow(es);
	}
    }

    return SAM_OK;
}

static sam_error
sam_pushabs(/*@in@*/ sam_es *restrict es,
	    bool stack,
	    sam_ma ma)
{
    sam_ml *restrict m = stack?
	sam_es_stack_get(es, ma.sa):
	sam_es_heap_get(es, ma.ha);

    return m == NULL?
	sam_error_segmentation_fault(es, stack, ma):
	    (sam_es_stack_push(es, sam_ml_new(m->value, m->type))?
	     SAM_OK: sam_error_stack_overflow(es));
}

static sam_error
sam_storeabs(/*@in@*/ sam_es *restrict es,
	     /*@only@*/ sam_ml *m,
	     bool stack,
	     sam_ma ma)
{
    return (stack?
	    sam_es_stack_set(es, m, ma.sa):
	    sam_es_heap_set(es, m, ma.ha))?
		SAM_OK: sam_error_segmentation_fault(es, stack, ma);
}

static sam_error
sam_addition(/*@in@*/ sam_es *restrict es,
	     bool	      add)
{
    int sign = add? 1: -1;

    sam_ml *restrict m2 = sam_es_stack_pop(es);
    if (m2 == NULL) {
	return sam_error_stack_underflow(es);
    }
    sam_ml *restrict m1 = sam_es_stack_pop(es);
    if (m1 == NULL) {
	free(m2);
	return sam_error_stack_underflow(es);
    }
    if (m1->type == SAM_ML_TYPE_NONE) {
	sam_error_uninitialized(es);
	m1->type = SAM_ML_TYPE_INT;
	m1->value.i = 0;
    }
    if (m2->type == SAM_ML_TYPE_NONE) {
	sam_error_uninitialized(es);
	m2->type = SAM_ML_TYPE_INT;
	m2->value.i = 0;
    }

    sam_ml_type t;
    switch (m1->type) {
	case SAM_ML_TYPE_PA:
	    if (m2->type == SAM_ML_TYPE_INT) {
		/* user could set an illegal index here */
		m1->value.pa = m1->value.pa + sign * m2->value.i;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    break;
	case SAM_ML_TYPE_HA:
	    /* user could set an illegal index here or could overflow
	     * the address */
	    if (m2->type == SAM_ML_TYPE_INT) {
		m1->value.ha = m1->value.ha + sign * m2->value.i;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    } else if (m2->type == SAM_ML_TYPE_HA && sign == -1) {
		m1->value.i = m1->value.ha - m2->value.ha;
		m1->type = SAM_ML_TYPE_INT;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    break;
	case SAM_ML_TYPE_SA:
	    if (m2->type == SAM_ML_TYPE_INT) {
		/* user could set an illegal index here or could
		 * overflow the address */
		m1->value.sa = m1->value.sa + sign * m2->value.i;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    } else if (m2->type == SAM_ML_TYPE_SA) {
		m1->value.i = m1->value.sa - m2->value.sa;
		m1->type = SAM_ML_TYPE_INT;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    break;
	case SAM_ML_TYPE_INT:
	    if (m2->type == SAM_ML_TYPE_INT) {
		m1->value.i += sign * m2->value.i;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    if (m2->type == SAM_ML_TYPE_PA) {
		/* user could set an illegal index here */
		m1->value.pa = m1->value.i + sign * m2->value.pa;
		m1->type = SAM_ML_TYPE_PA;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    if (m2->type == SAM_ML_TYPE_HA) {
		/* user could set an illegal index here or overflow */
		m1->value.ha = m1->value.i + sign * m2->value.ha;
		m1->type = SAM_ML_TYPE_HA;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    if (m2->type == SAM_ML_TYPE_SA) {
		/* user could set an illegal index here or overflow */
		m1->value.sa = m1->value.i + sign * m2->value.sa;
		m1->type = SAM_ML_TYPE_SA;
		free(m2);
		return sam_es_stack_push(es, m1)?
		    SAM_OK: sam_error_stack_overflow(es);
	    }
	    break;
	default:
	    t = m1->type;
	    free(m1);
	    free(m2);
	    return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }
    t = m2->type;
    free(m1);
    free(m2);
    return sam_error_stack_input(es, "second", t, SAM_ML_TYPE_INT);
}

static sam_error
sam_integer_arithmetic(sam_es *restrict es, 
		       sam_integer_arithmetic_operation op)
{
    sam_ml *restrict m2 = sam_es_stack_pop(es);
    if (m2 == NULL) {
	return sam_error_stack_underflow(es);
    }
    sam_ml *restrict m1 = sam_es_stack_pop(es);
    if (m1 == NULL) {
	free(m2);
	return sam_error_stack_underflow(es);
    }
    if (op == SAM_OP_CMP || op == SAM_OP_LESS || op == SAM_OP_GREATER) {
	if(m1->type != m2->type) {
	    sam_ml_type expected = m1->type;
	    sam_ml_type found = m2->type;
	    free(m1);
	    free(m2);
	    return sam_error_stack_input(es, "second", found, expected);
	}
    } else {
	if (m1->type != SAM_ML_TYPE_INT) {
	    sam_ml_type t = m1->type;
	    free(m1);
	    free(m2);
	    return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
	}
	if (m2->type != SAM_ML_TYPE_INT) {
	    sam_ml_type t = m2->type;
	    free(m1);
	    free(m2);
	    return sam_error_stack_input(es, "second", t, SAM_ML_TYPE_INT);
	}
    }

    switch(op) {
	case SAM_OP_TIMES:
	    m1->value.i = m1->value.i * m2->value.i;
	    break;
	case SAM_OP_AND:
	    m1->value.i = m1->value.i && m2->value.i;
	    break;
	case SAM_OP_OR:
	    m1->value.i = m1->value.i && m2->value.i;
	    break;
	case SAM_OP_XOR:
	    m1->value.i = (!m1->value.i) ^ (!m2->value.i);
	    break;
	case SAM_OP_NOR:
	    m1->value.i = !(m1->value.i || m2->value.i);
	    break;
	case SAM_OP_NAND:
	    m1->value.i = !(m1->value.i && m2->value.i);
	    break;
	case SAM_OP_DIV:
	    if (m2->value.i == 0) {
		free(m1);
		free(m2);
		return sam_error_division_by_zero(es);
	    }
	    m1->value.i = m1->value.i / m2->value.i;
	    break;
	case SAM_OP_MOD:
	    if(m2->value.i == 0) {
		free(m1);
		free(m2);
		return sam_error_division_by_zero(es);
	    }
	    m1->value.i = m1->value.i % m2->value.i;
	    break;
	case SAM_OP_BITAND:
	    m1->value.i = m1->value.i & m2->value.i;
	    break;
	case SAM_OP_BITOR:
	    m1->value.i = m1->value.i | m2->value.i;
	    break;
	case SAM_OP_BITNAND:
	    m1->value.i = ~(m1->value.i & m2->value.i);
	    break;
	case SAM_OP_BITNOR:
	    m1->value.i = ~(m1->value.i | m2->value.i);
	    break;
	case SAM_OP_BITXOR:
	    m1->value.i = m1->value.i ^ m2->value.i;
	    break;
	case SAM_OP_CMP:
	    m1->value.i = m1->value.i < m2->value.i?
		-1: m2->value.i == m1->value.i? 0: 1;
	    m1->type = SAM_ML_TYPE_INT;
	    break;
	case SAM_OP_GREATER:
	    m1->value.i = m1->value.i > m2->value.i;
	    m1->type = SAM_ML_TYPE_INT;
	    break;
	case SAM_OP_LESS:
	    m1->value.i = m1->value.i < m2->value.i;
	    m1->type = SAM_ML_TYPE_INT;
	    break;
    }
    free(m2);
    if (!sam_es_stack_push(es, m1)) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static bool
sam_float_equal(sam_float a,
		sam_float b)
{
    return fabs(a - b) <= ((fabs(b) > fabs(a)?
	  fabs(b): fabs(a)) * SAM_EPSILON);
}

static sam_error
sam_float_arithmetic(sam_es *restrict es,
		     sam_float_arithmetic_operation op)
{
    sam_ml *restrict m2 = sam_es_stack_pop(es);
    if (m2 == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m2->type != SAM_ML_TYPE_FLOAT) {
	sam_ml_type t = m2->type;
	free(m2);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_FLOAT);
    }
    sam_ml *restrict m1 = sam_es_stack_pop(es);
    if (m1 == NULL) {
	free(m2);
	return sam_error_stack_underflow(es);
    }
    if (m1->type != SAM_ML_TYPE_FLOAT) {
	sam_ml_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input(es, "second", t, SAM_ML_TYPE_INT);
    }

    switch(op) {
	case SAM_OP_ADDF:
	    m1->value.f = m1->value.f + m2->value.f;
	    break;
	case SAM_OP_SUBF:
	    m1->value.f = m1->value.f - m2->value.f;
	    break;
	case SAM_OP_TIMESF:
	    m1->value.f = m1->value.f * m2->value.f;
	    break;
	case SAM_OP_DIVF:
	    m1->value.f = m1->value.f / m2->value.f;
	    break;
	case SAM_OP_CMPF:
	    m1->value.f =
		sam_float_equal(m1->value.f, m2->value.f)?
		0: m1->value.f < m2->value.f? -1: 1;
	    break;
    }
    free(m2);
    m1->type = SAM_ML_TYPE_FLOAT;

    return sam_es_stack_push(es, m1)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_unary_arithmetic(sam_es			    *restrict es,
		     sam_unary_arithmetic_operation  op)
{
    sam_ml *m1;

    if ((m1 = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m1->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m1->type;
	free(m1);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }

    switch(op) {
	case SAM_OP_NOT: /*@fallthrough@*/
	case SAM_OP_ISNIL:
	    m1->value.i = !m1->value.i;
	    break;
	case SAM_OP_BITNOT:
	    m1->value.i = ~m1->value.i;
	    break;
	case SAM_OP_ISPOS:
	    m1->value.i = m1->value.i > 0;
	    break;
	case SAM_OP_ISNEG:
	    m1->value.i = m1->value.i < 0;
	    break;
    }

    return sam_es_stack_push(es, m1)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_int
sam_do_shift(sam_int	       i,
	     sam_int	       j,
	     sam_bitshift_type type)
{
    switch (type) {
	case SAM_SHIFT_ARITH_LEFT:
	    return i << j;
	case SAM_SHIFT_ARITH_RIGHT:
	    return i >> j;
	case SAM_SHIFT_LOGIC_RIGHT:
	    return ((sam_unsigned_int)i) >> j;
	default: /*@notreached@*/
	    return 0;
    }
}

static sam_error
sam_bitshift(/*@in@*/ sam_es    *restrict es,
	     sam_bitshift_type	type)
{
    sam_ml	    *m;
    sam_instruction *cur = sam_es_instructions_cur(es);

    if (cur->optype != SAM_OP_TYPE_INT) {
	return sam_error_optype(es);
    }
    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }
    if (cur->operand.i < 0) {
	sam_int i = m->value.i;
	free(m);
	return sam_error_negative_shift(es, i);
    }
    m->value.i = sam_do_shift(m->value.i, cur->operand.i, type);

    return sam_es_stack_push(es, m)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_bitshiftind(/*@in@*/ sam_es *restrict es,
		sam_bitshift_type type)
{
    sam_ml *restrict m2 = sam_es_stack_pop(es);
    if (m2 == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m2->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m2->type;
	free(m2);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }

    sam_ml *restrict m1 = sam_es_stack_pop(es);
    if (m1 == NULL) {
	free(m2);
	return sam_error_stack_underflow(es);
    }
    if (m1->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input(es, "second", t, SAM_ML_TYPE_INT);
    }
    m1->value.i = sam_do_shift(m1->value.i, m2->value.i, type);
    free(m2);
    return sam_es_stack_push(es, m1)? SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_es_read(/*@in@*/ sam_es *restrict es,
	    const char *restrict fmt,
	    sam_ml_type t)
{
    sam_ml_value v = {.i = 0};

    return (t == SAM_ML_TYPE_FLOAT?
	sam_io_scanf(es, fmt, &v.f):
	sam_io_scanf(es, fmt, &v.i)) == 1?
	    (sam_es_stack_push(es, sam_ml_new(v, t))?
		SAM_OK: sam_error_stack_overflow(es)):
	    sam_error_io(es);

}

/* Opcode implementations. */
static sam_error
sam_op_ftoi(/*@in@*/ sam_es *restrict es)
{
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_FLOAT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_type_conversion(es, SAM_ML_TYPE_INT, t,
					 SAM_ML_TYPE_FLOAT);
    }
    m->value.i = floor(m->value.f);
    m->type = SAM_ML_TYPE_INT;

    return sam_es_stack_push(es, m)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_ftoir(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_FLOAT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_type_conversion(es, SAM_ML_TYPE_INT, t,
					 SAM_ML_TYPE_FLOAT);
    }
    m->value.i = round(m->value.f);
    m->type = SAM_ML_TYPE_INT;

    return sam_es_stack_push(es, m)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_itof(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_type_conversion(es, SAM_ML_TYPE_FLOAT, t,
					 SAM_ML_TYPE_INT);
    }
    m->value.f = (sam_float)m->value.i;
    m->type = SAM_ML_TYPE_FLOAT;
    if (!sam_es_stack_push(es, m)) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_pushimm(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);

    if (cur->optype != SAM_OP_TYPE_INT) {
	return sam_error_optype(es);
    }
    sam_ml_value v = {.i = cur->operand.i};
    sam_ml *restrict m = sam_ml_new(v, SAM_ML_TYPE_INT);
    if (!sam_es_stack_push(es, m)) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_pushimmf(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);
    sam_ml_value v;

    if (cur->optype != SAM_OP_TYPE_FLOAT) {
	return sam_error_optype(es);
    }
    v.f = cur->operand.f;

    sam_ml *restrict m = sam_ml_new(v, SAM_ML_TYPE_FLOAT);
    return sam_es_stack_push(es, m)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_pushimmch(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);
    sam_ml_value v;

    if (cur->optype != SAM_OP_TYPE_CHAR) {
	return sam_error_optype(es);
    }
    v.i = cur->operand.c;
    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_INT))) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_pushimmma(/*@in@*/ sam_es *restrict es)
{
    sam_instruction	*cur = sam_es_instructions_cur(es);
    sam_ml_value	 v;

    if (cur->optype != SAM_OP_TYPE_INT) {
	return sam_error_optype(es);
    }
    /* weird things happen when user pushes a negative operand */
    v.sa = (size_t)cur->operand.i;
    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_SA))) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_pushimmpa(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *cur = sam_es_instructions_cur(es);
    if (cur->optype == SAM_OP_TYPE_INT) {
	if (!sam_es_stack_push(es, sam_ml_new(
		    (sam_ml_value){ .pa = cur->operand.i }, SAM_ML_TYPE_PA))) {
	    return sam_error_stack_overflow(es);
	}
    } else if (cur->optype == SAM_OP_TYPE_LABEL) {
	sam_ml_value v;
	if (sam_es_labels_get(es, &v.pa, cur->operand.s)) {
	    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_PA))) {
		return sam_error_stack_overflow(es);
	    }
	} else {
	    return sam_error_unknown_identifier(es, cur->operand.s);
	}
    } else {
	return sam_error_optype(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_pushimmstr(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *cur = sam_es_instructions_cur(es);
    sam_ml *m;
    sam_ml_value v;
    size_t len, i;

    if (cur->optype != SAM_OP_TYPE_STR) {
	return sam_error_optype(es);
    }
    len = strlen(cur->operand.s);
    if ((v.ha = sam_es_heap_alloc(es, len + 1)) == SAM_HEAP_PTR_MAX) {
	return sam_error_no_memory(es);
    }
    for (i = 0; i < len; ++i) {
	m = sam_es_heap_get(es, i + v.ha);
	m->value.i = cur->operand.s[i];
	m->type = SAM_ML_TYPE_INT;
    }
    m = sam_es_heap_get(es, i + v.ha);
    m->value.i = '\0';
    m->type = SAM_ML_TYPE_INT;

    return sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_HA))?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_pushsp(/*@in@*/ sam_es *restrict es)
{
    sam_ml_value v;
    v.sa = sam_es_stack_len(es);
    return sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_SA))?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_pushfbr(/*@in@*/ sam_es *restrict es)
{
    sam_ml_value v;
    v.sa = sam_es_fbr_get(es);
    return sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_SA))?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_popsp(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m;
    size_t  i;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_SA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_SA);
    }
    i = m->value.sa;
    free(m);

    return sam_sp_shift(es, i);
}

static sam_error
sam_op_popfbr(/*@in@*/ sam_es *restrict es)
{
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_SA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_SA);
    }
    sam_es_fbr_set(es, m->value.sa);
    free(m);
    return SAM_OK;
}

static sam_error
sam_op_dup(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m1, *m2;

    if ((m1 = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    m2 = sam_ml_new(m1->value, m1->type);

    if (!sam_es_stack_push(es, m1)) {
	free(m2);
	return sam_error_stack_overflow(es);
    }
    return sam_es_stack_push(es, m2)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_swap(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m1, *m2;

    if ((m1 = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if ((m2 = sam_es_stack_pop(es)) == NULL) {
	free(m1);
	return sam_error_stack_underflow(es);
    }
    if (!sam_es_stack_push(es, m1)) {
	free(m2);
	return sam_error_stack_overflow(es);
    }
    return sam_es_stack_push(es, m2)?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_addsp(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *cur = sam_es_instructions_cur(es);

    if (cur->optype != SAM_OP_TYPE_INT) {
	return sam_error_optype(es);
    }
    if (cur->operand.i + (int)sam_es_stack_len(es) < 0) {
	return sam_error_stack_underflow(es);
    }
    return sam_sp_shift(es, (size_t)cur->operand.i + sam_es_stack_len(es));
}

static sam_error
sam_op_malloc(/*@in@*/ sam_es *restrict es)
{
    size_t i;
    sam_ml_value v;
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (es == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }
    if (m->value.i == 0) {
	m->value.i = 1;
    }
    if ((v.ha = sam_es_heap_alloc(es, m->value.i)) == SAM_HEAP_PTR_MAX) {
	free(m);
	return sam_error_no_memory(es);
    }
    for (i = v.ha; i < (size_t)m->value.i + v.ha; ++i) {
	sam_ml *m_i = sam_es_heap_get(es, i);
	m_i->type = SAM_ML_TYPE_NONE;
	m_i->value.i = 0;
    }
    free(m);
    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_HA))) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_free(/*@in@*/ sam_es *restrict es)
{
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_HA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_HA);
    }
    sam_ha ha = m->value.ha;
    free(m);
    return ha >= sam_es_heap_len(es)? sam_error_free(es, ha): sam_es_heap_dealloc(es, ha);
}

static sam_error
sam_op_pushind(/*@in@*/ sam_es *restrict es)
{
    sam_ml  *m;
    sam_ml_type t;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type == SAM_ML_TYPE_HA) {
	sam_ma ma = {.ha = m->value.ha};
	free(m);
	return sam_pushabs(es, false, ma);
    }
    if (m->type == SAM_ML_TYPE_SA) {
	sam_ma ma = {.sa = m->value.sa};
	free(m);
	return sam_pushabs(es, true, ma);
    }
    t = m->type;
    free(m);
    return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_SA);   /* XXX */
}

static sam_error
sam_op_storeind(/*@in@*/ sam_es *restrict es)
{
    sam_ml_type t;
    /*@null@*/ sam_ml *m2 = sam_es_stack_pop(es);

    if (m2 == NULL) {
	return sam_error_stack_underflow(es);
    }

    /*@null@*/ sam_ml *m1 = sam_es_stack_pop(es);
    if (m1 == NULL) {
	free(m2);
	return sam_error_stack_underflow(es);
    }
    if (m1->type == SAM_ML_TYPE_HA) {
	sam_ma ma = {.ha = m1->value.ha};
	free(m1);
	return sam_storeabs(es, m2, false, ma);
    }
    if (m1->type == SAM_ML_TYPE_SA) {
	sam_ma ma = {.sa = m1->value.sa};
	free(m1);
	return sam_storeabs(es, m2, true, ma);
    }
    t = m1->type;
    free(m1);
    free(m2);
    return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_SA); /* XXX */
}

/* cannot be used to push from the heap. */
static sam_error
sam_op_pushabs(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);

    if (cur->optype != SAM_OP_TYPE_INT) {
	return sam_error_optype(es);
    }
    
    sam_ma ma = {.sa = cur->operand.i};

    return sam_pushabs(es, true, ma);
}

/* cannot be used to store onto the heap. */
static sam_error
sam_op_storeabs(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (cur->optype != SAM_OP_TYPE_INT) {
	free(m);
	return sam_error_optype(es);
    }

    sam_ma ma = {.sa = cur->operand.i};
    return sam_storeabs(es, m, true, ma);
}

static sam_error
sam_op_pushoff(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);

    if (cur->optype != SAM_OP_TYPE_INT) {
	return sam_error_optype(es);
    }

    sam_ma ma = {.sa = sam_es_fbr_get(es) + cur->operand.i};
    return sam_pushabs(es, true, ma);
}

static sam_error
sam_op_storeoff(/*@in@*/ sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);
    sam_ml	    *m;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (cur->optype != SAM_OP_TYPE_INT) {
	free(m);
	return sam_error_optype(es);
    }

    sam_ma ma = {.sa = sam_es_fbr_get(es) + cur->operand.i};
    return sam_storeabs(es, m, true, ma);
}

static sam_error
sam_op_add(/*@in@*/ sam_es *restrict es)
{
    return sam_addition(es, true);
}

static sam_error
sam_op_sub(/*@in@*/ sam_es *restrict es)
{
    return sam_addition(es, false);
}

static sam_error
sam_op_times(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_TIMES);
}

static sam_error
sam_op_div(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_DIV);
}

static sam_error
sam_op_mod(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_MOD);
}

static sam_error
sam_op_addf(/*@in@*/ sam_es *restrict es)
{
    return sam_float_arithmetic(es, SAM_OP_ADDF);
}

static sam_error
sam_op_subf(/*@in@*/ sam_es *restrict es)
{
    return sam_float_arithmetic(es, SAM_OP_SUBF);
}

static sam_error
sam_op_timesf(/*@in@*/ sam_es *restrict es)
{
    return sam_float_arithmetic(es, SAM_OP_TIMESF);
}

static sam_error
sam_op_divf(/*@in@*/ sam_es *restrict es)
{
    return sam_float_arithmetic(es, SAM_OP_DIVF);
}

static sam_error
sam_op_lshift(/*@in@*/ sam_es *restrict es)
{
    return sam_bitshift(es, SAM_SHIFT_ARITH_LEFT);
}

static sam_error
sam_op_lshiftind(/*@in@*/ sam_es *restrict es)
{
    return sam_bitshiftind(es, SAM_SHIFT_ARITH_LEFT);
}

static sam_error
sam_op_rshift(/*@in@*/ sam_es *restrict es)
{
    return sam_bitshift(es, SAM_SHIFT_ARITH_RIGHT);
}

static sam_error
sam_op_rshiftind(/*@in@*/ sam_es *restrict es)
{
    return sam_bitshiftind(es, SAM_SHIFT_ARITH_RIGHT);
}

static sam_error
sam_op_lrshift(/*@in@*/ sam_es *restrict es)
{
    return sam_bitshift(es, SAM_SHIFT_LOGIC_RIGHT);
}

static sam_error
sam_op_lrshiftind(/*@in@*/ sam_es *restrict es)
{
    return sam_bitshiftind(es, SAM_SHIFT_LOGIC_RIGHT);
}

static sam_error
sam_op_and(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_AND);
}

static sam_error
sam_op_or(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_OR);
}

static sam_error
sam_op_nand(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_NAND);
}

static sam_error
sam_op_nor(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_NOR);
}

static sam_error
sam_op_xor(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_XOR);
}

static sam_error
sam_op_not(/*@in@*/ sam_es *restrict es)
{   
    return sam_unary_arithmetic(es, SAM_OP_NOT);
}

static sam_error
sam_op_bitand(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_BITAND);
}

static sam_error
sam_op_bitor(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_BITOR);
}

static sam_error
sam_op_bitnand(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_BITNAND);
}

static sam_error
sam_op_bitnor(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_BITNOR);
}

static sam_error
sam_op_bitxor(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_BITXOR);
}

static sam_error
sam_op_bitnot(/*@in@*/ sam_es *restrict es)
{
    return sam_unary_arithmetic(es, SAM_OP_BITNOT);
}

static sam_error
sam_op_cmp(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_CMP);
}

static sam_error
sam_op_cmpf(/*@in@*/ sam_es *restrict es)
{
    return sam_float_arithmetic(es, SAM_OP_CMPF);
}

static sam_error
sam_op_greater(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_GREATER);
}

static sam_error
sam_op_less(/*@in@*/ sam_es *restrict es)
{
    return sam_integer_arithmetic(es, SAM_OP_LESS);
}

static sam_error
sam_op_equal(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m1, *m2;

    if ((m2 = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if ((m1 = sam_es_stack_pop(es)) == NULL) {
	free(m2);
	return sam_error_stack_underflow(es);
    }
    switch (m1->type) {
	case SAM_ML_TYPE_FLOAT:
	    if (m2->type == SAM_ML_TYPE_FLOAT) {
		m1->value.i = sam_float_equal(m1->value.f, m2->value.f);
	    } else {
		m1->value.i = false;
	    }
	    break;
	case SAM_ML_TYPE_INT:
	    if (m2->type == SAM_ML_TYPE_INT) {
		m1->value.i = m1->value.i == m2->value.i;
	    } else {
		m1->value.i = false;
	    }
	    break;
	case SAM_ML_TYPE_PA:
	    if (m2->type == SAM_ML_TYPE_PA) {
		m1->value.i = m1->value.pa == m2->value.pa;
	    } else {
		m1->value.i = false;
	    }
	    break;
	case SAM_ML_TYPE_HA:
	    if (m2->type == SAM_ML_TYPE_HA) {
		m1->value.i = m1->value.ha == m2->value.ha;
	    } else {
		m1->value.i = false;
	    }
	    break;
	case SAM_ML_TYPE_SA:
	    if (m2->type == SAM_ML_TYPE_SA) {
		m1->value.i = m1->value.sa == m2->value.sa;
	    } else {
		m1->value.i = false;
	    }
	    break;
	case SAM_ML_TYPE_NONE: /*@fallthrough@*/
	default:
	    m1->value.i = false;
	    break;
    }

    m1->type = SAM_ML_TYPE_INT;
    free(m2);
    if (!sam_es_stack_push(es, m1)) {
	return sam_error_stack_overflow(es);
    }

    return SAM_OK;
}

static sam_error
sam_op_isnil(/*@in@*/ sam_es *restrict es)
{
    return sam_unary_arithmetic(es, SAM_OP_ISNIL);
}

static sam_error
sam_op_ispos(/*@in@*/ sam_es *restrict es)
{
    return sam_unary_arithmetic(es, SAM_OP_ISPOS);
}

static sam_error
sam_op_isneg(/*@in@*/ sam_es *restrict es)
{
    return sam_unary_arithmetic(es, SAM_OP_ISNEG);
}

static sam_error
sam_op_jump(/*@in@*/ sam_es *restrict es)
{
    sam_pa    target;
    sam_error err;

    if ((err = sam_get_jump_target(es, &target)) != SAM_OK) {
	return err;
    }
    sam_es_pc_set(es, target);

    return SAM_OK;
}

static sam_error
sam_op_jumpc(/*@in@*/ sam_es *restrict es)
{
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }
    if (m->value.i == 0) {
	free(m);
	return SAM_OK;
    }
    sam_pa target;
    sam_error err = sam_get_jump_target(es, &target);
    if (err != SAM_OK) {
	free(m);
	return err;
    }

    sam_es_pc_set(es, target);
    free(m);
    return SAM_OK;
}

static sam_error
sam_op_jumpind(/*@in@*/ sam_es *restrict es)
{
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_PA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_PA);
    }
    sam_es_pc_set(es, m->value.pa - 1);
    free(m);
    return SAM_OK;
}

static sam_error
sam_op_rst(/*@in@*/ sam_es *restrict es)
{
    return sam_op_jumpind(es);
}

static sam_error
sam_op_jsr(/*@in@*/ sam_es *restrict es)
{
    sam_ml_value v;
    sam_pa	 target;
    sam_error	 err;

    v.pa = sam_es_pc_get(es) + 1;
    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_PA))) {
	return sam_error_stack_overflow(es);
    }
    if ((err = sam_get_jump_target(es, &target)) != SAM_OK) {
	return err;
    }
    sam_es_pc_set(es, target);

    return SAM_OK;
}

static sam_error
sam_op_jsrind(/*@in@*/ sam_es *restrict es)
{
    sam_ml	 *m;
    sam_ml_value  v;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_PA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_PA);
    }

    v.pa = sam_es_pc_get(es) + 1;
    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_PA))) {
	free(m);
	return sam_error_stack_overflow(es);
    }
    sam_es_pc_set(es, m->value.pa - 1);

    free(m);
    return SAM_OK;
}

static sam_error
sam_op_skip(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    sam_es_pc_set(es, sam_es_pc_get(es) + m->value.pa);

    free(m);
    return SAM_ENOSYS;
}

static sam_error
sam_op_link(/*@in@*/ sam_es *restrict es)
{
    sam_ml_value v = {.sa = sam_es_fbr_get(es)};
    if (!sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_SA))) {
	sam_error_stack_overflow(es);
    }
    sam_es_fbr_set(es, sam_es_stack_len(es) - 1);
    return SAM_OK;
}

static sam_error
sam_op_unlink(/*@in@*/ sam_es *restrict es)
{
    return sam_op_popfbr(es);
}

static sam_error
sam_op_read(/*@in@*/ sam_es *restrict es)
{
    return sam_es_read(es, "%ld", SAM_ML_TYPE_INT);
}

static sam_error
sam_op_readf(/*@in@*/ sam_es *restrict es)
{
    return sam_es_read(es, "%f", SAM_ML_TYPE_FLOAT);
}

static sam_error
sam_op_readch(/*@in@*/ sam_es *restrict es)
{
    return sam_es_read(es, "%c", SAM_ML_TYPE_INT);
}

static sam_error
sam_op_readstr(/*@in@*/ sam_es *restrict es)
{
    char   *str;
    size_t  len, i;
    sam_ml_value v;

    if (sam_io_afgets(es, &str, SAM_IOS_IN) == NULL) {
	return sam_error_io(es);
    }
    len = strlen(str);

    if ((v.ha = sam_es_heap_alloc(es, len + 1)) == SAM_HEAP_PTR_MAX) {
	free(str);
	return sam_error_no_memory(es);
    }
    for (i = 0; i <= len; ++i) {
	sam_ml *restrict m = sam_es_heap_get(es, i + v.ha);
	m->value.i = str[i];
	m->type = SAM_ML_TYPE_INT;
    }
    free(str);
    return sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_HA))?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_write(/*@in@*/ sam_es *restrict es)
{
    sam_ml *m;
    sam_int i;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }
    i = m->value.i;
    free(m);

    return sam_io_printf(es, "%ld", i) > 0? SAM_OK: sam_error_io(es);
}

static sam_error
sam_op_writef(/*@in@*/ sam_es *restrict es)
{
    sam_ml    *m;
    sam_float  f;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_FLOAT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_FLOAT);
    }
    f = m->value.f;
    free(m);

    return sam_io_printf(es, "%g", f) > 0? SAM_OK: sam_error_io(es);
}

static sam_error
sam_op_writech(/*@in@*/ sam_es *restrict es)
{
    sam_ml   *m;
    sam_char  c;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
    }
    c = m->value.i;
    free(m);

    return sam_io_printf(es, "%c", c) == 1? SAM_OK: sam_error_io(es);
}

static sam_error
sam_op_writestr(/*@in@*/ sam_es *restrict es)
{
    sam_ml    *m;
    sam_ha     ha, i;
    sam_error  rv;
    char      *str;

    if ((m = sam_es_stack_pop(es)) == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_HA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_HA);
    }
    ha = m->value.ha;
    free(m);

    if (ha >= sam_es_heap_len(es)) {
	sam_ma ma = {.ha = ha};
	return sam_error_segmentation_fault(es, false, ma);
    }
    i = ha;

    if ((rv = sam_es_string_get(es, &str, ha)) != SAM_OK) {
	return rv;
    }
    rv = sam_io_printf(es, "%s", str) > 0? SAM_OK: sam_error_io(es);
    free(str);
    return rv;
}

static sam_error
sam_op_stop(/*@in@*/ sam_es *restrict es)
{
    if (sam_es_stack_len(es) > 1) {
	return sam_error_final_stack_state(es);
    }

    return SAM_STOP;
}

#if defined(SAM_EXTENSIONS)
# if defined(HAVE_DLFCN_H)
static sam_error
sam_op_load(sam_es *restrict es)
{
    sam_instruction *cur = sam_es_instructions_cur(es);

    if (cur->optype != SAM_OP_TYPE_LABEL) {
	return sam_error_optype(es);
    }
    return sam_es_dlhandles_ins(es, cur->operand.s);
}

static sam_error
sam_op_call(sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);
    if (sam_es_instructions_cur(es)->optype != SAM_OP_TYPE_LABEL) {
	return sam_error_optype(es);
    }

    sam_library_fn fn = sam_es_dlhandles_get(es, cur->operand.s);
    return fn == NULL? sam_error_dlsym(es): fn(es);
}

# else /* HAVE_DLFCN_H */

static sam_error
sam_op_load(__attribute__((unused)) sam_es *restrict es)
{
    fputs("error: samiam was not compiled with support for the load "
	  "instruction.\n", stderr);
    return SAM_ENOSYS;
}

static sam_error
sam_op_call(__attribute__((unused)) sam_es *restrict es)
{
    fputs("error: samiam was not compiled with support for the call "
	  "instruction.\n", stderr);
    return SAM_ENOSYS;
}

# endif /* HAVE_DLFCN_H */
#endif /* SAM_EXTENSIONS */

#if 0
static sam_error
sam_op_import(sam_es *restrict es)
{
    sam_instruction *restrict cur = sam_es_instructions_cur(es);
    if (sam_es_instructions_cur(es)->optype != SAM_OP_TYPE_LABEL) {
	return sam_error_optype(es);
    }
    return sam_es_stack_push(es, sam_ml_new((sam_ml_value) {
				    .i = sam_main(sam_es_options(es),
						  cur->operand.s,
						  sam_es_io_funcs(es))
				    }, SAM_ML_TYPE_INT))?
	SAM_OK: sam_error_stack_overflow(es);
}

static sam_error
sam_op_export(__attribute__((unused)) sam_es *restrict es)
{
    return SAM_OK;
}
#endif

static sam_error
sam_op_patoi(/*@in@*/ sam_es *restrict es)
{
    sam_ml *restrict m = sam_es_stack_pop(es);

    if (m == NULL) {
	return sam_error_stack_underflow(es);
    }
    if (m->type != SAM_ML_TYPE_PA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_type_conversion(es, SAM_ML_TYPE_INT, t, SAM_ML_TYPE_PA);
    }
    m->value.i = m->value.pa;
    m->type = SAM_ML_TYPE_INT;
    return sam_es_stack_push(es, m)? SAM_OK: sam_error_stack_overflow(es);
}

static const struct {
    const char *name;
    sam_op_type optype;
    sam_handler handler;
} sam_opcodes[] = {
    { "FTOI",		SAM_OP_TYPE_NONE,  sam_op_ftoi		},
    { "FTOIR",		SAM_OP_TYPE_NONE,  sam_op_ftoir		},
    { "ITOF",		SAM_OP_TYPE_NONE,  sam_op_itof		},
    { "PUSHIMM",	SAM_OP_TYPE_INT,   sam_op_pushimm	},
    { "PUSHIMMF",	SAM_OP_TYPE_FLOAT, sam_op_pushimmf	},
    { "PUSHIMMCH",	SAM_OP_TYPE_CHAR,  sam_op_pushimmch	},
    { "PUSHIMMMA",	SAM_OP_TYPE_INT,   sam_op_pushimmma	},
    { "PUSHIMMPA",	SAM_OP_TYPE_LABEL |
			SAM_OP_TYPE_INT,   sam_op_pushimmpa	},
    { "PUSHIMMSTR",	SAM_OP_TYPE_STR,   sam_op_pushimmstr	},
    { "PUSHSP",		SAM_OP_TYPE_NONE,  sam_op_pushsp	},
    { "PUSHFBR",	SAM_OP_TYPE_NONE,  sam_op_pushfbr	},
    { "POPSP",		SAM_OP_TYPE_NONE,  sam_op_popsp		},
    { "POPFBR",		SAM_OP_TYPE_NONE,  sam_op_popfbr	},
    { "DUP",		SAM_OP_TYPE_NONE,  sam_op_dup		},
    { "SWAP",		SAM_OP_TYPE_NONE,  sam_op_swap		},
    { "ADDSP",		SAM_OP_TYPE_INT,   sam_op_addsp		},
    { "MALLOC",		SAM_OP_TYPE_NONE,  sam_op_malloc	},
    { "FREE",		SAM_OP_TYPE_NONE,  sam_op_free		},
    { "PUSHIND",	SAM_OP_TYPE_NONE,  sam_op_pushind	},
    { "STOREIND",	SAM_OP_TYPE_NONE,  sam_op_storeind	},
    { "PUSHABS",	SAM_OP_TYPE_INT,   sam_op_pushabs	},
    { "STOREABS",	SAM_OP_TYPE_INT,   sam_op_storeabs	},
    { "PUSHOFF",	SAM_OP_TYPE_INT,   sam_op_pushoff	},
    { "STOREOFF",	SAM_OP_TYPE_INT,   sam_op_storeoff	},
    { "ADD",		SAM_OP_TYPE_NONE,  sam_op_add		},
    { "SUB",		SAM_OP_TYPE_NONE,  sam_op_sub		},
    { "TIMES",		SAM_OP_TYPE_NONE,  sam_op_times		},
    { "DIV",		SAM_OP_TYPE_NONE,  sam_op_div		},
    { "MOD",		SAM_OP_TYPE_NONE,  sam_op_mod		},
    { "ADDF",		SAM_OP_TYPE_NONE,  sam_op_addf		},
    { "SUBF",		SAM_OP_TYPE_NONE,  sam_op_subf		},
    { "TIMESF",		SAM_OP_TYPE_NONE,  sam_op_timesf	},
    { "DIVF",		SAM_OP_TYPE_NONE,  sam_op_divf		},
    { "LSHIFT",		SAM_OP_TYPE_INT,   sam_op_lshift	},
    { "LSHIFTIND",	SAM_OP_TYPE_NONE,  sam_op_lshiftind	},
    { "RSHIFT",		SAM_OP_TYPE_INT,   sam_op_rshift	},
    { "RSHIFTIND",	SAM_OP_TYPE_NONE,  sam_op_rshiftind	},
#if defined(SAM_EXTENSIONS)
    { "LRSHIFT",	SAM_OP_TYPE_INT,   sam_op_lrshift	},
    { "LRSHIFTIND",	SAM_OP_TYPE_NONE,  sam_op_lrshiftind	},
#endif
    { "AND",		SAM_OP_TYPE_NONE,  sam_op_and		},
    { "OR",		SAM_OP_TYPE_NONE,  sam_op_or		},
    { "NAND",		SAM_OP_TYPE_NONE,  sam_op_nand		},
    { "NOR",		SAM_OP_TYPE_NONE,  sam_op_nor		},
    { "XOR",		SAM_OP_TYPE_NONE,  sam_op_xor		},
    { "NOT",		SAM_OP_TYPE_NONE,  sam_op_not		},
    { "BITAND",		SAM_OP_TYPE_NONE,  sam_op_bitand	},
    { "BITOR",		SAM_OP_TYPE_NONE,  sam_op_bitor		},
    { "BITNAND",	SAM_OP_TYPE_NONE,  sam_op_bitnand	},
    { "BITNOR",		SAM_OP_TYPE_NONE,  sam_op_bitnor	},
    { "BITXOR",		SAM_OP_TYPE_NONE,  sam_op_bitxor	},
    { "BITNOT",		SAM_OP_TYPE_NONE,  sam_op_bitnot	},
    { "CMP",		SAM_OP_TYPE_NONE,  sam_op_cmp		},
    { "CMPF",		SAM_OP_TYPE_NONE,  sam_op_cmpf		},
    { "GREATER",	SAM_OP_TYPE_NONE,  sam_op_greater	},
    { "LESS",		SAM_OP_TYPE_NONE,  sam_op_less		},
    { "EQUAL",		SAM_OP_TYPE_NONE,  sam_op_equal		},
    { "ISNIL",		SAM_OP_TYPE_NONE,  sam_op_isnil		},
    { "ISPOS",		SAM_OP_TYPE_NONE,  sam_op_ispos		},
    { "ISNEG",		SAM_OP_TYPE_NONE,  sam_op_isneg		},
    { "JUMP",		SAM_OP_TYPE_LABEL |
			SAM_OP_TYPE_INT,   sam_op_jump		},
    { "JUMPC",		SAM_OP_TYPE_LABEL |
			SAM_OP_TYPE_INT,   sam_op_jumpc		},
    { "JUMPIND",	SAM_OP_TYPE_NONE,  sam_op_jumpind	},
    { "RST",		SAM_OP_TYPE_NONE,  sam_op_rst		},
    { "JSR",		SAM_OP_TYPE_LABEL |
			SAM_OP_TYPE_INT,   sam_op_jsr		},
    { "JSRIND",		SAM_OP_TYPE_NONE,  sam_op_jsrind	},
    { "SKIP",		SAM_OP_TYPE_NONE,  sam_op_skip		},
    { "LINK",		SAM_OP_TYPE_NONE,  sam_op_link		},
    { "UNLINK",		SAM_OP_TYPE_NONE,  sam_op_unlink	},
    { "READ",		SAM_OP_TYPE_NONE,  sam_op_read		},
    { "READF",		SAM_OP_TYPE_NONE,  sam_op_readf		},
    { "READCH",		SAM_OP_TYPE_NONE,  sam_op_readch	},
    { "READSTR",	SAM_OP_TYPE_NONE,  sam_op_readstr	},
    { "WRITE",		SAM_OP_TYPE_NONE,  sam_op_write		},
    { "WRITEF",		SAM_OP_TYPE_NONE,  sam_op_writef	},
    { "WRITECH",	SAM_OP_TYPE_NONE,  sam_op_writech	},
    { "WRITESTR",	SAM_OP_TYPE_NONE,  sam_op_writestr	},
    { "STOP",		SAM_OP_TYPE_NONE,  sam_op_stop		},
#if defined(SAM_EXTENSIONS)
    { "patoi",		SAM_OP_TYPE_NONE,  sam_op_patoi		},
    { "load",		SAM_OP_TYPE_LABEL, sam_op_load		},
    { "call",		SAM_OP_TYPE_LABEL, sam_op_call		},
#endif
#if 0
    { "import",		SAM_OP_TYPE_LABEL, sam_op_import	},
    { "export",		SAM_OP_TYPE_LABEL, sam_op_export	},
#endif
    { "",		SAM_OP_TYPE_NONE,  NULL			},
};

sam_instruction *
sam_opcode_get(/*@in@*/ /*@dependent@*/ const char *name)
{
    for (size_t j = 0; sam_opcodes[j].handler != NULL; ++j) {
	if (strcmp(name, sam_opcodes[j].name) == 0) {
	    sam_instruction *restrict i = sam_malloc(sizeof (sam_instruction));
	    i->name = name;
	    i->optype = sam_opcodes[j].optype;
	    i->handler = sam_opcodes[j].handler;
	    i->operand.i = 0;
	    return i;
	}
    }
    return NULL;
}
