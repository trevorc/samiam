/*
 * $Id$
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2007 Trevor Caira, Jimmy Hartzell, Daniel Perelman
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

#ifndef LIBSAM_IO_H
#define LIBSAM_IO_H

#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "types.h"
#include "execute_types.h"

typedef enum {
    SAM_IOS_OUT,
    SAM_IOS_ERR,
    SAM_IOS_IN
} sam_io_stream;

typedef int   (*sam_io_vfprintf_func)   (sam_io_stream ios,
					 void *data,
					 const char *restrict fmt,
					 va_list ap);
typedef int   (*sam_io_vfscanf_func)    (sam_io_stream ios,
					 void *data,
					 const char *restrict fmt,
					 va_list ap);
typedef char *(*sam_io_afgets_func)	(char **restrict s,
					 sam_io_stream ios,
					 void *data);
typedef void  (*sam_io_bt_func)		(const sam_es *restrict es,
					 void *data);

/**
 *  Input/output callbacks. Used by various i/o opcodes.
 */
typedef union {
    /*@null@*/ /*@dependent@*/ sam_io_vfprintf_func vfprintf;
    /*@null@*/ /*@dependent@*/ sam_io_vfscanf_func  vfscanf;
    /*@null@*/ /*@dependent@*/ sam_io_afgets_func   afgets;
    /*@null@*/ /*@dependent@*/ sam_io_bt_func	    bt;
} sam_io_func;

typedef enum {
    SAM_IO_VFPRINTF,
    SAM_IO_VFSCANF,
    SAM_IO_AFGETS,
    SAM_IO_BT,
} sam_io_func_name;

typedef sam_io_func (*sam_io_dispatcher)(sam_io_func_name io_func);

extern FILE *sam_ios_to_file   (sam_io_stream ios);
extern void  sam_io_bt         (const sam_es *restrict es);
extern int   sam_io_printf     (const sam_es *restrict es,
				const char *restrict fmt,
				...)
__attribute__((format (printf, 2, 3)));
extern int   sam_io_fprintf    (const sam_es *restrict es,
				sam_io_stream ios,
				const char *restrict fmt,
				...)
__attribute__((format (printf, 3, 4)));
extern int   sam_io_scanf      (const sam_es *restrict es,
				const char *restrict fmt,
				...)
__attribute__((format (scanf, 2, 3)));
extern char *sam_io_afgets     (const sam_es *es,
				char **restrict s,
				sam_io_stream ios);

#endif /* LIBSAM_IO_H */
