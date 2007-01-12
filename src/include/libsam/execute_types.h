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
 * Revision 1.1  2007/01/04 05:39:57  trevor
 * New header architecture.
 *
 */

#ifndef LIBSAM_EXECUTE_TYPES_H
#define LIBSAM_EXECUTE_TYPES_H

#include <stdbool.h>

#include "config.h"
#include "types.h"

/**
 *  The various types allowed in sam memory locations.
 */
typedef enum {
    SAM_ML_TYPE_NONE,	/**< A null operand. */
    SAM_ML_TYPE_INT,	/**< An integer type. */
    SAM_ML_TYPE_FLOAT,	/**< An IEEE 754 floating point number. */
    SAM_ML_TYPE_SA,	/**< A memory address pointing to a location on
			 *   the stack. */
    SAM_ML_TYPE_HA,	/**< A memory address pointing to a location on
			 *   the heap. */
    SAM_ML_TYPE_PA	/**< A program address pointing to an
			 *   instruction in the source file. */
} sam_ml_type;

/** A value on the stack or heap. */
typedef union {
    sam_int   i;
    sam_float f;
    sam_pa    pa;
    sam_ha    ha;
    sam_sa    sa;
} sam_ml_value;

/** An element on the stack or on the heap. */
typedef struct {
    unsigned  type: 8;
    sam_ml_value value;
} sam_ml;

/** A pointer to an element on the heap or the stack. */
typedef struct {
    bool stack; /**< Flag indicating whether the pointer is on the
		     *	 stack or heap. */
    union {
	sam_ha ha;
	sam_sa sa;
    } index;	    /**< The value of the pointer. */
} sam_ma;

typedef struct _sam_es sam_es;

extern const char *sam_ml_type_to_string(sam_ml_type t);
extern char	   sam_ml_type_to_char	(sam_ml_type t);
extern sam_ml	  *sam_ml_new		(sam_ml_value v,
					 sam_ml_type  t);

#endif /* LIBSAM_EXECUTE_TYPES_H */
