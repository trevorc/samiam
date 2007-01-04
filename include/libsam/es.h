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
 * Revision 1.2  2006/12/25 18:58:21  trevor
 * fix #27.
 *
 * Revision 1.1  2006/12/25 00:21:39  trevor
 * New SDK suite of headers.
 *
 */

#ifndef LIBSAM_EXECUTE_H
#define LIBSAM_EXECUTE_H

#include <stdbool.h>

#include "config.h"
#include "error.h"
#include "execute_types.h"
#include "types.h"
#include "main.h"
#include "opcode.h"

typedef sam_error	   (*sam_library_fn)	     (sam_es *restrict es);

extern inline void	     sam_es_bt_set  (sam_es *restrict es,
						      bool st);
extern bool		     sam_es_bt_get  (const sam_es *restrict es);
extern inline sam_pa	     sam_es_pc_get	     (const sam_es *restrict es);
extern inline sam_pa	     sam_es_pc_pp	     (sam_es *restrict es);
extern inline void	     sam_es_pc_set	     (sam_es *restrict es,
						      sam_pa  pc);
extern inline sam_sa	     sam_es_fbr_get	     (const sam_es *restrict es);
extern inline void	     sam_es_fbr_set	     (sam_es *restrict es,
						      sam_sa  fbr);
extern sam_error	     sam_es_string_get	     (sam_es *restrict es,
						      char **restrict str,
						      sam_ha ha);
extern sam_ml		    *sam_es_stack_pop	     (sam_es *restrict es);
extern bool		     sam_es_stack_push	     (sam_es *restrict es,
						      sam_ml *restrict m);
extern inline sam_ml	    *sam_es_stack_get	     (const sam_es *restrict es,
						      sam_sa sa);
extern bool		     sam_es_stack_set	     (sam_es *restrict es,
						      sam_ml *restrict ml,
						      sam_sa  sa);
extern size_t		     sam_es_stack_len	     (const sam_es *restrict es);
extern inline sam_ml	    *sam_es_heap_get	     (const sam_es *restrict es,
						      sam_ha  ha);
extern bool		     sam_es_heap_set	     (sam_es *restrict es,
						      sam_ml *restrict ml,
						      sam_ha ha);
extern inline size_t	     sam_es_heap_len	     (const sam_es *restrict es);
extern sam_ha		     sam_es_heap_alloc	     (sam_es *restrict es,
						      size_t size);
extern sam_error	     sam_es_heap_dealloc     (sam_es *restrict es,
						      sam_ha  ha);
inline bool		     sam_es_heap_leak_check  (const sam_es *restrict es,
						      unsigned long *restrict block_count,
						      unsigned long *restrict leak_size);
extern bool		     sam_es_labels_ins	     (sam_es *restrict es,
						      const char *restrict label,
						      sam_pa line_no);
extern bool		     sam_es_labels_get	     (sam_es *restrict es,
						      sam_pa *restrict pa,
						      const char *restrict name);
extern inline void	     sam_es_instructions_ins (sam_es *restrict es,
						      sam_instruction *restrict i);
extern inline sam_instruction *sam_es_instructions_get(const sam_es *restrict es,
						      sam_pa pa);
extern inline sam_instruction *sam_es_instructions_cur(sam_es *restrict es);
extern inline size_t	     sam_es_instructions_len (const sam_es *restrict es);
extern sam_error	     sam_es_dlhandles_ins    (sam_es *restrict es,
						      const char *restrict path);
extern sam_library_fn	     sam_es_dlhandles_get    (sam_es *restrict es,
						      const char *restrict sym);
extern void		     sam_es_dlhandles_close  (sam_es *restrict es);
extern sam_io_vfprintf_func  sam_es_io_funcs_vfprintf(const sam_es *restrict es);
extern sam_io_vfscanf_func   sam_es_io_funcs_vfscanf (const sam_es *restrict es);
extern sam_io_afgets_func    sam_es_io_funcs_afgets  (const sam_es *restrict es);
extern sam_io_bt_func	     sam_es_io_funcs_bt	     (const sam_es *restrict es);
extern bool		     sam_es_options_get	     (const sam_es *restrict es,
						      sam_options option);
extern sam_es		    *sam_es_new		     (sam_options options,
						      /*@null@*/ sam_io_funcs *restrict io_funcs);
extern void		     sam_es_free	     (sam_es *restrict es);

#endif /* LIBSAM_EXECUTE_H */
