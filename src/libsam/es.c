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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libsam/array.h>
#include <libsam/hash_table.h>
#include <libsam/es.h>
#include <libsam/string.h>
#include <libsam/util.h>

#include "parse.h"

#if defined(HAVE_MMAN_H)
# include <sys/mman.h>
#endif

#if defined(HAVE_DLFCN_H)
# include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
typedef struct {
    /*@dependent@*/ const char *restrict name;	/**< Name of symbol loaded. */
    /*@observer@*/  void *restrict handle;	/**< Handle returned from dlopen(). */
} sam_dlhandle;
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

typedef struct _sam_heap_pointer {
    size_t start;
    size_t size;
    /*@null@*/ /*@only@*/ struct _sam_heap_pointer *next;
} sam_heap_pointer;

typedef struct {
    /**	A linked list of pointers into the array of free locations on
     * the heap.  */
    /*@null@*/ sam_heap_pointer *restrict free_list;

    /**	A linked list of pointers into the array of used locations on
     * the heap.  */
    /*@null@*/ sam_heap_pointer *restrict used_list;

    /** The array of all locations on the heap. */
    sam_array a;
} sam_heap;

typedef struct _sam_es_change_list sam_es_change_list;

struct _sam_es_change_list {
    sam_es_change change;
    sam_es_change_list *next;
    sam_es_change_list *prev;
};

/** The parsed instructions and labels along with the current state
 *  of execution. */
struct _sam_es {
    bool bt;		    /**< Is a stack trace is needed? */
    sam_string input;	    /**< The sam input file data. */
    sam_pa pc;		    /**< The index into sam_es# program
			     *   pointing to the current instruction,
			     *   aka the program counter. Incremented by
			     *   #sam_execute, not by the individual
			     *   instruction handlers. */
    sam_sa fbr;		    /**< The frame base register. */

    /* stack pointer is stack->len */
    sam_array  stack;	    /**< The sam stack, an array of {@link
			     *  sam_ml}s.  The stack pointer register is
			     *  simply the length of this array. */
    sam_heap   heap;	    /**< The sam heap, managed by the functions
			     *   #sam_es_heap_alloc and #sam_heap_free. */
    sam_array instructions; /**< A shallow copy of the instructions
			     *   array allocated in main and initialized
			     *   in sam_parse(). */
    sam_array locs;	    /**< An array of sam_es_locs ordered
			         by pa. */
    sam_hash_table labels;  /**< A shallow copy of the labels hash
			     *   table allocated in main and initialized
			     *   in sam_parse(). */
    sam_es_change_list *first_change;
    sam_es_change_list *last_change;
    sam_io_dispatcher io_dispatcher;
    sam_options options;
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array dlhandles;    /**< Handles returned from dlopen(). */
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
};

/* The sam allocator. */
/*@only@*/ static inline sam_heap_pointer *
sam_es_heap_pointer_new(size_t start,
			size_t size,
			/*@only@*/ /*@null@*/ sam_heap_pointer *next)
{
    sam_heap_pointer *restrict p = sam_malloc(sizeof (sam_heap_pointer));
    p->start = start;
    p->size = size;
    p->next = next;
    return p;
}

/** Update the linked list of pointers to used locations in the heap to
 * reflect a newly allocated or freed block. */
static sam_heap_pointer *
sam_es_heap_pointer_update(/*@null@*/ sam_heap_pointer *p,
			   size_t start,
			   size_t size)
{
    sam_heap_pointer *head, *last;

    if (p == NULL) {
	return sam_es_heap_pointer_new(start, size, NULL);
    }
    if (p->start > start) {
	return sam_es_heap_pointer_new(start, size, p);
    }
    head = last = p;
    p = p->next;

    for (;;) {
	if (p == NULL) {
	    last->next = sam_es_heap_pointer_new(start, size, NULL);
	    return head;
	}
	if (p->start > start) {
	    last->next = sam_es_heap_pointer_new(start, size, p);
	    return head;
	}
	last = p;
	p = p->next;
    }
}

