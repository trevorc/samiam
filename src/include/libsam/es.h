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

#ifndef LIBSAM_EXECUTE_H
#define LIBSAM_EXECUTE_H

#include <stdbool.h>

#include "array.h"
#include "config.h"
#include "error.h"
#include "execute_types.h"
#include "types.h"
#include "main.h"
#include "opcode.h"
#include "string.h"

typedef struct {
    unsigned stack: 1;
    unsigned add: 1;
    unsigned remove: 1;
    sam_ma ma;		    /* point of removal/addition */
    const sam_ml *ml;	    /* item added */
    size_t size;	    /* size of allocation */
} __attribute__((packed))
sam_es_change;

/**
 * The list of labels corresponding to a line of code.
 */
typedef struct {
    sam_pa pa;
    sam_array labels;
} sam_es_loc;

typedef sam_error	   (*sam_library_fn)	     (sam_es *restrict es);

extern inline void	     sam_es_bt_set	     (sam_es *restrict es,
						      bool st);
extern bool		     sam_es_bt_get	     (const sam_es *restrict es);
extern inline sam_pa	     sam_es_pc_get	     (const sam_es *restrict es);
extern inline sam_pa	     sam_es_pc_pp	     (sam_es *restrict es);
extern inline void	     sam_es_pc_set	     (sam_es *restrict es,
						      sam_pa  pc);
extern inline size_t	     sam_es_sp_get	     (sam_es *restrict es);
extern inline sam_sa	     sam_es_fbr_get	     (const sam_es *restrict es);
extern inline void	     sam_es_fbr_set	     (sam_es *restrict es,
						      sam_sa  fbr);
extern sam_error	     sam_es_string_get	     (sam_es *restrict es, char **restrict str,
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
extern inline bool	     sam_es_heap_leak_check  (const sam_es *restrict es,
						      unsigned long *restrict block_count,
						      unsigned long *restrict leak_size);
extern bool		     sam_es_labels_ins	     (sam_es *restrict es,
						      char *restrict label,
						      sam_pa line_no);
extern bool		     sam_es_labels_get	     (sam_es *restrict es,
						      sam_pa *restrict pa,
						      const char *restrict name,
						      unsigned short module);
extern bool		     sam_es_labels_get_cur   (sam_es *restrict es,
						      sam_pa *restrict pa,
						      const char *restrict name);
extern inline sam_array	    *sam_es_locs_get	     (sam_es *restrict es);
extern inline const char    *sam_es_file_get	     (sam_es *restrict es,
						      unsigned short module);
extern inline void	     sam_es_instructions_ins (sam_es *restrict es,
						      sam_instruction *restrict i);
extern inline sam_instruction *sam_es_instructions_get(const sam_es *restrict es,
						      sam_pa pa);
extern inline sam_instruction *sam_es_instructions_get_cur(const sam_es *restrict es,
							   unsigned short line);
extern inline sam_instruction *sam_es_instructions_cur(sam_es *restrict es);
extern inline size_t	     sam_es_instructions_len (const sam_es *restrict es,
						      unsigned short module);
extern inline size_t	     sam_es_instructions_len_cur(const sam_es *restrict es);
extern void		     sam_es_ro_alloc	     (const sam_es *restrict es,
						      const char *symbol,
						      sam_ml_value value,
						      sam_ml_type type);
extern void		     sam_es_global_alloc     (const sam_es *restrict es,
						      const char *symbol,
						      sam_ml_value value,
						      sam_ml_type type);
extern void		     sam_es_export	     (const sam_es *restrict es,
						      const char *symbol);
extern void		     sam_es_import	     (const sam_es *restrict es,
						      const char *symbol);
extern sam_io_vfprintf_func  sam_es_io_func_vfprintf (const sam_es *restrict es);
extern sam_io_vfscanf_func   sam_es_io_func_vfscanf  (const sam_es *restrict es);
extern sam_io_afgets_func    sam_es_io_func_afgets   (const sam_es *restrict es);
extern sam_io_bt_func	     sam_es_io_func_bt	     (const sam_es *restrict es);
extern void		    *sam_es_io_data_get	     (const sam_es *restrict es);
extern bool		     sam_es_options_get	     (const sam_es *restrict es,
						      sam_options option);
extern sam_string	    *sam_es_input_get	     (sam_es *restrict es);

extern sam_es		    *sam_es_new		     (const char *restrict file,
						      sam_options options,
						      /*@null@*/ sam_io_dispatcher dispatcher,
						      /*@null@*/ void *io_data);
extern void		     sam_es_free	     (sam_es *restrict es);
extern void		     sam_es_reset	     (sam_es *restrict es);
extern bool		     sam_es_change_get	     (sam_es *restrict es,
						      sam_es_change *restrict ch);
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
extern sam_error	     sam_es_dlhandles_ins    (sam_es *restrict es,
						      const char *restrict path);
extern sam_library_fn	     sam_es_dlhandles_get    (sam_es *restrict es,
						      const char *restrict sym);
extern void		     sam_es_dlhandles_close  (sam_es *restrict es);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

#endif /* LIBSAM_EXECUTE_H */
