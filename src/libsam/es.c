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
#include <assert.h>

#include "libsam.h"

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

#define SAM_MODULE_CUR ((sam_es_module *)es->modules.arr[sam_es_pc_get(es).m])
#define SAM_MODULE_LAST ((sam_es_module *)es->modules.arr[es->modules.len - 1])
#define SAM_MODULE(n) ((sam_es_module *)es->modules.arr[(n)])

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
typedef struct {
    /*@dependent@*/ const char *restrict name;	/**< Name of symbol loaded. */
    /*@observer@*/  void *restrict handle;	/**< Handle returned from dlopen(). */
} sam_dlhandle;
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

/* as we are simulating an abstract bytecode, there's no reason that
 * "heap allocations" be stored in a continuous heap. Each heap
 * allocation the program makes is tracked separately by samiam. This is
 * an array of heap allocations, where each heap allocation is
 * represented by the sam_heap_allocation structure */
typedef sam_array sam_heap;

typedef struct _sam_heap_allocation {
    bool free; /*is this being used as an allocation index?*/
    sam_array words; /*what's at this allocation?*/
} sam_heap_allocation;

typedef struct _sam_es_change_list sam_es_change_list;

struct _sam_es_change_list {
    sam_es_change change;
    sam_es_change_list *next;
    sam_es_change_list *prev;
};

typedef struct {
    const char *file;	    /**< The name of the file. */
    sam_array instructions; /**< A shallow copy of the instructions
			     *   array allocated in main and initialized
			     *   in sam_parse(). */
    sam_hash_table labels;  /**< A shallow copy of the labels hash
			     *   table allocated in main and initialized
			     *   in sam_parse(). Has all labels this
			     *   module can access. */
    sam_hash_table globals; /**< A shallow copy of the globals hash
			     *   table allocated in main and initialized
			     *   in sam_parse(). Has all globals this
			     *   module can access. */
    sam_array allocs;	    /**< Automatically allocated read-only data
			         and globals. */
} sam_es_module;

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

    sam_array modules;
    sam_array locs;	    /**< An array of sam_es_locs ordered
			         by pa. */

    sam_array  stack;	    /**< The sam stack, an array of {@link
			     *  sam_ml}s.  The stack pointer register is
			     *  simply the length of this array. */
    sam_heap   heap;	    /**< The sam heap, managed by the functions
			     *   #sam_es_heap_alloc and #sam_heap_free. */
    sam_hash_table symbols; /**< Export symbol table. */

    sam_es_change_list *first_change;
    sam_es_change_list *last_change;
    sam_io_dispatcher io_dispatcher;
    void *io_data;
    sam_options options;
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array dlhandles;    /**< Handles returned from dlopen(). */
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
};

static inline void
sam_es_heap_allocation_free(sam_heap_allocation *alloc)
{
    if (alloc->free) {
	free(alloc);
    } else {
	sam_array_free(&alloc->words);
	free(alloc);
    }
}

static inline sam_heap_allocation *
sam_es_heap_allocation_new(size_t size) {
    sam_heap_allocation *res;
    size_t i;

    res = malloc(sizeof(sam_heap_allocation));
    
    res->free = false;
    sam_array_init(&res->words);
    for (i=0;i<size;i++) {
	sam_ml_value v = { .i = 0 };
	sam_array_ins(&res->words, sam_ml_new(v, SAM_ML_TYPE_NONE));
    }
    return res;
}

static inline sam_heap_allocation *
sam_es_heap_unused_allocation_new() {
    sam_heap_allocation *res;
    res = malloc(sizeof(sam_heap_allocation));
    res->free = true;
    return res;
}


