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
 * Revision 1.6  2006/12/19 17:57:19  trevor
 * Documented sam_main. Moved sam_exit_code to sam.h.
 *
 * Revision 1.5  2006/12/19 10:41:26  anyoneeb
 * Last splint warnings fix for now.
 *
 * Revision 1.4  2006/12/12 23:31:35  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#ifndef SAM_H
#define SAM_H

/** Global program behavior defaults configurable by command line
 * options. */
typedef enum {
    quiet = 1 << 0	/**< Suppress verbose error messages. Library
			 *   errors are however not suppressed. */
} sam_options;

/** Exit codes for main() in case of error. */
typedef enum {
    SAM_EMPTY_STACK = -1,   /**< The stack was empty after program
			     *	 execution. */
    SAM_PARSE_ERROR = -2,   /**< There was a problem parsing input. */
    SAM_USAGE	    = -3    /**< Usage was printed, there was a problem
			     *	 parsing the command line args. */
} sam_exit_code;

typedef int (*sam_read_int_func)(/*@out@*/ int *n);
typedef int (*sam_read_float_func)(/*@out@*/ double *f);
typedef int (*sam_read_char_func)(/*@out@*/ char *c);
typedef int (*sam_read_str_func)(/*@out@*/ char **s);
typedef int (*sam_write_int_func)(int n);
typedef int (*sam_write_float_func)(double f);
typedef int (*sam_write_char_func)(char c);
typedef int (*sam_write_str_func)(char *s);

/**
 *  Input/output callbacks. Used by various i/o opcodes.
 */
typedef struct {
    /*@null@*/ /*@dependent@*/ sam_read_int_func    read_int_func;
    /*@null@*/ /*@dependent@*/ sam_read_float_func  read_float_func;
    /*@null@*/ /*@dependent@*/ sam_read_char_func   read_char_func;
    /*@null@*/ /*@dependent@*/ sam_read_str_func    read_str_func;
    /*@null@*/ /*@dependent@*/ sam_write_int_func   write_int_func;
    /*@null@*/ /*@dependent@*/ sam_write_float_func write_float_func;
    /*@null@*/ /*@dependent@*/ sam_write_char_func  write_char_func;
    /*@null@*/ /*@dependent@*/ sam_write_str_func   write_str_func;
} sam_io_funcs;

/**
 *  Entry point to libsam.
 *
 *  @param options The bitwise OR of the flags controlling libsam's
 *		   behavior.
 *  @param file	   The file containing the sam source to interpret, or
 *		   NULL if standard input should be read.
 *  @param io_funcs The set of input/output functions which should be
 *		    called when that particular form of input or output
 *		    is requested by the sam program. Any of the members
 *		    of the structure may be NULL if the default for that
 *		    function should be used, or the entire structure may
 *		    be NULL if all default functions should be used.
 *
 *  @return The return value of the sam value, or one of the special
 *	    #sam_exit_code in case a return value could not be extracted
 *	    from the source due to error.
 */
extern int sam_main(sam_options options,
		    /*@null@*/ const char   *file,
		    /*@null@*/ sam_io_funcs *io_funcs);

#endif /* SAM_H */
