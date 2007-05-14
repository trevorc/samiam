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

#ifndef LIBSAM_TYPES_H
#define LIBSAM_TYPES_H

#include <limits.h>
#include <stdlib.h>

#include "config.h"

/**
 *  The various types allowed in sam operand positions.
 */
typedef enum {
    SAM_OP_TYPE_NONE,		/**< A null operand. */
    SAM_OP_TYPE_INT,		/**< An integer type. */
    SAM_OP_TYPE_FLOAT = 1 << 1, /**< An IEEE 754 floating point number. */
    SAM_OP_TYPE_CHAR  = 1 << 2, /**< A single ASCII character. */
    SAM_OP_TYPE_LABEL = 1 << 3, /**< A double-quote delimited string or bare
				 *   label stored as a C string pointing to a
				 *   program address. */
    SAM_OP_TYPE_STR   = 1 << 4  /**< A list of characters to be allocated on the
				 *   heap and represented as a heap address. */
} sam_op_type;

/** An integer, stored by the interpreter as a long to maximize
 * portability */
typedef long sam_int;
typedef unsigned long sam_unsigned_int;

/** Make sure our chars are wide enough to be suitable return values. */
typedef int sam_char;

/** An index into the array of instructions. */
typedef struct {
    unsigned short l;	/* line number */
    unsigned short m;	/* module number */
} sam_pa;

/** An index into the heap. */
typedef size_t sam_ha;

/** An index into the stack. */
typedef size_t sam_sa;

/** A value on the stack, heap or as an operand. */
typedef union {
    sam_int   i;
    sam_float f;
    sam_char  c;
    char     *s;
    sam_pa    pa;
} sam_op_value;

/** A label in the sam source. */
typedef struct {
    /** The pointer into the input to the name of the label. */
    /*@observer@*/ char *restrict name;
    /** The index into the array of instructions pointing to the following
     *  instruction from the source. */
    sam_pa pa;
} sam_label;

#endif /* LIBSAM_TYPES_H */
