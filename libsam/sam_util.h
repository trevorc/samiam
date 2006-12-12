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
 * Revision 1.3  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef SAM_UTIL_H
#define SAM_UTIL_H

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
 *  The various types allowed in sam operand positions or in sam memory
 *  locations.
 */
typedef enum {
    TYPE_NONE  = 1 << 0, /**< A null operand. */
    TYPE_INT   = 1 << 1, /**< An integer type. */
    TYPE_FLOAT = 1 << 2, /**< An IEEE 754 floating point number. */
    TYPE_CHAR  = 1 << 3, /**< A single ASCII character. */
    TYPE_LABEL = 1 << 4, /**< A double-quote delimited string or bare
			  *   label stored as a C string pointing to a
			  *   program address. */
    TYPE_STR   = 1 << 5, /**< A list of characters to be allocated on the
			  *   heap and represented as a memory address. */
    TYPE_MA    = 1 << 6, /**< A memory address pointing to a location on
			  *   the stack (nonnegative) or a location on
			  *   the heap (negative) */
    TYPE_PA    = 1 << 7	 /**< A program address pointing to an
			  *   instruction in the source file. */
} sam_type;

/** An integer, stored by the interpreter as a long to maximize
 * portability */
typedef long sam_int;

/** Make sure our chars are wide enough to be suitable return values. */
typedef int sam_char;

/** An index into the array of instructions. */
typedef size_t sam_program_address;

/** An index into the stack or the heap. */
typedef struct {
    unsigned stack:  1;	/** sam_bool#TRUE if this is a stack address,
			 *  sam_bool#FALSE if it's a heap address. */
    unsigned index: 15;	/** The index into the corresponding array. */
} sam_memory_address;

/** A value on the stack, heap or as an operand. */
typedef union {
    sam_int		 i;
    sam_float		 f;
    sam_char		 c;
    char		*s;
    sam_program_address	 pa;
    sam_memory_address	 ma;
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
#if defined(SAM_EXTENSIONS) && defined(HAVE_DLFCN_H)
    ,
    SAM_EDLOPEN,	/**< There was a problem interfacing with the
			 *   dynamic linking loader. */
    SAM_EDLSYM		/**< The given symbol could not be resolved. */
#endif /* SAM_EXTENSIONS && HAVE_DLFCN_H */
} sam_error;

/** An opcode handler function. This type of function is paired with
 *  each opcode and called when that kind of opcode is executed. */
typedef sam_error (*sam_handler)(struct _sam_execution_state *s);

/** An operation in sam. */
typedef struct _sam_instruction {
    /*@observer@*/ const char *name;	/**< The character string
					 *   representing this opcode. */
    sam_type    optype;			/**< The OR of types allowed
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
    sam_program_address pa;	/**< The index into the array of
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
