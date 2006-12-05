/*
 * sam_execute.c    run a parsed sam program
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
 */

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(HAVE_DLFCN_H)
# include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#include "sam_util.h"
#include "sam_main.h"
#include "sam_execute.h"

/** Global indicating whether a stack trace is needed. */
static sam_bool stack_trace;

typedef struct _sam_heap_pointer {
    size_t start;
    size_t size;
    /*@null@*/ /*@only@*/ struct _sam_heap_pointer *next;
} sam_heap_pointer;

/** The sam free store. */
typedef struct {
    /**	A linked list of pointers into the array of free locations on
     * the heap.  */
    /*@null@*/ sam_heap_pointer *free_list;

    /**	A linked list of pointers into the array of used locations on
     * the heap.  */
    /*@null@*/ sam_heap_pointer *used_list;

    /** The array of all locations on the heap. */
    sam_array a;
} sam_heap;

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
typedef struct {
    /*@dependent@*/ char *name;	    /**< Name of symbol loaded. */
    /*@observer@*/  void *handle;    /**< Handle returned from dlopen(). */
} sam_dlhandle;
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

/** An element on the stack or on the heap. */
typedef struct {
    unsigned  type: 8;
    sam_value value;
} sam_memory_location;

/** The parsed instructions and labels along with the current state
 *  of execution. */
typedef struct _sam_execution_state {
    sam_program_address	pc;	/**< The index into sam_execution_state#
				 *   program pointing to the current
				 *   instruction, aka the program
				 *   counter. Incremented by
				 *   #sam_execute, not by the individual
				 *   instruction handlers. */
    size_t		fbr;	/**< The frame base register. */

    /* stack pointer is stack->len */
    sam_array  stack;		/**< The sam stack, an array of {@link
				 *   sam_memory_location}s. The stack
				 *   pointer register is simply the length
				 *   of this array. */
    sam_heap   heap;		/**< The sam heap, managed by the
				 *   functions #sam_heap_alloc and
				 *   #sam_heap_free. */
    sam_array  *program;	/**< A shallow copy of the instructions
				 *   array allocated in main and
				 *   initialized in sam_parse(). */
    sam_array  *labels;		/**< A shallow copy of the labels
				 *   array allocated in main and
				 *   initialized in sam_parse(). */
    sam_io_funcs *io_funcs;
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array dlhandles;	/**< Handles returned from dlopen(). */
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
} sam_execution_state;

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

/*
 * Portable implementation of round(3) for non-C99 and non-POSIX.1-2001
 * implementing systems (but returns int instead of double).
 */
static int
sam_round(sam_float f)
{
    sam_float fl = (sam_float)floor(f);

    if (f - fl >= .5) {
	return (int)ceil(f);
    } else {
	return (int)fl;
    }
}

static sam_memory_location *
sam_memory_location_new(sam_value v,
			sam_type  t)
{
    sam_memory_location *m = sam_malloc(sizeof (sam_memory_location));

    /* entire width of m must be initialized */
    memset(m, 0, sizeof (sam_memory_location));

    m->type = t;
    m->value = v;
    return m;
}

/*@null@*/ static sam_memory_location *
sam_pop(sam_execution_state *s)
{
    return sam_array_rem(&s->stack);
}

static void
sam_push(/*@in@*/ sam_execution_state *s,
	 /*@only@*/ /*@out@*/ sam_memory_location *m)
{
    sam_array_ins(&s->stack, m);
}

/*@observer@*/ static const char *
sam_type_to_string(sam_type t)
{
    switch (t) {
	case TYPE_INT:	    return "integer";
	case TYPE_FLOAT:    return "float";
	case TYPE_CHAR:	    return "character";
	case TYPE_LABEL:    return "label";
	case TYPE_STR:	    return "string";
	case TYPE_MA:	    return "memory address";
	case TYPE_PA:	    return "program address";
	case TYPE_NONE:
	default:	    return "nonetype";
    }
}

static char
sam_type_to_char(sam_type t)
{
    switch (t) {
	case TYPE_INT:	    return 'I';
	case TYPE_FLOAT:    return 'F';
	case TYPE_CHAR:	    return 'C';
	case TYPE_LABEL:    return 'L';
	case TYPE_STR:	    return 'S';
	case TYPE_MA:	    return 'M';
	case TYPE_PA:	    return 'P';
	case TYPE_NONE:
	default:	    return '?';
    }
}

static void
sam_print_char(sam_char c)
{
    if (isprint(c)) {
	fprintf(stderr, "'%c'", (char)c);
	return;
    }
    fputs("\'\\", stderr);
    switch (c) {
	case '\a':
	    fputc('a', stderr);
	    break;
	case '\b':
	    fputc('b', stderr);
	    break;
	case '\f':
	    fputc('f', stderr);
	    break;
	case '\n':
	    fputc('n', stderr);
	    break;
	case '\r':
	    fputc('r', stderr);
	    break;
	case '\t':
	    fputc('t', stderr);
	    break;
	case '\v':
	    fputc('v', stderr);
	    break;
	default:
	    fprintf(stderr, "%d", c);
    }
    fputc('\'', stderr);
}

static void
sam_print_value(sam_value v,
		sam_type  t)
{
    switch (t) {
	case TYPE_INT:
	    fprintf(stderr, "%ld", v.i);
	    break;
	case TYPE_FLOAT:
	    fprintf(stderr, "%.3f", v.f);
	    break;
	case TYPE_CHAR:
	    sam_print_char(v.c);
	    break;
	case TYPE_STR:
	case TYPE_LABEL:
	    fprintf(stderr, "\"%s\"", v.s);
	    break;
	case TYPE_MA:
	    fprintf(stderr, "%u%c",
		    (unsigned)v.ma.index,
		    (sam_bool)v.ma.stack? 'S': 'H');
	    break;
	case TYPE_PA:
	    fprintf(stderr, "%u", (unsigned)v.pa);
	    break;
	default:
	case TYPE_NONE:
	    fputc('?', stderr);
	    break;
    }
}

/* Execution errors. */
static sam_error
sam_error_optype(sam_execution_state *s)
{
    if ((options & quiet) == 0) {
	sam_instruction *cur = s->program->arr[s->pc];
	fprintf(stderr,
		"error: bad operand type given: %s.\n",
		sam_type_to_string(cur->optype));
	stack_trace = TRUE;
    }
    return SAM_EOPTYPE;
}

static sam_error
sam_error_segmentation_fault(sam_memory_address ma)
{
    if ((options & quiet) == 0) {
	fprintf(stderr,
		"error: segmentation fault. attempt to access illegal "
		"memory at %s address %u.\n",
		(sam_bool)ma.stack? "stack": "heap",
		(unsigned)ma.index);
	stack_trace = TRUE;
    }
    return SAM_ESEGFAULT;
}

static sam_error
sam_error_free(size_t index)
{
    if ((options & quiet) == 0) {
	fprintf(stderr,
		"error: attempt to free nonexistant or unused heap "
		"address %uH\n",
		(unsigned)index);
	stack_trace = TRUE;
    }
    return SAM_EFREE;
}

static sam_error
sam_error_stack_underflow(void)
{
    if ((options & quiet) == 0) {
	fputs("error: stack underflow.\n", stderr);
	stack_trace = TRUE;
    }
    return SAM_ESTACK_UNDRFLW;
}

static sam_error
sam_error_type_conversion(sam_type to,
			  sam_type found,
			  sam_type expected)
{
    if ((options & quiet) == 0) {
	fprintf(stderr,
		"warning: invalid type conversion to %s.\n"
		"\texpected: %s.\n"
		"\tfound: %s.\n",
		sam_type_to_string(to),
		sam_type_to_string(found),
		sam_type_to_string(expected));
	stack_trace = TRUE;
    }
    return SAM_ETYPE_CONVERT;
}