static void
sam_es_heap_used_list_free(/*@in@*/ sam_es *restrict es,
			   /*@null@*/ sam_heap_pointer *u)
{
    sam_heap_pointer *next;

    if (u == NULL) {
	return;
    }
    next = u->next;
    sam_es_heap_dealloc(es, u->start);
    sam_es_heap_used_list_free(es, next);
}

static void
sam_es_heap_free_list_free(/*@null@*/ sam_heap_pointer *p)
{
    sam_heap_pointer *next;

    if (p == NULL) {
	return;
    }
    next = p->next;
    free(p);
    sam_es_heap_free_list_free(next);
}

static inline void
sam_es_heap_free(sam_es *restrict es)
{
    sam_es_heap_used_list_free(es, es->heap.used_list);
    sam_es_heap_free_list_free(es->heap.free_list);
    free(es->heap.a.arr);
}

static inline void
sam_es_dlhandles_free(sam_array *restrict dlhandles)
{
    free(dlhandles->arr);
}

/* TODO: collapse changes */
static void
sam_es_change_register(sam_es *restrict es,
		       const sam_es_change *restrict change)
{
    sam_es_change_list *new = sam_malloc(sizeof (sam_es_change_list));
    new->change = *change;
    new->next = NULL;
    new->prev = es->last_change;

    if (es->last_change != NULL) {
	es->last_change->next = new;
    }
    es->last_change = new;
    if (es->first_change == NULL) {
	es->first_change = new;
    }
}

static sam_es_loc *
sam_es_loc_new(sam_pa pa, char *label)
{
    sam_es_loc *loc = sam_malloc(sizeof (sam_es_loc));

    loc->pa = pa;
    sam_array_init(&loc->labels);
    sam_array_ins(&loc->labels, label);

    return loc;
}

/* Assumes that you will insert them in order! */
static void
sam_es_loc_ins(sam_es *restrict es,
	       sam_pa line_no,
	       char *label)
{
    sam_es_loc **restrict locs = (sam_es_loc **)es->locs.arr;

    if (*locs == NULL || es->locs.len == 0
	    || locs[es->locs.len - 1]->pa != line_no) {
	sam_array_ins(&es->locs,
		      sam_es_loc_new(line_no, label));
    } else {
	sam_array_ins(&locs[es->locs.len - 1]->labels, label);
    }
}

#if 0
/*@only@*/ static int
sam_asprintf(char **restrict p,
	     const char *format,
	     ...)
{
    va_list ap;
    int	    len;

    if (p == NULL) {
	return -1;
    }

    va_start(ap, format);
    len = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    if (len < 0) {
	return -1;
    }

    *p = sam_malloc(len + 1);
    va_start(ap, format);
    len = vsnprintf(*p, len + 1, format, ap);
    va_end(ap);

    return len;
}
#endif

inline void
sam_es_bt_set(sam_es *restrict es,
		       bool bt)
{
    es->bt = bt;
}

inline bool
sam_es_bt_get(const sam_es *restrict es)
{
    return es->bt;
}

inline sam_pa
sam_es_pc_get(const sam_es *restrict es)
{
    return es->pc;
}

inline sam_pa
sam_es_pc_pp(sam_es *restrict es)
{
    return es->pc++;
}

inline void
sam_es_pc_set(sam_es *restrict es,
	      sam_pa pc)
{
    es->pc = pc;
}

inline sam_sa
sam_es_fbr_get(const sam_es *restrict es)
{
    return es->fbr;
}

inline void
sam_es_fbr_set(sam_es *restrict es,
	       sam_sa fbr)
{
    es->fbr = fbr;
}

inline size_t
sam_es_sp_get(sam_es *restrict es)
{
    return es->stack.len;
}

sam_error
sam_es_string_get(/*@in@*/ sam_es *restrict es,
		  /*@out@*/ char **restrict str,
		  sam_ha ha)
{
    size_t i = ha, len = 0;

    *str = NULL;
    for (;; ++i, ++len) {
	if (i == es->heap.a.len) {
	    sam_ma ma = {.ha = len};
	    return sam_error_segmentation_fault(es, false, ma);
	}
	sam_ml *restrict m = sam_es_heap_get(es, i);
	if(m == NULL || m->value.i == '\0') {
	    break;
	}
    }
    *str = sam_malloc(len + 1);
    for (i = 0; i <= len; ++i) {
	sam_ml *restrict m = sam_es_heap_get(es, i + ha);
	if (m == NULL || m->value.i == '\0') {
	    (*str)[i] = '\0';
	    break;
	}
	(*str)[i] = m->value.i;
    }

    return SAM_OK;
}

