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

#ifndef LIBSAM_ERROR_H
#define LIBSAM_ERROR_H

#include "config.h"
#include "types.h"
#include "execute_types.h"

/** Error codes representing execution errors. */
typedef enum {
    SAM_OK,		/**< No error occurred. */
    SAM_STOP,		/**< The program should terminate gracefully. */
    SAM_EOPTYPE,	/**< An unexpected operand type was
			 *   encountered. */
    SAM_ESEGFAULT,	/**< An attempt was made to access illegal
			 *   (free, uninitialized, or unallocated)
			 *   memory. */
    SAM_EFREE,		/**< An attempt was made to free an address off
			 *   the heap. */
    SAM_ESTACK_UNDRFLW,	/**< An attempt was made to pop a value off the
			 *   stack when the stack contained no items */
    SAM_ESTACK_OVERFLW,	/**< An attempt was made to push a value onto
			 *   the stack when the stack was already at its
			 *   capacity. */
    SAM_ENOMEM,		/**< An allocation failed because the heap was at
			 *   its capacity. */
    SAM_ETYPE_CONVERT,	/**< An unexpected source type was found when 
			 * using a type conversion operator. */
    SAM_EFINAL_STACK,	/**< Program execution terminated with more than
			 *   one element left on the stack. */
    SAM_EUNKNOWN_IDENT,	/**< A reference was made to an unknown
			 *   identifier. */
    SAM_ESTACK_INPUT,	/**< The stack input to an operation is of the
			 *   wrong type. */
    SAM_EIO,		/**< There was an input/output error. */
    SAM_EDIVISION,	/**< Attempt to divide by zero. */
    SAM_ESHIFT,		/**< Attempt to shift by a negative value. */
    SAM_ENOSYS,		/**< This opcode is not supported on this
			 *   system because support for it was not
			 *   compiled in. */
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    SAM_EDLOPEN,	/**< There was a problem interfacing with the
			 *   dynamic linking loader. */
    SAM_EDLSYM,		/**< The given symbol could not be resolved. */

#endif
} sam_error;

extern sam_error sam_error_optype	     (sam_es *restrict es);
extern sam_error sam_error_segmentation_fault(sam_es *restrict es,
					      bool stack,
					      sam_ma  ma);
extern sam_error sam_error_free		     (sam_es *restrict es,
					      sam_ha  ha);
extern sam_error sam_error_stack_underflow   (sam_es *restrict es);
extern sam_error sam_error_stack_overflow    (sam_es *restrict es);
extern sam_error sam_error_no_memory	     (sam_es *restrict es);
extern sam_error sam_error_type_conversion   (sam_es *restrict es,
					      sam_ml_type to,
					      sam_ml_type found,
					      sam_ml_type expected);
extern sam_error sam_error_unknown_identifier(sam_es *restrict es,
					      const char *restrict name);
extern sam_error sam_error_stack_input	     (sam_es *restrict es,
					      const char *restrict which,
					      sam_ml_type found,
					      sam_ml_type expected);
extern sam_error sam_error_negative_shift    (sam_es *restrict es,
					      sam_int i);
extern sam_error sam_error_division_by_zero  (sam_es *restrict es);
extern sam_error sam_error_io		     (sam_es *restrict es);
extern void sam_error_uninitialized	     (sam_es *restrict es);
extern void sam_error_number_format	     (sam_es *restrict es,
					      const char *buf);
extern sam_error sam_error_final_stack_state (sam_es *restrict es);
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
extern sam_error sam_error_dlopen	     (sam_es *restrict es,
					      const char *filename,
					      const char *reason);
extern sam_error sam_error_dlsym	     (sam_es *restrict es);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

#endif /* LIBSAM_ERROR_H */