static sam_error
sam_error_final_stack_state(void)
{
    if ((options & quiet) == 0) {
	fputs("warning: more than one element left on stack.\n", stderr);
	stack_trace = TRUE;
    }
    return SAM_EFINAL_STACK;
}

static void
sam_check_for_leaks(sam_execution_state *s)
{
    if ((options & quiet) == 0) {
	unsigned	  block_count = 0;
	size_t		  leak_size = 0;
	sam_heap_pointer *h = s->heap.used_list;

	for (; h != NULL; h = h->next) {
	    ++block_count;
	    leak_size += h->size;
	}

	if (leak_size > 0) {
	    fprintf(stderr,
		    "warning: your program leaks %u byte%c in %u "
		    "block%c.\n",
		    (unsigned)leak_size, leak_size == 1? '\0': 's',
		    block_count, block_count == 1? '\0': 's');
	}
    }
}

/**
 * The stop instruction was not found when there were no more
 * instructions left to execute.
 */
static void
sam_error_forgot_stop(void)
{
    if ((options & quiet) == 0) {
	fputs("warning: final instruction must be STOP.\n", stderr);
	stack_trace = TRUE;
    }
}

/**
 * The last item on the stack at program termination was not of type
 * integer.
 *
 *  @param s The current state of execution.
 */
static void
sam_error_retval_type(sam_execution_state *s)
{
    if ((options & quiet) == 0) {
	sam_memory_location *m = s->stack.arr[0];

	fprintf(stderr,
		"warning: expected bottom of stack to contain an integer (found: %s).\n",
		sam_type_to_string(m->type));
	stack_trace = TRUE;
    }
}

static sam_error
sam_error_unknown_identifier(const char *name)
{
    if ((options & quiet) == 0) {
	fprintf(stderr, "error: unknown identifier found: %s\n", name);
	stack_trace = TRUE;
    }
    return SAM_EUNKNOWN_IDENT;
}

static sam_error
sam_error_stack_input(sam_execution_state *s,
		      sam_bool		   first,
		      sam_type		   found,
		      sam_type		   expected)
{
    if ((options & quiet) == 0) {
	sam_instruction *cur = s->program->arr[s->pc];

	fprintf(stderr,
		"error: invalid type of %s stack argument to %s.\n"
		"found: %s\n"
		"expected: %s\n",
		first? "first": "second",
		cur->name,
		sam_type_to_string(found),
		sam_type_to_string(expected));
	stack_trace = TRUE;
    }

    return SAM_ESTACK_INPUT;
}

static sam_error
sam_error_stack_input1(sam_execution_state *s,
		       sam_type		   found,
		       sam_type		   expected)
{
    return sam_error_stack_input(s, TRUE, found, expected);
}

static sam_error
sam_error_stack_input2(sam_execution_state *s,
		       sam_type		   found,
		       sam_type		   expected)
{
    return sam_error_stack_input(s, FALSE, found, expected);
}

static sam_error
sam_error_division_by_zero(void)
{
    if ((options & quiet) == 0) {
	fputs("error: division by zero\n", stderr);
	stack_trace = TRUE;
    }

    return SAM_EDIVISION;
}

static sam_error
sam_error_io(void)
{
    if ((options & quiet) == 0) {
	perror("error: input/output error");
	stack_trace = TRUE;
    }

    errno = 0;
    return SAM_EIO;
}

static void
sam_error_number_format(const char *buf)
{
    if ((options & quiet) == 0) {
	fprintf(stderr,
		"warning: \"%s\" could not be parsed as an integer.\n",
		buf);
	stack_trace = TRUE;
    }
}

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
static sam_error
sam_error_dlopen(const char *filename,
		 char	    *reason)
{
    if ((options & quiet) == 0) {
	fprintf(stderr,
		"error: couldn't import library %s (%s).\n",
		filename, reason);
	stack_trace = TRUE;
    }

    return SAM_EDLOPEN;
}

static sam_error
sam_error_dlsym(sam_execution_state *s)
{
    if ((options & quiet) == 0) {
	sam_instruction *cur = s->program->arr[s->pc];

	fprintf(stderr,
		"error: couldn't call %s (not found).\n",
		cur->operand.s);

	stack_trace = TRUE;
    }

    return SAM_EDLSYM;
}
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

/* Opcode utility functions. */
static sam_bool
sam_label_lookup(/*@in@*/ sam_execution_state *s,
		 /*@out@*/ sam_program_address *pa,
		 const char *name)
{
    size_t i;

    for (i = 0; i < s->labels->len; ++i) {
	sam_label *label = s->labels->arr[i];
	if (strcmp(label->name, name) == 0) {
	    *pa = label->pa;
	    return TRUE;
	}
    }

    *pa = 0;
    return FALSE;
}

#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
static sam_dlhandle *
sam_dlhandle_new(/*@dependent@*/ char *name,
		 /*@observer@*/  void *handle)
{
    sam_dlhandle *h = sam_malloc(sizeof(sam_dlhandle));
    h->name = name;
    h->handle = handle;
    return h;
}

static sam_bool
sam_dllookup(/*@in@*/ sam_execution_state *s,
	     const char *name)
{
    size_t i;

    for (i = 0; i < s->labels->len; ++i) {
	sam_label *label = s->labels->arr[i];
	if (strcmp(label->name, name) == 0) {
	    return TRUE;
	}
    }

    return FALSE;
}

static sam_error
sam_dlopen(/*@in@*/  sam_execution_state *s,
	   /*@observer@*/ /*@out@*/ char *filename)
{
    /*@observer@*/ void *handle;

    if ((handle = dlopen(filename, RTLD_NOW)) == NULL) {
	return sam_error_dlopen(filename, dlerror());
    }
    sam_array_ins(&s->dlhandles, sam_dlhandle_new(filename, handle));

    return SAM_OK;
}

/*@null@*/ /*@dependent@*/ static void **
(*sam_dlsym(sam_execution_state *s))(void)
{
    sam_instruction    *cur = s->program->arr[s->pc];
    void	    **(*fn)(void);
    size_t		i;

    for (i = 0; i < s->dlhandles.len; ++i) {
	sam_dlhandle *h = s->dlhandles.arr[i];
	if ((*(void **)(&fn) =
	     dlsym(h->handle, cur->operand.s)) != NULL) {
	    return fn;
	}
    }
    (void)dlerror();

    return NULL;
}
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */

/* The sam allocator. */
/*@only@*/ static sam_heap_pointer *
sam_heap_pointer_new(size_t start,
		     size_t size,
		     /*@only@*/ /*@null@*/ sam_heap_pointer *next)
{
    sam_heap_pointer *p = sam_malloc(sizeof(sam_heap_pointer));
    p->start = start;
    p->size = size;
    p->next = next;
    return p;
}

/** Update the linked list of pointers to used locations in the heap to
 * reflect a newly allocated or freed block. */
static sam_heap_pointer *
sam_heap_pointer_update(/*@only@*/ sam_heap_pointer *p,
			size_t start,
			size_t size)
{
    sam_heap_pointer *head, *last;

    if (p == NULL) {
	return sam_heap_pointer_new(start, size, NULL);
    }
    if (p->start > start) {
	return sam_heap_pointer_new(start, size, p);
    }
    head = last = p;
    p = p->next;

    for (;;) {
	if (p == NULL) {
	    last->next = sam_heap_pointer_new(start, size, NULL);
	    return head;
	}
	if (p->start > start) {
	    last->next = sam_heap_pointer_new(start, size, p);
	    return head;
	}
	last = p;
	p = p->next;
    }
}

/* maybe combine sam_heap_alloc and sam_heap_dealloc? */