/*@null@*/ inline sam_ml *
sam_es_stack_pop(/*@in@*/ sam_es *restrict es)
{
    sam_es_change ch = {
	.stack  = 1,
	.remove = 1,
	.ma.sa  = es->stack.len,
    };
    sam_es_change_register(es, &ch);
    return sam_array_rem(&es->stack);
}

bool
sam_es_stack_push(/*@in@*/ sam_es *restrict es,
		  /*@only@*/ /*@out@*/ sam_ml *ml)
{
    if (es->stack.len == SAM_STACK_PTR_MAX) {
	free(ml);
	return false;
    }

    sam_es_change ch = {
	.stack = 1,
	.add   = 1,
	.ma.sa = es->stack.len,
	.ml    = ml,
    };
    sam_es_change_register(es, &ch);

    sam_array_ins(&es->stack, ml);
    return true;
}

inline size_t
sam_es_stack_len(const sam_es *restrict es)
{
    return es->stack.len;
}

inline sam_ml *
sam_es_stack_get(const sam_es *restrict es,
		 sam_sa sa)
{
    return sa < sam_es_stack_len(es)? es->stack.arr[sa]: NULL;
}

bool
sam_es_stack_set(sam_es *restrict es,
		 /*@only@*/ sam_ml *ml,
		 sam_sa  sa)
{
    if (sa >= sam_es_stack_len(es)) {
	free(ml);
	return false;
    }

    sam_es_change ch = {
	.stack = 1,
	.ma.sa = sa,
	.ml    = ml,
    };
    sam_es_change_register(es, &ch);

    free(es->stack.arr[sa]);
    es->stack.arr[sa] = ml;

    return true;
}

inline sam_ml *
sam_es_heap_get(const sam_es *restrict es,
		sam_ha ha)
{
    return ha < es->heap.a.len? es->heap.a.arr[ha]: NULL;
}

bool
sam_es_heap_set(sam_es *restrict es,
		/*@only@*/ sam_ml *ml,
		sam_ha ha)
{
    if (ha >= es->heap.a.len) {
	free(ml);
	return false;
    }

    sam_es_change ch = {
	.stack = 0,
	.ma.ha = ha,
	.ml    = ml,
    };
    sam_es_change_register(es, &ch);

    free(es->heap.a.arr[ha]);
    es->heap.a.arr[ha] = ml;

    return true;
}

bool
sam_es_labels_ins(sam_es *restrict es,
		  char *restrict label,
		  sam_pa line_no)
{
    if (sam_hash_table_ins(&es->labels, label, line_no)) {
	sam_es_loc_ins(es, line_no, label);
	return true;
    } else {
	return false;
    }
}

inline bool
sam_es_labels_get(/*@in@*/ sam_es *restrict es,
		  /*@out@*/ sam_pa *restrict pa,
		  const char *restrict name)
{
    return sam_hash_table_get(&es->labels, name, pa);
}

inline sam_array *
sam_es_locs_get(sam_es *restrict es)
{
    return &es->locs;
}

#if 0
inline const char *
sam_es_labels_get_by_pa(sam_es *restrict es,
			sam_pa pa)
{
    ;
}
#endif

inline void
sam_es_instructions_ins(sam_es *restrict es,
			sam_instruction *restrict i)
{
    sam_array_ins(&es->instructions, i);
}

/*@null@*/ inline sam_instruction *
sam_es_instructions_get(/*@in@*/ const sam_es *restrict es,
			sam_pa pa)
{
    return pa < es->instructions.len? es->instructions.arr[pa]: NULL;
}

sam_instruction *
sam_es_instructions_cur(sam_es *restrict es)
{
    return es->instructions.arr[es->pc];
}

inline size_t
sam_es_instructions_len(const sam_es *restrict es)
{
    return es->instructions.len;
}

