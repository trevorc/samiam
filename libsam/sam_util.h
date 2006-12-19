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
 * Revision 1.6  2006/12/19 05:41:15  anyoneeb
 * Sepated operand types from memory types.
 *
 * Revision 1.5  2006/12/19 03:19:19  trevor
 * fixed doc for TYPE_STR
 *
 * Revision 1.4  2006/12/17 00:15:42  trevor
 * Added two new types: TYPE_HA and TYPE_SA. Removed TYPE_MA. Shifted values
 * of types down. Removed dynamic loading defs.
 *
 * Revision 1.3  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef SAM_UTIL_H
#define SAM_UTIL_H

#include <limits.h>

/** Convenience type for code readability. */
typedef enum {
    FALSE,
    TRUE
} sam_bool;

/** Safe, dynamically allocating array type. */
typedef struct {
    size_t len;	    /**< Number of elements used in the array. */
    size_t alloc;   /**< Allocated size (in number of elements) of the
		     *	 array. */
    void **arr;	    /**< The array itself. */
} sam_array;

/** Safe, dynamically allocating string type. */
typedef struct {
    size_t len;	    /**< Number of bytes allocated for the string (not
		     **	 including the trailing NUL inserted by
		     **	 #sam_string_ins()) */
    size_t alloc;   /**< Number of bytes allocated for the string
		     *	 including for the trailing NUL */
    char *data;	    /**< The C string character array. */
} sam_string;

/**
 *  The various types allowed in sam operand positions.
 */
typedef enum {
    SAM_OP_TYPE_NONE,		/**< A null operand. */
    SAM_OP_TYPE_INT   = 1 << 0, /**< An integer type. */
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

/** Make sure our chars are wide enough to be suitable return values. */
typedef int sam_char;

/** An index into the array of instructions. */
typedef size_t sam_pa;

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
    sam_ha    ha;
    sam_sa    sa;
} sam_value;

struct _sam_instruction;
struct _sam_execution_state;

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
    SAM_ENOSYS		/**< This opcode is not supported on this
			 *   system because support for it was not
			 *   compiled in. */
} sam_error;

/** An opcode handler function. This type of function is paired with
 *  each opcode and called when that kind of opcode is executed. */
typedef sam_error (*sam_handler)(struct _sam_execution_state *s);

/** An operation in sam. */
typedef struct _sam_instruction {
    /*@observer@*/ const char *name;	/**< The character string
					 *   representing this opcode. */
    sam_op_type optype;			/**< The OR of types allowed
					 *   to be in the operand
					 *   position of this opcode. */
    sam_value	operand;		/**< The value of this
					 *   instruction's operand,
					 *   assigned on parsing. */
    /*@null@*/ /*@observer@*/
    sam_handler handler;		/**< A pointer to the function
					 *   called when this
					 *   instruction is executed. */
} sam_instruction;

/** A label in the sam source. */
typedef struct {
    /*@observer@*/ char *name;	/**< The pointer into the input to the
				 *   name of the label. */
    sam_pa pa;			/**< The index into the array of
				 *   instructions pointing to the
				 *   following instruction from the
				 *   source. */
} sam_label;

/** Exit codes for main() */
typedef enum {
    SAM_EMPTY_STACK = -1,   /**< The stack was empty after program
			     *	 execution. */
    SAM_PARSE_ERROR = -2,   /**< There was a problem parsing input. */
    SAM_USAGE	    = -3    /**< Usage was printed, there was a problem
			     *	 parsing the command line args. */
} sam_exit_code;

/*@out@*/ /*@only@*/ /*@notnull@*/ extern void *sam_malloc(size_t size);
extern void free(/*@only@*/ /*@out@*/ /*@null@*/ void *p);
/*@only@*/ /*@notnull@*/ extern void *sam_realloc(/*@only@*/ void *p, size_t size);
extern void sam_array_init(/*@out@*/ sam_array *a);
extern void sam_array_ins(/*@in@*/ sam_array *a, /*@only@*/ void *m);
/*@only@*/ /*@null@*/ extern void *sam_array_rem(sam_array *a);
extern void sam_array_free(sam_array *a);
extern void sam_string_init(/*@out@*/ sam_string *s);
extern void sam_string_ins(/*@in@*/ sam_string *s, char *src, size_t n);
extern void sam_string_free(sam_string *s);
/*@null@*/ extern char *sam_string_read(/*@in@*/ FILE *in, /*@out@*/ sam_string *s);

#endif /* SAM_UTIL_H */
