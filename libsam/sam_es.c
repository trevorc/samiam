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
 * Revision 1.4  2007/01/06 01:07:09  trevor
 * Removed sam_main.h include.
 *
 * Revision 1.3  2007/01/04 06:16:04  trevor
 * Remove unused header dep.
 *
 * Revision 1.2  2007/01/04 06:04:54  trevor
 * Make gcc happy about dynamic loading.
 *
 * Revision 1.1  2007/01/04 05:42:58  trevor
 * Make es an opaque data structure and limit its access.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libsam/array.h>
#include <libsam/hash_table.h>
#include <libsam/es.h>
#include <libsam/util.h>

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
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

/** The parsed instructions and labels along with the current state
 *  of execution. */
struct _sam_es {
    volatile bool lock; /**< Is it unsafe to access or modify the
			     *   execution state? */
    bool bt;   /**< Is a stack trace is needed? */
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
    sam_hash_table labels;  /**< A shallow copy of the labels hash
			     *   table allocated in main and initialized
			     *   in sam_parse(). */
    sam_io_funcs io_funcs;
    sam_options  options;
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array dlhandles;    /**< Handles returned from dlopen(). */
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
};

/* Execution errors. */
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

#if 0
static sam_bool
sam_es_dlhandles_get(/*@in@*/ sam_es *es,
		     const char *name)
{
    size_t i;

    for (i = 0; i < es->dlhandles.len; ++i) {
	sam_dlhandle *handle = es->dlhandles.arr[i];
	if (strcmp(handle->name, name) == 0) {
	    return false;
	}
    }

    return true;
}
#endif

static inline void
sam_es_dlhandles_free(sam_array *restrict dlhandles)
{
    free(dlhandles->arr);
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

#if 0
/*
 * Portable implementation of round(3) for non-C99 and non-POSIX.1-2001
 * implementing systems (but returns int instead of double).
 */
sam_int
sam_round(sam_float f)
{
    if (f < 0) {
	sam_float fl = ceil(f);

	if (fabs(f - fl) >= .5) {
	    return floor(f);
	} else {
	    return fl;
	}
    } else {
	sam_float fl = floor(f);

	if (fabs(f - fl) >= .5) {
	    return ceil(f);
	} else {
	    return fl;
	}
    }
}
#endif

sam_error
sam_es_string_get(/*@in@*/ sam_es *restrict es,
		  /*@out@*/ char **restrict str,
		  sam_ha ha)
{
    size_t i = ha, len = 0;

    *str = NULL;
    for (;; ++i, ++len) {
	if (i == es->heap.a.len) {
	    sam_ma ma;
	    ma.stack = false;
	    ma.index.ha = len;
	    return sam_error_segmentation_fault(es, ma);
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
    return sam_array_rem(&es->stack);
}

bool
sam_es_stack_push(/*@in@*/ sam_es *restrict es,
		  /*@only@*/ /*@out@*/ sam_ml *m)
{
    if (es->stack.len == SAM_STACK_PTR_MAX) {
	free(m);
	return false;
    }
    sam_array_ins(&es->stack, m);
    return true;
}

inline static sam_ml *
sam_es_stack_peek_bottom(const sam_es *restrict es)
{
    return sam_es_stack_get(es, 0);
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
    free(es->heap.a.arr[ha]);
    es->heap.a.arr[ha] = ml;

    return true;
}

bool
sam_es_labels_ins(sam_es *restrict es,
		  const char *restrict label,
		  sam_pa line_no)
{
    return sam_hash_table_ins(&es->labels, label, line_no);
}

inline bool
sam_es_labels_get(/*@in@*/ sam_es *restrict es,
		  /*@out@*/ sam_pa *restrict pa,
		  const char *restrict name)
{
    return sam_hash_table_get(&es->labels, name, pa);
}

inline void
sam_es_instructions_ins (sam_es *restrict es,
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
		sam_ml_value v = { 0 };
		sam_array_ins(&es->heap.a,
			      sam_ml_new(v, SAM_ML_TYPE_NONE));
	    }
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
	    return start;
	}
	last = es->heap.free_list;
	es->heap.free_list = es->heap.free_list->next;
    }
}

sam_error
sam_es_heap_dealloc(sam_es *restrict es,
		    sam_ha index)
{
    sam_heap_pointer *u = es->heap.used_list;
    sam_heap_pointer *last = NULL;

    if (es->heap.a.arr[index] == NULL) {
	return SAM_OK;
    }

    for (;;) {
	if (u == NULL) {
	    return sam_error_free(es, index);
	}
	if (u->start == index) {
	    if (last == NULL) {
		es->heap.used_list = u->next;
	    } else {
		last->next = u->next;
	    }
	    for (size_t i = 0; i < u->size; ++i) {
		sam_ml *restrict m = es->heap.a.arr[index + i];
		free(m);
		es->heap.a.arr[index + i] = NULL;
	    }
	    es->heap.free_list =
		sam_es_heap_pointer_update(es->heap.free_list,
					   u->start, u->size);
	    free(u);
	    return SAM_OK;
	}
	if (u->start > index) {
	    return sam_error_free(es, index);
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
    /* Not released. */
    es->lock = true;

    for (size_t i = 0; i < es->dlhandles.len; ++i) {
	dlclose(es->dlhandles.arr[i]);
    }
}

sam_io_vfprintf_func
sam_es_io_funcs_vfprintf(const sam_es *restrict es)
{
    return es->io_funcs.vfprintf;
}

sam_io_vfscanf_func
sam_es_io_funcs_vfscanf(const sam_es *restrict es)
{
    return es->io_funcs.vfscanf;
}

sam_io_afgets_func
sam_es_io_funcs_afgets(const sam_es *restrict es)
{
    return es->io_funcs.afgets;
}

sam_io_bt_func
sam_es_io_funcs_bt(const sam_es *restrict es)
{
    return es->io_funcs.bt;
}

bool
sam_es_options_get(const sam_es *restrict es, sam_options option)
{
    return (~es->options & option) == 0;
}

/*@only@*/ sam_es *
sam_es_new(sam_options options,
	   /*@in@*/ sam_io_funcs *restrict io_funcs)
{
    sam_es *restrict es = sam_malloc(sizeof (sam_es));
    sam_io_funcs io_funcs_default = { NULL, NULL, NULL, NULL };

    es->lock = false;
    es->bt = false;
    es->pc = 0;
    es->fbr = 0;
    sam_array_init(&es->stack);
    sam_array_init(&es->heap.a);
    sam_array_init(&es->instructions);
    sam_hash_table_init(&es->labels);
    es->io_funcs = io_funcs == NULL? io_funcs_default: *io_funcs;
    es->options = options;
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_init(&es->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

    return es;
}

void
sam_es_free(/*@in@*/ /*@only@*/ sam_es *restrict es)
{
    sam_array_free(&es->stack);
    sam_es_heap_free(es);
    sam_array_free(&es->instructions);
    sam_hash_table_free(&es->labels);
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_free(&es->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
    free(es);
}
