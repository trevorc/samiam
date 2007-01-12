/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2006 Trevor Caira, Jimmy Hartzell
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
 * Revision 1.1  2007/01/04 05:39:57  trevor
 * New header architecture.
 *
 */

#ifndef LIBSAM_OPCODE_H
#define LIBSAM_OPCODE_H

#include "config.h"
#include "types.h"
#include "error.h"

typedef struct _sam_instruction sam_instruction;
typedef sam_error (*sam_handler)(sam_es *restrict es);

/** An operation in sam. */
struct _sam_instruction {
    /*@observer@*/ const char *name;	/**< The character string
					 *   representing this opcode. */
    sam_op_type optype;			/**< The OR of types allowed
					 *   to be in the operand
					 *   position of this opcode. */
    sam_op_value operand;		/**< The value of this
					 *   instruction's operand,
					 *   assigned on parsing. */
    /*@null@*/ /*@observer@*/
    sam_handler handler;		/**< A pointer to the function
					 *   called when this
					 *   instruction is executed. */
};

/*@null@*/ extern sam_instruction *sam_opcode_get(const char *restrict name);

#endif /* LIBSAM_OPCODE_H */