/**
 * Allocate space on the heap. Updates the current
 * #sam_execution_state's sam_heap#used_list
 * and sam_heap#free_list.
 *
 *  @param s The current execution state.
 *  @param size The number of memory locations to allocate.
 *
 *  @return An index into the sam_heap#a#arr.
 */
static size_t
sam_heap_alloc(sam_execution_state *s,
	       size_t		    size)
{
    sam_heap_pointer *f = s->heap.free_list;
    /*@null@*/ sam_heap_pointer *last = NULL;

    for (;;) {
	if (f == NULL) {
	    size_t i, start = s->heap.a.len;

	    for (i = start; i < start + size; ++i) {
		sam_value v = { 0 };
		sam_array_ins(&s->heap.a,
			      sam_memory_location_new(v, TYPE_NONE));
	    }
	    s->heap.used_list = sam_heap_pointer_update(s->heap.used_list,
							start, size);
	    return start;
	}
	if (f->size >= size) {
	    size_t i, start = f->start;

	    for (i = start; i < size + start; ++i) {
		s->heap.a.arr[i] = sam_malloc(sizeof(sam_memory_location));
	    }
	    s->heap.used_list = sam_heap_pointer_update(s->heap.used_list,
							start, size);
	    if (f->size == size) {
		if (last == NULL) {
		    s->heap.free_list = f->next;
		} else {
		    last->next = f->next;
		}
		free(f);
	    } else {
		f->start += size;
		f->size -= size;
	    }
	    return start;
	}
	last = f;
	f = f->next;
    }
}

static sam_error
sam_heap_dealloc(sam_heap *heap,
		 size_t	   index)
{
    sam_heap_pointer	*u = heap->used_list;
    sam_heap_pointer	*last = NULL;

    if (heap->a.arr[index] == NULL) {
	return SAM_OK;
    }

    for (;;) {
	if (u == NULL) {
	    return sam_error_free(index);
	}
	if (u->start == index) {
	    size_t i;

	    if (last == NULL) {
		heap->used_list = u->next;
	    } else {
		last->next = u->next;
	    } for (i = 0; i < u->size; ++i) {
		sam_memory_location *m = heap->a.arr[index + i];
		free(m);
		heap->a.arr[index + i] = NULL;
	    }
	    heap->free_list = sam_heap_pointer_update(heap->free_list,
						      u->start, u->size);
	    free(u);
	    return SAM_OK;
	}
	if (u->start > index) {
	    return sam_error_free(index);
	}
	last = u;
	u = u->next;
    }
}

static void
sam_heap_used_list_free(/*@in@*/   sam_heap	    *heap,
			/*@null@*/ sam_heap_pointer *u)
{
    sam_heap_pointer *next;

    if (u == NULL) {
	return;
    }
    next = u->next;
    sam_heap_dealloc(heap, u->start);
    sam_heap_used_list_free(heap, next);
}

static void
sam_heap_free_list_free(/*@null@*/ sam_heap_pointer *p)
{
    sam_heap_pointer *next;

    if (p == NULL) {
	return;
    }
    next = p->next;
    free(p);
    sam_heap_free_list_free(next);
}

static void
sam_heap_free(sam_heap *heap)
{
    sam_heap_used_list_free(heap, heap->used_list);
    sam_heap_free_list_free(heap->free_list);
    free(heap->a.arr);
}

static sam_error
sam_sp_shift(sam_execution_state *s,
	     size_t		  sp)
{
    while (sp < s->stack.len) {
	sam_memory_location *m;
	if ((m = sam_pop(s)) == NULL) {
	    return sam_error_stack_underflow();
	}
	free(m);
    }
    while (sp > s->stack.len) {
	sam_value v = { 0 };
	sam_push(s, sam_memory_location_new(v, TYPE_NONE));
    }

    return SAM_OK;
}

static sam_error
sam_pushabs(sam_execution_state *s,
	    sam_memory_address   ma)
{
    sam_memory_location *m;

    if ((sam_bool)ma.stack) {
	if (ma.index >= s->stack.len) {
	    return sam_error_segmentation_fault(ma);
	}
	m = s->stack.arr[ma.index];
    } else {
	if (ma.index >= s->heap.a.len ||
	    s->heap.a.arr[ma.index] == NULL) {
	    return sam_error_segmentation_fault(ma);
	}
	m = s->heap.a.arr[ma.index];
    }
    sam_push(s, sam_memory_location_new(m->value, m->type));

    return SAM_OK;
}

static sam_error
sam_storeabs(sam_execution_state *s,
	     sam_memory_location *m,
	     sam_memory_address   ma)
{
    if ((sam_bool)ma.stack) {
	if (ma.index >= s->stack.len) {
	    return sam_error_segmentation_fault(ma);
	}
	free(s->stack.arr[ma.index]);
	s->stack.arr[ma.index] = m;
    } else {
	if (ma.index >= s->heap.a.len) {
	    return sam_error_segmentation_fault(ma);
	}
	free(s->heap.a.arr[ma.index]);
	s->heap.a.arr[ma.index] = m;
    }

    return SAM_OK;
}

static sam_error
sam_addition(sam_execution_state *s,
	     sam_bool		  add)
{
    sam_memory_location *m1, *m2;
    int addfactor = add? 1: -1;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m2);
	return sam_error_stack_underflow();
    }
    if (m1->type == TYPE_FLOAT) {
	sam_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    if (m2->type == TYPE_FLOAT ||
	(m1->type == m2->type &&
	 (m1->type == TYPE_MA || m1->type == TYPE_PA))) {
	sam_type t = m2->type;
	free(m1);
	free(m2);
	return sam_error_stack_input2(s, t, TYPE_INT);
    }

    /* first arg is a memory address, second arg is an int */
    if (m1->type == TYPE_MA &&
	(m2->type == TYPE_INT || m2->type == TYPE_CHAR)) {
	/* user could set an illegal index here */
	m1->value.ma.index = (size_t)((int)m1->value.ma.index +
				      addfactor * m2->value.i);

	free(m2);
	sam_push(s, m1);
	return SAM_OK;
    }
    /* first arg is an int, second arg is a memory address */
    if ((m1->type == TYPE_INT || m1->type == TYPE_CHAR) &&
	m2->type == TYPE_MA) {
	/* user could set an illegal index here */
	m1->value.ma.index = m1->value.i + addfactor * m2->value.ma.index;
	m1->value.ma.stack = m2->value.ma.stack;
	m1->type = TYPE_MA;
	free(m2);
	sam_push(s, m1);
	return SAM_OK;
    }
    /* first arg is a program address, second arg is an int */
    if ((m1->type == TYPE_PA) &&
	(m2->type == TYPE_INT || m2->type == TYPE_CHAR)) {
	/* user could set an illegal index here */
	m1->value.pa = m1->value.pa + addfactor * 
	    (sam_program_address)m2->value.i;
	free(m2);
	sam_push(s, m1);
	return SAM_OK;
    }
    /* first arg is an int, second arg is a program address */
    if ((m1->type == TYPE_INT || m1->type == TYPE_CHAR) &&
	m2->type == TYPE_PA) {
	/* user could set an illegal index here */
	m1->value.pa = (sam_program_address)m1->value.i +
	    addfactor * m2->value.pa;
	m1->type = TYPE_PA;
	free(m2);
	sam_push(s, m1);
	return SAM_OK;
    }

    m1->value.i = m1->value.i + addfactor * m2->value.i;
    m1->type = TYPE_INT;
    sam_push(s, m1);

    free(m2);
    return SAM_OK;
}