static inline void
sam_es_heap_free(sam_es *restrict es)
{
    size_t i;
    for (i=0;i<es->heap.len;i++) {
	sam_es_heap_allocation_free(es->heap.arr[i]);
	es->heap.arr[i] = sam_es_heap_unused_allocation_new();
    }
    sam_array_free(&es->heap);
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

    if (es->locs.len == 0 ||
	!(locs[es->locs.len - 1]->pa.m == line_no.m &&
	  locs[es->locs.len - 1]->pa.l == line_no.l)) {
	sam_array_ins(&es->locs, sam_es_loc_new(line_no, label));
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
    sam_pa pc = es->pc;
    ++es->pc.l;
    return pc;
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
sam_es_string_alloc(sam_es *restrict es,
		    char *str,
		    sam_ha *restrict res) {
    size_t i;
    sam_ha cur;

    /*XXX what if this wraps around the max number of heap
     * allocations?*/
    *res = sam_es_heap_alloc(es,strlen(str)+1);

    for (i=0, cur=*res; i<=strlen(str); i++, cur.index++) {
	sam_ml *restrict m = sam_es_heap_get(es, cur);
	m->type = SAM_ML_TYPE_INT;
	m->value.i = str[i];
    }

    return SAM_OK;
}

sam_error
sam_es_string_get(/*@in@*/ sam_es *restrict es,
		  /*@out@*/ char **restrict str,
		  sam_ha ha)
{
    size_t len = 0;
    size_t i;
    sam_ha addr = ha;

    *str = NULL;
    for (;; ++addr.index, ++len) {
	sam_ml *restrict m = sam_es_heap_get(es, addr);
	if (m == NULL) {
	    sam_ma ma = {.ha = addr};
	    return sam_error_segmentation_fault(es, false, ma);
	} else if (m->value.i == '\0') {
	    break;
	}
    }
    *str = sam_malloc(len + 1);
    addr = ha;
    for (i = 0; i <= len; ++i,++addr.index) {
	sam_ml *restrict m = sam_es_heap_get(es, addr);
	assert(m != NULL);
	if (m->value.i == '\0') {
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
    sam_heap_allocation *alloc;

    /*get allocation*/
    if (ha.alloc >= es->heap.len) {
	return NULL;
    }
    alloc = es->heap.arr[ha.alloc];

    /*get exact address*/
    if (ha.index >= alloc->words.len) {
	return NULL;
    }
    return alloc->words.arr[ha.index];
}

bool
sam_es_heap_set(sam_es *restrict es,
		/*@only@*/ sam_ml *ml,
		sam_ha ha)
{
    sam_heap_allocation *alloc;
    if (ha.alloc >= es->heap.len) {
	goto error;
    }

    alloc = es->heap.arr[ha.alloc];
    
    if (ha.index >= alloc->words.len) {
	goto error;
    }

    sam_es_change ch = {
	.stack = 0,
	.ma.ha = ha,
	.ml    = ml,
    };
    sam_es_change_register(es, &ch);

    free(alloc->words.arr[ha.index]);
    alloc->words.arr[ha.index] = ml;

    return true;
error:
    free(ml);
    return false;
}

static sam_pa *
sam_es_pa_new(sam_pa pa)
{
    sam_pa *ptr = sam_malloc(sizeof (sam_pa));
    *ptr = pa;
    return ptr;
}

static sam_ha *
sam_es_ha_new(sam_ha ha)
{
    sam_ha *ptr = sam_malloc(sizeof (sam_ha));
    *ptr = ha;
    return ptr;
}

bool
sam_es_labels_ins(sam_es *restrict es,
		  char *restrict label,
		  sam_pa line_no)
{
    if (sam_hash_table_ins(&SAM_MODULE_LAST->labels,
			   label,
			   sam_es_pa_new(line_no))) {
	sam_es_loc_ins(es, line_no, label);
	return true;
    } else {
	return false;
    }
}

inline bool
sam_es_labels_get(/*@in@*/ sam_es *restrict es,
		  /*@out@*/ sam_pa *restrict pa,
		  const char *restrict name,
		  unsigned short module)
{
    sam_pa *restrict tmp_pa =
	sam_hash_table_get(&SAM_MODULE(module)->labels, name);

    if (tmp_pa == NULL) {
	return false;
    }
    *pa = *tmp_pa;

    return true;
}

inline bool
sam_es_labels_get_cur(/*@in@*/ sam_es *restrict es,
		      /*@out@*/ sam_pa *restrict pa,
		      const char *restrict name)
{
    return sam_es_labels_get(es, pa, name, sam_es_pc_get(es).m);
}

bool
sam_es_globals_ins(sam_es *restrict es,
		  char *restrict symbol,
		  size_t size)
{
    sam_ha loc = sam_es_heap_alloc(es, size);
    return sam_hash_table_ins(&SAM_MODULE_LAST->globals,
			   symbol,
			   sam_es_ha_new(loc));
}

inline bool
sam_es_globals_get(/*@in@*/ const sam_es *restrict es,
		  /*@out@*/ sam_ha *restrict ha,
		  const char *restrict name,
		  unsigned short module)
{
    sam_ha *restrict tmp_ha =
	sam_hash_table_get(&SAM_MODULE(module)->globals, name);

    if (tmp_ha == NULL) {
	return false;
    }
    *ha = *tmp_ha;

    return true;
}

inline bool
sam_es_globals_get_cur(/*@in@*/ sam_es *restrict es,
		      /*@out@*/ sam_ha *restrict ha,
		      const char *restrict name)
{
    return sam_es_globals_get(es, ha, name, sam_es_pc_get(es).m);
}

inline sam_array *
sam_es_locs_get(sam_es *restrict es)
{
    return &es->locs;
}

inline const char *
sam_es_file_get(sam_es *restrict es,
		unsigned short module)
{
    return SAM_MODULE(module)->file;
}

inline void
sam_es_instructions_ins(sam_es *restrict es,
			sam_instruction *restrict i)
{
    sam_array_ins(&SAM_MODULE_LAST->instructions, i);
}

/*@null@*/ inline sam_instruction *
sam_es_instructions_get(/*@in@*/ const sam_es *restrict es,
			sam_pa pa)
{
    return pa.l < SAM_MODULE(pa.m)->instructions.len?
	SAM_MODULE(pa.m)->instructions.arr[pa.l]: NULL;
}

/*@null@*/ inline sam_instruction *
sam_es_instructions_get_cur(/*@in@*/ const sam_es *restrict es,
			    unsigned short line)
{
    return line < SAM_MODULE_CUR->instructions.len?
	SAM_MODULE_CUR->instructions.arr[line]: NULL;
}

sam_instruction *
sam_es_instructions_cur(sam_es *restrict es)
{
    return SAM_MODULE_CUR->instructions.arr[es->pc.l];
}

inline size_t
sam_es_instructions_len(const sam_es *restrict es,
			unsigned short module)
{
    return SAM_MODULE(module)->instructions.len;
}

inline size_t
sam_es_instructions_len_cur(const sam_es *restrict es)
{
    return SAM_MODULE_CUR->instructions.len;
}

inline unsigned short
sam_es_modules_len(const sam_es *restrict es)
{
    return es->modules.len;
}

#if 0
void
sam_es_ro_alloc(const sam_es *restrict es,
		const char *symbol,
		sam_ml_value value,
		sam_ml_type type)
{
}

void
sam_es_global_alloc(const sam_es *restrict es,
		    const char *symbol,
		    sam_ml_value value,
		    sam_ml_type type)
{
}
void
sam_es_export(const sam_es *restrict es, const char *symbol)
{
}

void
sam_es_import(const sam_es *restrict es, const char *symbol)
{
}
#endif

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
    size_t i;
    sam_ha res;
    res.index = 0;
    for (i=0;i<es->heap.len;i++) {
	if (((sam_heap_allocation*)es->heap.arr[i])->free) {
	    free(es->heap.arr[i]);
	    es->heap.arr[i] = sam_es_heap_allocation_new(size);
	    res.alloc = i;
	    goto finish;
	}
    }
    sam_array_ins(&es->heap,sam_es_heap_allocation_new(size));
    res.alloc = es->heap.len-1;
finish: 
    {
	sam_es_change ch = {
	    .stack = 0,
	    .add = 1,
	    .ma = {
		.ha = res
	    },
	    .size = size,
	};
	sam_es_change_register(es, &ch);
	return res;
    }
}

sam_error
sam_es_heap_dealloc(sam_es *restrict es,
		    sam_ha ha)
{
    size_t size;
    if (ha.index != 0)
	goto failure; /*pointer doesn't point to start of allocation*/

    if (ha.alloc >= es->heap.len)
	goto failure; /*pointer doesn't point to a valid allocation*/

    if (((sam_heap_allocation*)es->heap.arr[ha.alloc])->free)
	goto failure; /*double-free*/

    size = ((sam_heap_allocation*)es->heap.arr[ha.alloc])->words.len;

    sam_es_heap_allocation_free(es->heap.arr[ha.alloc]);
    es->heap.arr[ha.alloc] = sam_es_heap_unused_allocation_new();

    sam_es_change ch = {
	.stack = 0,
	.remove = 1,
	.ma = {
	    .ha = ha
	},
	.size = size
    };
    sam_es_change_register(es,&ch);
    return SAM_OK;
failure:
    return sam_error_free(es,ha);
}

sam_io_vfprintf_func
sam_es_io_func_vfprintf(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL?
	NULL: es->io_dispatcher(SAM_IO_VFPRINTF, es->io_data).vfprintf;
}

sam_io_vfscanf_func
sam_es_io_func_vfscanf(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL?
	NULL: es->io_dispatcher(SAM_IO_VFSCANF, es->io_data).vfscanf;
}

sam_io_afgets_func
sam_es_io_func_afgets(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL?
	NULL: es->io_dispatcher(SAM_IO_AFGETS, es->io_data).afgets;
}

sam_io_bt_func
sam_es_io_func_bt(const sam_es *restrict es)
{
    return es->io_dispatcher == NULL?
	NULL: es->io_dispatcher(SAM_IO_BT, es->io_data).bt;
}

void *
sam_es_io_data_get(const sam_es *restrict es)
{
    return es->io_data;
}

bool
sam_es_options_get(const sam_es *restrict es,
		   sam_options option)
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

/* The part of up setting up an execution state which is unrelated to
 * parsing. */
static void
sam_es_init(sam_es *restrict es)
{
    es->bt = false;
    es->pc = (sam_pa){.l = 0, .m = 0};
    es->fbr = 0;
    sam_array_init(&es->stack);
    sam_array_init(&es->heap);
    es->first_change = NULL;
    es->last_change = NULL;

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_init(&es->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
}

/* The part of taking down an execution state which is unrelated to
 * parsing. */
static void
sam_es_clear(sam_es *restrict es)
{
    sam_array_free(&es->stack);
    sam_es_heap_free(es);

    while (sam_es_change_get(es, NULL));
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_free(&es->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
}

static void
sam_es_module_free(sam_es_module *restrict module)
{
    sam_array_free(&module->instructions);
    sam_array_free(&module->allocs);
    sam_hash_table_free(&module->labels);
    sam_hash_table_free(&module->globals);
}

static sam_es_module *
sam_es_module_new(sam_es *const restrict es,
		  const char *file)
{
    sam_es_module *restrict module = sam_malloc(sizeof (sam_es_module));

    module->file = file;
    sam_array_init(&module->instructions);
    sam_array_init(&module->allocs);
    sam_hash_table_init(&module->labels);
    sam_hash_table_init(&module->globals);

    sam_array_ins(&es->modules, module);

    if (!sam_parse(es, file)) {
	return NULL;
    }

    return module;
}

/*@only@*/ sam_es *
sam_es_new(const char *restrict file,
	   sam_options options,
	   /*@in@*/ sam_io_dispatcher io_dispatcher,
	   void *io_data)
{
    sam_es *restrict es = sam_malloc(sizeof (sam_es));
    sam_es_init(es);

    es->options = options;
    es->io_dispatcher = io_dispatcher;
    es->io_data = io_data;

    sam_array_init(&es->modules);
    sam_array_init(&es->locs);
    sam_es_module *module = sam_es_module_new(es, file);

    if (module == NULL) {
	sam_es_free(es);
	return NULL;
    }

    return es;
}

void
sam_es_free(/*@in@*/ /*@only@*/ sam_es *restrict es)
{
    sam_es_clear(es);

    for (size_t i = 0; i < es->locs.len; ++i) {
	sam_es_loc *restrict loc = es->locs.arr[i];
	free(loc->labels.arr);
    }
    sam_array_free(&es->locs);

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

    for (size_t i = 0; i < es->modules.len; ++i) {
	sam_es_module_free(SAM_MODULE(i));
    }
    sam_array_free(&es->modules);

    free(es);
}

void
sam_es_reset(sam_es *restrict es)
{
    sam_es_clear(es);
    sam_es_init(es);
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