inline size_t
sam_es_heap_len(const sam_es *restrict es)
{
    return es->heap.a.len;
}

/**
 * Allocate space on the heap. Updates the current
 * #sam_es's sam_heap#used_list
 * and sam_heap#free_list.
 *
 *  @param es The current execution state.
 *  @param size The number of memory locations to allocate.
 *
 *  @return An index into the sam_heap#a#arr, or #SAM_HEAP_PTR_MAX on
 *	    overflow.
 */
sam_ha
sam_es_heap_alloc(/*@in@*/ sam_es *restrict es,
		  size_t size)
{
    /*@null@*/ sam_heap_pointer *last = NULL;

    if (SAM_HEAP_PTR_MAX - es->heap.a.len <= size) {
	return SAM_HEAP_PTR_MAX;
    }

    for (;;) {
	if (es->heap.free_list == NULL) {
	    size_t start = es->heap.a.len;

	    for (size_t i = start; i < start + size; ++i) {
		sam_ml_value v = { .i = 0 };
		sam_array_ins(&es->heap.a,
			      sam_ml_new(v, SAM_ML_TYPE_NONE));
	    }
	    sam_es_change ch = {
		.stack = 0,
		.add = 1,
		.ma = {
		    .ha = start
		},
		.size = size,
	    };
	    sam_es_change_register(es, &ch);
	    es->heap.used_list =
		sam_es_heap_pointer_update(es->heap.used_list, start, size);
	    return start;
	}
	if (es->heap.free_list->size >= size) {
	    size_t start = es->heap.free_list->start;

	    for (size_t i = start; i < size + start; ++i) {
		es->heap.a.arr[i] = sam_malloc(sizeof (sam_ml));
	    }
	    es->heap.used_list =
		sam_es_heap_pointer_update(es->heap.used_list, start, size);
	    if (es->heap.free_list->size == size) {
		if (last == NULL) {
		    es->heap.free_list = es->heap.free_list->next;
		} else {
		    last->next = es->heap.free_list->next;
		}
		free(es->heap.free_list);
	    } else {
		es->heap.free_list->start += size;
		es->heap.free_list->size -= size;
	    }
	    sam_es_change ch = {
		.stack = 0,
		.add = 1,
		.ma = {
		    .ha = start
		},
		.size = size,
	    };
	    sam_es_change_register(es, &ch);
	    return start;
	}
	last = es->heap.free_list;
	es->heap.free_list = es->heap.free_list->next;
    }
}

sam_error
sam_es_heap_dealloc(sam_es *restrict es,
		    sam_ha ha)
{
    sam_heap_pointer *u = es->heap.used_list;
    sam_heap_pointer *last = NULL;

    if (es->heap.a.arr[ha] == NULL) {
	return SAM_OK;
    }

    for (;;) {
	if (u == NULL) {
	    return sam_error_free(es, ha);
	}
	if (u->start == ha) {
	    if (last == NULL) {
		es->heap.used_list = u->next;
	    } else {
		last->next = u->next;
	    }
	    for (size_t i = 0; i < u->size; ++i) {
		sam_ml *restrict m = es->heap.a.arr[ha + i];
		free(m);
		es->heap.a.arr[ha + i] = NULL;
	    }
	    sam_es_change ch = {
		.stack = 0,
		.remove = 1,
		.ma = {
		    .ha = u->start
		},
		.size = u->size,
	    };
	    sam_es_change_register(es, &ch);
	    es->heap.free_list =
		sam_es_heap_pointer_update(es->heap.free_list,
					   u->start, u->size);
	    free(u);
	    return SAM_OK;
	}
	if (u->start > ha) {
	    return sam_error_free(es, ha);
	}
	last = u;
	u = u->next;
    }
}

inline bool
sam_es_heap_leak_check(const sam_es *restrict es,
		       unsigned long *restrict block_count,
		       unsigned long *restrict leak_size)
{
    for (sam_heap_pointer *h = es->heap.used_list;
	 h != NULL;
	 h = h->next) {
	++block_count;
	leak_size += h->size;
    }
    return *leak_size > 0;
}