static sam_error
sam_integer_arithmetic(sam_execution_state		*s, 
		       sam_integer_arithmetic_operation  op)
{
    sam_memory_location *m1, *m2;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m2);
	return sam_error_stack_underflow();
    }
    if (m1->type != TYPE_INT && m1->type != TYPE_CHAR) {
	sam_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    if (m2->type != TYPE_INT && m2->type != TYPE_CHAR) {
	sam_type t = m2->type;
	free(m1);
	free(m2);
	return sam_error_stack_input2(s, t, TYPE_INT);
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
	    if(m2->value.i == 0) {
		free(m1);
		free(m2);
		return sam_error_division_by_zero();
	    }
	    m1->value.i = m1->value.i / m2->value.i;
	    break;
	case SAM_OP_MOD:
	    if(m2->value.i == 0) {
		free(m1);
		free(m2);
		return sam_error_division_by_zero();
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
	    break;
	case SAM_OP_GREATER:
	    m1->value.i = m1->value.i > m2->value.i;
	case SAM_OP_LESS:
	    m1->value.i = m1->value.i < m2->value.i;
	    break;
    }
    sam_push(s, m1);

    free(m2);
    return SAM_OK;
}

static sam_bool
sam_float_equal(sam_float a,
		sam_float b)
{
    return fabs(a - b) <= ((fabs(b) > fabs(a)?
	  fabs(b): fabs(a)) * SAM_EPSILON);
}

static sam_error
sam_float_arithmetic(sam_execution_state	    *s,
		     sam_float_arithmetic_operation  op)
{
    sam_memory_location *m1, *m2;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m2->type != TYPE_FLOAT) {
	sam_type t = m2->type;
	free(m2);
	return sam_error_stack_input1(s, t, TYPE_FLOAT);
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m2);
	return sam_error_stack_underflow();
    }
    if (m1->type != TYPE_FLOAT) {
	sam_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input2(s, t, TYPE_INT);
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
    m1->type = TYPE_FLOAT;
    sam_push(s, m1);

    free(m2);
    return SAM_OK;
}

static sam_error
sam_unary_arithmetic(sam_execution_state	   *s,
		     sam_unary_arithmetic_operation op)
{
    sam_memory_location *m1;

    if ((m1 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m1->type != TYPE_INT && m1->type != TYPE_CHAR) {
	sam_type t = m1->type;
	free(m1);
	return sam_error_stack_input1(s, t, TYPE_INT);
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
    sam_push(s, m1);

    return SAM_OK;
}

static sam_error
sam_bitshift(sam_execution_state *s,
	     sam_bool		  left)
{
    sam_memory_location *m;
    sam_instruction	*cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_INT) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    if (left) {
	m->value.i <<= cur->operand.i;
    } else {
	m->value.i >>= cur->operand.i;
    }
    sam_push(s, m);

    return SAM_OK;
}
static sam_error
sam_bitshiftind(sam_execution_state *s,
		sam_bool		  left)
{
    sam_memory_location *m1, *m2;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m2->type != TYPE_INT) {
	sam_type t = m2->type;
	free(m2);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m2);
	return sam_error_stack_underflow();
    }
    if (m1->type != TYPE_INT) {
	sam_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input2(s, t, TYPE_INT);
    }
    if (left) {
	m1->value.i <<= m2->value.i;
    } else {
	m1->value.i >>= m2->value.i;
    }
    sam_push(s, m1);

    free(m1);
    return SAM_OK;
}

static sam_error 
sam_get_jump_target(sam_execution_state *s,
		    sam_program_address *p)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype == TYPE_INT) {
	/* subtract 1 when we change the pa because it will be
	 * incremented by the loop in sam_execute */
	*p = cur->operand.pa - 1;
    } else if (cur->optype == TYPE_LABEL) {
	sam_value v;

	if (sam_label_lookup(s, &v.pa, cur->operand.s)) {
	    /* as above */
	    *p = v.pa - 1;
	} else {
	    return sam_error_unknown_identifier(cur->operand.s);
	}
    } else {
	return sam_error_optype(s);
    }

    return SAM_OK;
}

static sam_error
sam_read_number(sam_execution_state *s,
		sam_type	     t)
{
    sam_value  v;
    char       buf[36];
    char      *endptr;

    if (fgets(buf, 36, stdin) == NULL) {
	return sam_error_io();
    }
    errno = 0;
    if (t == TYPE_INT) {
	v.i = strtol(buf, &endptr, 0);
    } else if (t == TYPE_FLOAT) {
	v.f = (sam_float)strtod(buf, &endptr);
    }
    if (errno) {
	return sam_error_io();
    }
    if (*endptr != '\0') {
	sam_error_number_format(buf);
    }

    sam_push(s, sam_memory_location_new(v, t));
    return SAM_OK;
}

static sam_error
sam_putchar(sam_char c)
{
    return putchar(c) >= 0 ? SAM_OK: sam_error_io();
}

/* Opcode implementations. */
static sam_error
sam_op_ftoi(sam_execution_state *s)
{
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_FLOAT) {
	sam_type t = m->type;
	free(m);
	return sam_error_type_conversion(TYPE_INT, t, TYPE_FLOAT);
    }
    m->value.i = (int)floor(m->value.f);
    m->type = TYPE_INT;
    sam_push(s, m);

    return SAM_OK;
}

static sam_error
sam_op_ftoir(sam_execution_state *s)
{
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_FLOAT) {
	sam_type t = m->type;
	free(m);
	return sam_error_type_conversion(TYPE_INT, t, TYPE_FLOAT);
    }
    m->value.i = sam_round(m->value.f);
    m->type = TYPE_INT;
    sam_push(s, m);

    return SAM_OK;
}

static sam_error
sam_op_itof(sam_execution_state *s)
{
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_INT) {
	sam_type t = m->type;
	free(m);
	return sam_error_type_conversion(TYPE_FLOAT, t, TYPE_INT);
    }
    m->value.f = (sam_float)m->value.i;
    m->type = TYPE_FLOAT;
    sam_push(s, m);

    return SAM_OK;
}

static sam_error
sam_op_pushimm(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    sam_push(s, sam_memory_location_new(cur->operand, cur->optype));

    return SAM_OK;
}

static sam_error
sam_op_pushimmf(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_FLOAT) {
	return sam_error_optype(s);
    }
    sam_push(s, sam_memory_location_new(cur->operand, cur->optype));

    return SAM_OK;
}

static sam_error
sam_op_pushimmch(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_CHAR) {
	return sam_error_optype(s);
    }
    sam_push(s, sam_memory_location_new(cur->operand, cur->optype));

    return SAM_OK;
}

static sam_error
sam_op_pushimmma(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];
    sam_value v;

    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    v.ma.stack = (unsigned)TRUE;
    /* weird things happen when user pushes a negative operand */
    v.ma.index = (size_t)cur->operand.i;
    sam_push(s, sam_memory_location_new(v, TYPE_MA));

    return SAM_OK;
}

static sam_error
sam_op_pushimmpa(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype == TYPE_INT) {
	sam_push(s, sam_memory_location_new(cur->operand, TYPE_PA));
    } else if (cur->optype == TYPE_LABEL) {
	sam_value v;

	if (sam_label_lookup(s, &v.pa, cur->operand.s)) {
	    sam_push(s, sam_memory_location_new(v, TYPE_PA));
	} else {
	    return sam_error_unknown_identifier(cur->operand.s);
	}
    } else {
	return sam_error_optype(s);
    }

    return SAM_OK;
}

static sam_error
sam_op_pushimmstr(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];
    sam_memory_location *m;
    sam_value v;
    size_t len, i;

    if (cur->optype != TYPE_STR) {
	return sam_error_optype(s);
    }
    len = strlen(cur->operand.s);
    v.ma.stack = (unsigned)FALSE;
    v.ma.index = sam_heap_alloc(s, len + 1);
    for (i = 0; i < len; ++i) {
	m = s->heap.a.arr[i + v.ma.index];
	m->value.c = cur->operand.s[i];
	m->type = TYPE_CHAR;
    }
    m = s->heap.a.arr[i + v.ma.index];
    m->value.c = '\0';
    m->type = TYPE_CHAR;

    sam_push(s, sam_memory_location_new(v, TYPE_MA));

    return SAM_OK;
}

static sam_error
sam_op_pushsp(sam_execution_state *s)
{
    sam_value v;

    v.ma.stack = (unsigned)TRUE;
    v.ma.index = s->stack.len;
    sam_push(s, sam_memory_location_new(v, TYPE_MA));

    return SAM_OK;
}

static sam_error
sam_op_pushfbr(sam_execution_state *s)
{
    sam_value v;

    v.ma.stack = (unsigned)TRUE;
    v.ma.index = s->fbr;
    sam_push(s, sam_memory_location_new(v, TYPE_MA));

    return SAM_OK;
}

static sam_error
sam_op_popsp(sam_execution_state *s)
{
    sam_memory_location *m;
    size_t		 index;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_MA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_MA);
    }
    index = m->value.ma.index;
    free(m);

    sam_sp_shift(s, index);
    return SAM_OK;
}

static sam_error
sam_op_popfbr(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_memory_address   ma;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_MA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_MA);
    }
    ma = m->value.ma;
    free(m);
    s->fbr = ma.index;

    return SAM_OK;
}

static sam_error
sam_op_dup(sam_execution_state *s)
{
    sam_memory_location *m1, *m2;

    if ((m1 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    m2 = sam_memory_location_new(m1->value, m1->type);

    sam_push(s, m1);
    sam_push(s, m2);

    return SAM_OK;
}

static sam_error
sam_op_swap(sam_execution_state *s)
{
    sam_memory_location *m1, *m2;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m1);
	return sam_error_stack_underflow();
    }
    sam_push(s, m2);
    sam_push(s, m1);

    return SAM_OK;
}

static sam_error
sam_op_addsp(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    if (cur->operand.i + (int)s->stack.len < 0) {
	return sam_error_stack_underflow();
    }
    return sam_sp_shift(s, (size_t)cur->operand.i + s->stack.len);
}

static sam_error
sam_op_malloc(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_value v;
    size_t i;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_INT) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    if (m->value.i == 0) {
	m->value.i = 1;
    }
    v.ma.stack = (unsigned)FALSE;
    v.ma.index = sam_heap_alloc(s, m->value.i);
    for (i = v.ma.index; i < (size_t)m->value.i + v.ma.index; ++i) {
	sam_memory_location *m = s->heap.a.arr[i];
	m->type = TYPE_NONE;
	m->value.i = 0;
    }
    sam_push(s, sam_memory_location_new(v, TYPE_MA));

    free(m);
    return SAM_OK;
}

static sam_error
sam_op_free(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_memory_address   ma;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_MA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_MA);
    }
    ma = m->value.ma;
    free(m);
    if (ma.index >= s->heap.a.len) {
	return sam_error_free(ma.index);
    }
    return sam_heap_dealloc(&s->heap, ma.index);
}

static sam_error
sam_op_pushind(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_memory_address   ma;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_MA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_MA);
    }
    ma = m->value.ma;
    free(m);

    return sam_pushabs(s, ma);
}

static sam_error
sam_op_storeind(sam_execution_state *s)
{
    /*@null@*/ sam_memory_location *m1, *m2;
    sam_memory_address ma;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m2);
	return sam_error_stack_underflow();
    }
    if (m1->type != TYPE_MA) {
	sam_type t = m1->type;
	free(m1);
	free(m2);
	return sam_error_stack_input1(s, t, TYPE_MA);
    }
    ma = m1->value.ma;
    free(m1);

    return sam_storeabs(s, m2, ma);
}

/* cannot be used to push from the heap. */
static sam_error
sam_op_pushabs(sam_execution_state *s)
{
    sam_memory_address   ma;
    sam_instruction	*cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    ma.stack = (unsigned)TRUE;
    ma.index = (size_t)cur->operand.i;

    return sam_pushabs(s, ma);
}

/* cannot be used to store onto the heap. */
static sam_error
sam_op_storeabs(sam_execution_state *s)
{
    sam_memory_address   ma;
    sam_instruction	*cur = s->program->arr[s->pc];
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    ma.stack = (unsigned)TRUE;
    ma.index = (size_t)cur->operand.i;

    return sam_storeabs(s, m, ma);
}

/* cannot be used to push from the heap. */
static sam_error
sam_op_pushoff(sam_execution_state *s)
{
    sam_memory_address   ma;
    sam_instruction	*cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    ma.stack = (unsigned)TRUE;
    ma.index = s->fbr + (size_t)cur->operand.i;

    return sam_pushabs(s, ma);
}

/* cannot be used to store onto the heap. */
static sam_error
sam_op_storeoff(sam_execution_state *s)
{
    sam_memory_address   ma;
    sam_instruction	*cur = s->program->arr[s->pc];
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (cur->optype != TYPE_INT) {
	return sam_error_optype(s);
    }
    ma.stack = (unsigned)TRUE;
    ma.index = s->fbr + (size_t)cur->operand.i;

    return sam_storeabs(s, m, ma);
}

static sam_error
sam_op_add(sam_execution_state *s)
{
    return sam_addition(s, TRUE);
}

static sam_error
sam_op_sub(sam_execution_state *s)
{
    return sam_addition(s, FALSE);
}

static sam_error
sam_op_times(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_TIMES);
}

static sam_error
sam_op_div(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_DIV);
}

static sam_error
sam_op_mod(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_MOD);
}

static sam_error
sam_op_addf(sam_execution_state *s)
{
    return sam_float_arithmetic(s, SAM_OP_ADDF);
}

static sam_error
sam_op_subf(sam_execution_state *s)
{
    return sam_float_arithmetic(s, SAM_OP_SUBF);
}

static sam_error
sam_op_timesf(sam_execution_state *s)
{
    return sam_float_arithmetic(s, SAM_OP_TIMESF);
}

static sam_error
sam_op_divf(sam_execution_state *s)
{
    return sam_float_arithmetic(s, SAM_OP_DIVF);
}

static sam_error
sam_op_lshift(sam_execution_state *s)
{
    return sam_bitshift(s, TRUE);
}

static sam_error
sam_op_lshiftind(sam_execution_state *s)
{
    return sam_bitshiftind(s, TRUE);
}

static sam_error
sam_op_rshift(sam_execution_state *s)
{
    return sam_bitshift(s, FALSE);
}

static sam_error
sam_op_rshiftind(sam_execution_state *s)
{
    return sam_bitshiftind(s, FALSE);
}

static sam_error
sam_op_and(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_AND);
}

static sam_error
sam_op_or(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_OR);
}

static sam_error
sam_op_nand(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_NAND);
}

static sam_error
sam_op_nor(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_NOR);
}

static sam_error
sam_op_xor(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_XOR);
}

static sam_error
sam_op_not(sam_execution_state *s)
{   
    return sam_unary_arithmetic(s, SAM_OP_NOT);
}

static sam_error
sam_op_bitand(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_BITAND);
}

static sam_error
sam_op_bitor(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_BITOR);
}

static sam_error
sam_op_bitnand(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_BITNAND);
}

static sam_error
sam_op_bitnor(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_BITNOR);
}

static sam_error
sam_op_bitxor(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_BITXOR);
}

static sam_error
sam_op_bitnot(sam_execution_state *s)
{
    return sam_unary_arithmetic(s, SAM_OP_BITNOT);
}