sam_io_vfprintf_func
sam_es_io_func_vfprintf(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL? NULL: es->io_dispatcher(SAM_IO_VFPRINTF).vfprintf;
}

sam_io_vfscanf_func
sam_es_io_func_vfscanf(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL? NULL: es->io_dispatcher(SAM_IO_VFSCANF).vfscanf;
}

sam_io_afgets_func
sam_es_io_func_afgets(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL? NULL: es->io_dispatcher(SAM_IO_AFGETS).afgets;
}

sam_io_bt_func
sam_es_io_func_bt(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL? NULL: es->io_dispatcher(SAM_IO_BT).bt;
}

bool
sam_es_options_get(const sam_es *restrict es, sam_options option)
{
    return (~es->options & option) == 0;
}

sam_string *
sam_es_input_get(sam_es *restrict es)
{
    return &es->input;
}

bool
sam_es_change_get(sam_es *restrict es,
		  sam_es_change *ch)
{
    if (es->first_change == NULL) {
	return false;
    }

    if (ch != NULL) {
	*ch = es->first_change->change;
    }

    sam_es_change_list *next = es->first_change->next;
    free(es->first_change);

    if (es->first_change == es->last_change) {
	es->first_change = NULL;
	es->last_change = NULL;
    } else {
	es->first_change = next;
	es->first_change->prev = NULL;
    }

    return true;
}

/*@only@*/ sam_es *
sam_es_new(const char *restrict file,
	   sam_options options,
	   /*@in@*/ sam_io_dispatcher io_dispatcher)
{
    sam_es *restrict es = sam_malloc(sizeof (sam_es));

    es->bt = false;
    es->pc = 0;
    es->fbr = 0;
    sam_array_init(&es->stack);
    sam_array_init(&es->heap.a);
    sam_array_init(&es->instructions);
    sam_array_init(&es->locs);
    sam_hash_table_init(&es->labels);
    es->heap.used_list = NULL;
    es->heap.free_list = NULL;
    es->first_change = NULL;
    es->last_change = NULL;
    es->io_dispatcher = io_dispatcher;
    es->options = options;
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_init(&es->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

    if (!sam_parse(es, file)) {
	sam_es_free(es);
	return NULL;
    }

    return es;
}

void
sam_es_free(/*@in@*/ /*@only@*/ sam_es *restrict es)
{
    if (es->input.alloc > 0) {
#if defined(HAVE_MMAN_H)
	if (es->input.mmapped) {
	    if (munmap(es->input.data, es->input.len) < 0) {
		perror("munmap");
	    }
	} else
#endif /* HAVE_MMAN_H */
	sam_string_free(&es->input);
    }

    sam_array_free(&es->stack);
    sam_es_heap_free(es);
    sam_array_free(&es->instructions);
    sam_array_free(&es->locs);
    sam_hash_table_free(&es->labels);
    while (sam_es_change_get(es, NULL));
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_free(&es->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
    free(es);
}

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
sam_error
sam_es_dlhandles_ins(/*@in@*/ sam_es *restrict es,
		     /*@observer@*/ /*@out@*/ const char *path)
{
    /*@observer@*/ void *restrict handle = dlopen(path, RTLD_NOW);
    if (handle == NULL) {
	return sam_error_dlopen(es, path, dlerror());
    }

    sam_dlhandle *restrict h = sam_malloc(sizeof (sam_dlhandle));
    h->name = path;
    h->handle = handle;
    sam_array_ins(&es->dlhandles, h);

    return SAM_OK;
}

/*@null@*/ /*@dependent@*/ sam_library_fn
sam_es_dlhandles_get(sam_es *restrict es,
		     const char *sym)
{
    sam_library_fn fn;
    for (size_t i = 0; i < es->dlhandles.len; ++i) {
	if ((fn = dlsym(((sam_dlhandle *)es->dlhandles.arr[i])->handle,
			sym)) != NULL) {
	    return fn;
	}
    }
    dlerror();

    return NULL;
}

inline void
sam_es_dlhandles_close(sam_es *restrict es)
{
    for (size_t i = 0; i < es->dlhandles.len; ++i) {
	dlclose(es->dlhandles.arr[i]);
    }
}
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