static sam_error
sam_op_cmp(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_CMP);
}

static sam_error
sam_op_cmpf(sam_execution_state *s)
{
    return sam_float_arithmetic(s, SAM_OP_CMPF);
}

static sam_error
sam_op_greater(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_GREATER);
}

static sam_error
sam_op_less(sam_execution_state *s)
{
    return sam_integer_arithmetic(s, SAM_OP_LESS);
}

static sam_error
sam_op_equal(sam_execution_state *s)
{
    sam_memory_location *m1, *m2;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if ((m1 = sam_pop(s)) == NULL) {
	free(m2);
	return sam_error_stack_underflow();
    }
    switch (m1->type) {
	case TYPE_FLOAT:
	    if (m2->type == TYPE_FLOAT) {
		m1->value.i = sam_float_equal(m1->value.f, m2->value.f);
	    } else {
		m1->value.i = FALSE;
	    }
	    break;
	case TYPE_INT:
	case TYPE_CHAR:
	    if (m2->type == TYPE_INT || m2->type == TYPE_CHAR) {
		m1->value.i = m1->value.i == m2->value.i;
	    } else {
		m1->value.i = FALSE;
	    }
	    break;
	case TYPE_PA:
	    if (m2->type == TYPE_PA) {
		m1->value.i = m1->value.pa == m2->value.pa;
	    } else {
		m1->value.i = FALSE;
	    }
	    break;
	case TYPE_MA:
	    if(m2->type == TYPE_MA) {
		m1->value.i = m1->value.ma.stack == m2->value.ma.stack &&
		    m1->value.ma.index == m2->value.ma.index;
	    } else {
		m1->value.i = FALSE;
	    }
	    break;
	case TYPE_NONE:
	default:
	    m1->value.i = FALSE;
	    break;
    }

    m1->type = TYPE_INT;
    free(m2);
    sam_push(s, m1);

    return SAM_OK;
}

static sam_error
sam_op_isnil(sam_execution_state *s)
{
    return sam_unary_arithmetic(s, SAM_OP_ISNIL);
}

static sam_error
sam_op_ispos(sam_execution_state *s)
{
    return sam_unary_arithmetic(s, SAM_OP_ISPOS);
}

static sam_error
sam_op_isneg(sam_execution_state *s)
{
    return sam_unary_arithmetic(s, SAM_OP_ISNEG);
}

static sam_error
sam_op_jump(sam_execution_state *s)
{
    sam_program_address	target;
    sam_error		err;

    if ((err = sam_get_jump_target(s, &target)) != SAM_OK) {
	return err;
    }
    s->pc = target;

    return SAM_OK;
}

static sam_error
sam_op_jumpc(sam_execution_state *s)
{
    sam_program_address target;
    sam_error		err;
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_INT && m->type != TYPE_CHAR) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    if (m->value.i == 0) {
	free(m);
	return SAM_OK;
    }
    if ((err = sam_get_jump_target(s, &target)) != SAM_OK) {
	free(m);
	return err;
    }

    s->pc = target;
    free(m);
    return SAM_OK;
}

static sam_error
sam_op_jumpind(sam_execution_state *s)
{
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_PA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_PA);
    }
    s->pc = m->value.pa - 1;

    free(m);
    return SAM_OK;
}

static sam_error
sam_op_rst(sam_execution_state *s)
{
    return sam_op_jumpind(s);
}

static sam_error
sam_op_jsr(sam_execution_state *s)
{
    sam_value		v;
    sam_program_address	target;
    sam_error		err;

    v.pa = s->pc + 1;
    sam_push(s, sam_memory_location_new(v, TYPE_PA));

    if ((err = sam_get_jump_target(s, &target)) != SAM_OK) {
	return err;
    }
    s->pc = target;

    return SAM_OK;
}

static sam_error
sam_op_jsrind(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_value		 v;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_PA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_PA);
    }

    v.pa = s->pc + 1;
    sam_push(s, sam_memory_location_new(v, TYPE_PA));
    s->pc = m->value.pa - 1;

    return SAM_OK;
}

static sam_error
sam_op_skip(sam_execution_state *s)
{
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    s->pc += m->value.pa;

    free(m);
    return SAM_ENOSYS;
}

static sam_error
sam_op_link(sam_execution_state *s)
{
    sam_value v;

    v.ma.stack = (unsigned)TRUE;
    v.ma.index = s->fbr;
    sam_push(s, sam_memory_location_new(v, TYPE_MA));
    s->fbr = s->stack.len - 1;

    return SAM_OK;
}

static sam_error
sam_op_unlink(sam_execution_state *s)
{
    return sam_op_popfbr(s);
}

static sam_error
sam_op_read(sam_execution_state *s)
{
    sam_value v;
    int	      err;
    int	      buf;

    if (s->io_funcs->read_int_func == NULL) {
	return sam_read_number(s, TYPE_INT);
    }
    if ((err = s->io_funcs->read_int_func(&buf)) < 0) {
	return SAM_EIO;
    }
    v.i = buf;
    sam_push(s, sam_memory_location_new(v, TYPE_INT));

    return SAM_OK;
}

static sam_error
sam_op_readf(sam_execution_state *s)
{
    sam_value v;
    int	      err;
    double    buf;

    if (s->io_funcs->read_float_func == NULL) {
	return sam_read_number(s, TYPE_FLOAT);
    }
    if ((err = s->io_funcs->read_float_func(&buf)) < 0) {
	return SAM_EIO;
    }
    v.f = (sam_float)buf;
    sam_push(s, sam_memory_location_new(v, TYPE_FLOAT));

    return SAM_OK;
}

static sam_error
sam_op_readch(sam_execution_state *s)
{
    sam_value v;
    char      buf;

    if (s->io_funcs->read_char_func == NULL) {
	errno = 0;
	v.c = getchar();

	if (v.c == EOF && ferror(stdin) != 0) {
	    clearerr(stdin);
	    return sam_error_io();
	}
    } else if (s->io_funcs->read_char_func(&buf) < 0) {
	return SAM_EIO;
    }

    v.c = buf;
    sam_push(s, sam_memory_location_new(v, TYPE_CHAR));
    return SAM_OK;
}

static sam_error
sam_op_readstr(sam_execution_state *s)
{
    sam_memory_location *m;
    char		*str;
    sam_value		 v;
    size_t		 len, i;

    if (s->io_funcs->read_str_func == NULL) {
	sam_string string;

	if (sam_string_read(stdin, &string) == NULL) {
	    sam_string_free(&string);
	    return sam_error_io();
	}
	str = string.data;
	len = string.len;
    } else {
	if (s->io_funcs->read_str_func(&str) < 0) {
	    return SAM_EIO;
	}
	len = strlen(str);
    }

    v.ma.stack = (unsigned)FALSE;
    v.ma.index = sam_heap_alloc(s, len + 1);
    for (i = 0; i <= len; ++i) {
	m = s->heap.a.arr[i + v.ma.index];
	m->value.c = str[i];
	m->type = TYPE_CHAR;
    }
    free(s);
    sam_push(s, sam_memory_location_new(v, TYPE_MA));

    return SAM_OK;
}

static sam_error
sam_op_write(sam_execution_state *s)
{
    sam_memory_location *m;
    int			 i;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_CHAR && m->type != TYPE_INT) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_INT);
    }
    i = m->value.i;
    free(m);

    if (s->io_funcs->write_int_func == NULL) {
	printf("%d", i);
	return errno == 0? SAM_OK: sam_error_io();
    }
    return s->io_funcs->write_int_func(i) == 0?  SAM_OK: sam_error_io();
}

static sam_error
sam_op_writef(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_float		 f;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_FLOAT) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_FLOAT);
    }
    f = m->value.f;
    free(m);

    if (s->io_funcs->write_float_func == NULL) {
	printf("%f", f);
	return errno == 0? SAM_OK: sam_error_io();
    }
    return s->io_funcs->write_float_func(f) == 0? SAM_OK: sam_error_io();
}

static sam_error
sam_op_writech(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_char		 c;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_CHAR && m->type != TYPE_INT) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_CHAR);
    }
    c = m->value.c;
    free(m);

    if (s->io_funcs->write_char_func == NULL) {
	return sam_putchar(c);
    }
    return s->io_funcs->write_char_func(c) == 0? SAM_OK: sam_error_io();
}

static sam_error
sam_op_writestr(sam_execution_state *s)
{
    sam_memory_location *m;
    sam_memory_address   ma;
    size_t		 i = 0, len = 0;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_MA) {
	sam_type t = m->type;
	free(m);
	return sam_error_stack_input1(s, t, TYPE_MA);
    }
    ma = m->value.ma;
    free(m);

    if (ma.index >= s->heap.a.len) {
	return sam_error_segmentation_fault(ma);
    }

    if (s->io_funcs->write_str_func == NULL) {
	for (;;) {
	    int c;
	    m = s->heap.a.arr[i];

	    if (m == NULL || m->value.c == '\0') {
		break;
	    }
	    if ((c = sam_putchar(m->value.c)) != SAM_OK) {
		return c;
	    }
	    if (++i == s->heap.a.len) {
		ma.index = i;
		return sam_error_segmentation_fault(ma);
	    }
	}

	/* XXX only until string parsing is fixed */
	return sam_putchar('\n');
    }
    for (;;) {
	m = s->heap.a.arr[len];
	if (m == NULL || m->value.c == '\0') {
	    break;
	}
	if (++len == s->heap.a.len) {
	    ma.index = len;
	    return sam_error_segmentation_fault(ma);
	}
    }
    {
	sam_error  rv;
	char	  *str = sam_malloc(sizeof (len + 1));

	for (i = 0; i < len; ++i) {
	    m = s->heap.a.arr[i];
	    if (m == NULL || m->value.c == '\0') {
		str[i] = '\0';
		break;
	    }
	    str[i] = m->value.c;
	}

	rv = s->io_funcs->write_str_func(str) == 0? SAM_OK: sam_error_io();
	free(str);
	return rv;
    }
}

static sam_error
sam_op_stop(sam_execution_state *s)
{
    if (s->stack.len > 1) {
	return sam_error_final_stack_state();
    }

    return SAM_STOP;
}

#if defined(SAM_EXTENSIONS)
# if defined(HAVE_DLFCN_H)

static sam_error
sam_op_import(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_LABEL) {
	return sam_error_optype(s);
    }
    if (sam_dllookup(s, cur->operand.s)) {
	return sam_dlopen(s, cur->operand.s);
    }

    return SAM_OK;
}

static sam_error
sam_op_call(sam_execution_state *s)
{
    sam_instruction *cur = s->program->arr[s->pc];

    if (cur->optype != TYPE_LABEL) {
	return sam_error_optype(s);
    }
    if (sam_dlsym(s) == NULL) {
	return sam_error_dlsym(s);
    }
    return SAM_OK;
}

# else /* HAVE_DLFCN_H */

static sam_error
sam_op_import(UNUSED sam_execution_state *s)
{
    fputs("error: samiam was not compiled with support for the import "
	  "instruction.\n", stderr);
    return SAM_ENOSYS;
}

static sam_error
sam_op_call(UNUSED sam_execution_state *s)
{
    fputs("error: samiam was not compiled with support for the open "
	  "instruction.\n", stderr);
    return SAM_ENOSYS;
}

# endif /* HAVE_DLFCN_H */

static sam_error
sam_op_patoi(sam_execution_state *s)
{
    sam_memory_location *m;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != TYPE_PA) {
	sam_type t = m->type;
	free(m);
	return sam_error_type_conversion(TYPE_INT, t, TYPE_PA);
    }
    m->value.i = (int)m->value.pa;
    m->type = TYPE_INT;
    sam_push(s, m);

    return SAM_OK;
}

#endif /* SAM_EXTENSIONS */

const sam_instruction sam_instructions[] = {
    { "FTOI",		TYPE_NONE,  {0}, sam_op_ftoi	   },
    { "FTOIR",		TYPE_NONE,  {0}, sam_op_ftoir	   },
    { "ITOF",		TYPE_NONE,  {0}, sam_op_itof	   },
    { "PUSHIMM",	TYPE_INT,   {0}, sam_op_pushimm	   },
    { "PUSHIMMF",	TYPE_FLOAT, {0}, sam_op_pushimmf   },
    { "PUSHIMMCH",	TYPE_CHAR,  {0}, sam_op_pushimmch  },
    { "PUSHIMMMA",	TYPE_INT,   {0}, sam_op_pushimmma  },
    { "PUSHIMMPA",	TYPE_LABEL |
	TYPE_INT,   {0}, sam_op_pushimmpa  },
    { "PUSHIMMSTR",	TYPE_STR,   {0}, sam_op_pushimmstr },
    { "PUSHSP",		TYPE_NONE,  {0}, sam_op_pushsp	   },
    { "PUSHFBR",	TYPE_NONE,  {0}, sam_op_pushfbr	   },
    { "POPSP",		TYPE_NONE,  {0}, sam_op_popsp	   },
    { "POPFBR",		TYPE_NONE,  {0}, sam_op_popfbr	   },
    { "DUP",		TYPE_NONE,  {0}, sam_op_dup	   },
    { "SWAP",		TYPE_NONE,  {0}, sam_op_swap	   },
    { "ADDSP",		TYPE_INT,   {0}, sam_op_addsp	   },
    { "MALLOC",		TYPE_NONE,  {0}, sam_op_malloc	   },
    { "FREE",		TYPE_NONE,  {0}, sam_op_free	   },
    { "PUSHIND",	TYPE_NONE,  {0}, sam_op_pushind	   },
    { "STOREIND",	TYPE_NONE,  {0}, sam_op_storeind   },
    { "PUSHABS",	TYPE_INT,   {0}, sam_op_pushabs	   },
    { "STOREABS",	TYPE_INT,   {0}, sam_op_storeabs   },
    { "PUSHOFF",	TYPE_INT,   {0}, sam_op_pushoff	   },
    { "STOREOFF",	TYPE_INT,   {0}, sam_op_storeoff   },
    { "ADD",		TYPE_NONE,  {0}, sam_op_add	   },
    { "SUB",		TYPE_NONE,  {0}, sam_op_sub	   },
    { "TIMES",		TYPE_NONE,  {0}, sam_op_times	   },
    { "DIV",		TYPE_NONE,  {0}, sam_op_div	   },
    { "MOD",		TYPE_NONE,  {0}, sam_op_mod	   },
    { "ADDF",		TYPE_NONE,  {0}, sam_op_addf	   },
    { "SUBF",		TYPE_NONE,  {0}, sam_op_subf	   },
    { "TIMESF",		TYPE_NONE,  {0}, sam_op_timesf	   },
    { "DIVF",		TYPE_NONE,  {0}, sam_op_divf	   },
    { "LSHIFT",		TYPE_INT,   {0}, sam_op_lshift	   },
    { "LSHIFTIND",	TYPE_NONE,  {0}, sam_op_lshiftind  },
    { "RSHIFT",		TYPE_INT,   {0}, sam_op_rshift	   },
    { "RSHIFTIND",	TYPE_NONE,  {0}, sam_op_rshiftind  },
    { "AND",		TYPE_NONE,  {0}, sam_op_and	   },
    { "OR",		TYPE_NONE,  {0}, sam_op_or	   },
    { "NAND",		TYPE_NONE,  {0}, sam_op_nand	   },
    { "NOR",		TYPE_NONE,  {0}, sam_op_nor	   },
    { "XOR",		TYPE_NONE,  {0}, sam_op_xor	   },
    { "NOT",		TYPE_NONE,  {0}, sam_op_not	   },
    { "BITAND",		TYPE_NONE,  {0}, sam_op_bitand	   },
    { "BITOR",		TYPE_NONE,  {0}, sam_op_bitor	   },
    { "BITNAND",	TYPE_NONE,  {0}, sam_op_bitnand	   },
    { "BITNOR",		TYPE_NONE,  {0}, sam_op_bitnor	   },
    { "BITXOR",		TYPE_NONE,  {0}, sam_op_bitxor	   },
    { "BITNOT",		TYPE_NONE,  {0}, sam_op_bitnot	   },
    { "CMP",		TYPE_NONE,  {0}, sam_op_cmp	   },
    { "CMPF",		TYPE_NONE,  {0}, sam_op_cmpf	   },
    { "GREATER",	TYPE_NONE,  {0}, sam_op_greater	   },
    { "LESS",		TYPE_NONE,  {0}, sam_op_less	   },
    { "EQUAL",		TYPE_NONE,  {0}, sam_op_equal	   },
    { "ISNIL",		TYPE_NONE,  {0}, sam_op_isnil	   },
    { "ISPOS",		TYPE_NONE,  {0}, sam_op_ispos	   },
    { "ISNEG",		TYPE_NONE,  {0}, sam_op_isneg	   },
    { "JUMP",		TYPE_LABEL |
	TYPE_INT,   {0}, sam_op_jump	   },
    { "JUMPC",		TYPE_LABEL |
	TYPE_INT,   {0}, sam_op_jumpc	   },
    { "JUMPIND",	TYPE_NONE,  {0}, sam_op_jumpind	   },
    { "RST",		TYPE_NONE,  {0}, sam_op_rst	   },
    { "JSR",		TYPE_LABEL |
	TYPE_INT,   {0}, sam_op_jsr	   },
    { "JSRIND",		TYPE_NONE,  {0}, sam_op_jsrind	   },
    { "SKIP",		TYPE_NONE,  {0}, sam_op_skip	   },
    { "LINK",		TYPE_NONE,  {0}, sam_op_link	   },
    { "UNLINK",		TYPE_NONE,  {0}, sam_op_unlink	   },
    { "READ",		TYPE_NONE,  {0}, sam_op_read	   },
    { "READF",		TYPE_NONE,  {0}, sam_op_readf	   },
    { "READCH",		TYPE_NONE,  {0}, sam_op_readch	   },
    { "READSTR",	TYPE_NONE,  {0}, sam_op_readstr	   },
    { "WRITE",		TYPE_NONE,  {0}, sam_op_write	   },
    { "WRITEF",		TYPE_NONE,  {0}, sam_op_writef	   },
    { "WRITECH",	TYPE_NONE,  {0}, sam_op_writech	   },
    { "WRITESTR",	TYPE_NONE,  {0}, sam_op_writestr   },
    { "STOP",		TYPE_NONE,  {0}, sam_op_stop	   },
#if defined(SAM_EXTENSIONS)
    { "import",		TYPE_LABEL, {0}, sam_op_import	   },
    { "call",		TYPE_LABEL, {0}, sam_op_call	   },
    { "patoi",		TYPE_NONE,  {0}, sam_op_patoi	   },
#endif
    { "",		TYPE_NONE,  {0}, NULL		   },
};

static void
sam_heap_init(sam_heap *heap)
{
    heap->free_list = NULL;
    heap->used_list = NULL;
    sam_array_init(&heap->a);
}

static void
sam_execution_state_init(/*@out@*/ sam_execution_state *s)
{
    s->pc = 0;
    s->fbr = 0;
    sam_array_init(&s->stack);
    sam_heap_init(&s->heap);
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_init(&s->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
}

static void
sam_execution_state_free(sam_execution_state *s)
{
    sam_array_free(s->program);
    sam_array_free(s->labels);
    sam_array_free(&s->stack);
    sam_heap_free(&s->heap);
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    sam_array_free(&s->dlhandles);
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
}

/* Die... with style! */
static void
sam_stack_trace(const sam_execution_state *s)
{
    size_t i;

    fprintf(stderr,
	    "\nstate of execution:\n"
	    "PC:\t%u\n"
	    "FBR:\t%u\n"
	    "SP:\t%u\n\n",
	    (unsigned)s->pc, (unsigned)s->fbr, (unsigned)s->stack.len);

    fputs("Heap\t    Stack\t    Program\n", stderr);
    for (i = 0; i <= s->program->len || i <= s->stack.len ||
	 i < s->heap.a.len; ++i) {
	if (i < s->heap.a.len) {
	    sam_memory_location *m = s->heap.a.arr[i];
	    if (m == NULL) {
		fputs ("NULL", stderr);
	    } else {
		fprintf(stderr, "%c: ", sam_type_to_char(m->type));
		sam_print_value(m->value, m->type);
	    }
	    fputs("\t", stderr);
	} else {
	    fputs("\t", stderr);
	}
	if (i < s->stack.len) {
	    sam_memory_location *m = s->stack.arr[i];
	    if (i == s->fbr) {
		fputs("==> ", stderr);
	    } else {
		fputs("    ", stderr);
	    }
	    fprintf(stderr, "%c: ", sam_type_to_char(m->type));
	    sam_print_value(m->value, m->type);
	    fputs("\t", stderr);
	} else {
	    fputs("    \t\t", stderr);
	}
	if (i <= s->program->len) {
	    if (i == (size_t)s->pc) {
		fputs("==> ", stderr);
	    } else {
		fputs("    ", stderr);
	    }
	    if (i < s->program->len) {
		sam_instruction *inst = s->program->arr[i];
		fprintf(stderr, "%s ", inst->name);
		if (inst->optype != TYPE_NONE) {
		    sam_print_value(inst->operand, inst->optype);
		}
	    }
	}
	fputc('\n', stderr);
    }
    fputc('\n', stderr);
}

static sam_int
sam_convert_to_int(/*@in@*/ sam_memory_location *m)
{
    switch (m->type) {
	case TYPE_INT:
	case TYPE_CHAR:
	    return m->value.i;
	case TYPE_FLOAT:
	    return (sam_int)m->value.f;
	case TYPE_MA:
	    return (sam_int)m->value.ma.index;
	case TYPE_PA:
	    return (sam_int)m->value.pa;
	case TYPE_NONE:
	default:
	    return 0;
    }
}

sam_exit_code
sam_execute(/*@in@*/ sam_array	  *instructions,
	    /*@in@*/ sam_array	  *labels,
	    /*@in@*/ sam_io_funcs *io_funcs)
{
    int retval = SAM_EMPTY_STACK, err = 0;
    sam_execution_state s;

    sam_execution_state_init(&s);
    s.program = instructions;
    s.labels = labels;
    s.io_funcs = io_funcs;

    for (s.pc = 0; s.pc < s.program->len && err == 0; ++s.pc) {
	err = ((sam_instruction *)s.program->arr[s.pc])->handler(&s);
    }
    sam_check_for_leaks(&s);
    if (err == 0) {
	sam_error_forgot_stop();
    }
    if (s.stack.len > 0 && err >= 0) {
	sam_memory_location *m = s.stack.arr[0];
	if (m->type != TYPE_INT) {
	    sam_error_retval_type(&s);
	}
	retval = sam_convert_to_int(m);
    }
    if (stack_trace) {
	sam_stack_trace(&s);
    }

    sam_execution_state_free(&s);
    return retval;
}
