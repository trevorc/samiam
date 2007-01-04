/*
 * sam_parse.c      read and parse sam from stdin or a file
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
 * Revision 1.15  2007/01/04 06:09:25  trevor
 * New sam_es architecture.
 *
 * Revision 1.14  2006/12/27 21:18:03  trevor
 * Fix #02. Accept non-newline terminated files.
 *
 * Revision 1.13  2006/12/25 00:29:54  trevor
 * Update for new hash table labels.
 *
 * Revision 1.12  2006/12/22 04:45:25  trevor
 * 7-keystroke fix. (two labels can now point to the sam line)
 *
 * Revision 1.11  2006/12/19 18:17:53  trevor
 * Moved parts of sam_util.h which were library-generic into libsam.h.
 *
 * Revision 1.10  2006/12/19 09:50:02  trevor
 * fixed a cast
 *
 * Revision 1.9  2006/12/19 09:29:34  trevor
 * Removed implicit casts.
 *
 * Revision 1.8  2006/12/19 07:28:09  anyoneeb
 * Split sam_value into sam_op_value and sam_ml_value.
 *
 * Revision 1.7  2006/12/19 05:43:40  anyoneeb
 * Added self to copyright.
 *
 * Revision 1.6  2006/12/19 05:41:15  anyoneeb
 * Sepated operand types from memory types.
 *
 * Revision 1.5  2006/12/17 00:44:15  trevor
 * Remove dynamic loading #includes. Rename sam_program_address to sam_pa.
 *
 * Revision 1.4  2006/12/12 23:31:36  trevor
 * Added the Id and Log tags and copyright notice where they were missing.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libsam/array.h>
#include <libsam/es.h>
#include <libsam/hash_table.h>
#include <libsam/string.h>
#include <libsam/opcode.h>
#include <libsam/main.h>

#if defined(HAVE_MMAN_H)
# include <sys/stat.h>
# include <sys/mman.h>
# include <fcntl.h>
#endif /* HAVE_MMAN_H */

#if defined(HAVE_UNISTD_H) || defined(HAVE_MMAN_H)
# include <unistd.h>
#endif /* HAVE_UNISTD_H || HAVE_MMAN_H */

#include "sam_parse.h"
#include "sam_main.h"

#if defined(isspace)
# undef isspace
#endif
#define isspace(c) ((c) == ' ' || (c) == '\n' || (c) == '\t')

static bool mmapped = false;

static void
sam_eat_whitespace(char *restrict *restrict s)
{
    if (*s == NULL || **s == '\0') {
	return;
    }
    for (;;) {
	if (isspace(**s)) {
	    *(*s)++ = '\0';
	} else if ((*s)[0] == '/' && (*s)[1] == '/') {
	    while(**s != '\0' && **s != '\n') {
		*(*s)++ = '\0';
	    }
	} else {
	    break;
	}
    } 
}

static void
sam_truncate(char *s)
{
    static const size_t TRUNC_HEAD = 12, TRUNC_TAIL = 5;
    size_t len = strlen(s);
    char *tail = s + len - 1;

    for (; isspace(*tail); --len) {
	*tail-- = '\0';
    }
    for (char *t = s; *t != '\0'; ++t) {
	if (*t == '\n') {
	    *t = ' ';
	}
    }

    if (len > TRUNC_HEAD + 3 + TRUNC_TAIL) {
	sprintf(s + TRUNC_HEAD, "...%s", s + len - TRUNC_TAIL);
    }
}

/** The sourcefile provided contains no instructions or labels. */
static inline void
sam_error_empty_input(const sam_es *restrict es)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es, SAM_IOS_ERR, "error: empty source file.\n");
    }
}

/** There was a problem parsing an identifier. */
static inline void
sam_error_identifier(const sam_es *restrict es,
		     char *s)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_truncate(s);
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: couldn't parse identifier: %s.\n",
		       s);
    }
}

/** An unknown opcode was encountered. */
static inline void
sam_error_opcode(const sam_es *restrict es,
		 char *s)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_truncate(s);
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: unknown opcode found: %s.\n",
		       s);
    }
}

/** There was a problem parsing an operand. */
static inline void
sam_error_operand(const sam_es *restrict es,
		  const char *restrict opcode,
		  char *operand)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_truncate(operand);

	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: couldn't parse operand for %s: %s.\n",
		       opcode,
		       operand);
    }
}

/** The same label was found twice. */
static inline void
sam_error_duplicate_label(const sam_es *restrict es,
			  const char *restrict label,
			  sam_pa pa)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       "error: duplicate label \"%s\" was found at program "
		       "address %lu.\n",
		       label,
		       (unsigned long)pa);
    }
}

static bool
sam_try_parse_identifier(char **restrict input,
			 /*@out@*/ char **restrict identifier,
			 /*@null@*/ sam_op_type *restrict optype)
{
    char *start = *input;

    if (!isalpha(*start)) {
	*identifier = NULL;
	return false;
    }
    for (;;) {
	if (!isalnum(*start) && *start != '_') {
	    *identifier = *input;
	    *input = start;
	    if (optype != NULL) {
		*optype = SAM_OP_TYPE_LABEL;
	    }
	    return true;
	}
	++start;
    }
}

static bool
sam_try_parse_string(/*@in@*/  char **restrict input,
		     /*@out@*/ char **restrict string,
		     /*@null@*/ sam_op_type *optype)
{
    char *start = *input + 1;

    if(**input != '"') {
	*string = NULL;
	return false;
    }

    for (;;) {
	if (*start == '\0') {
	    *string = NULL;
	    return false;
	}
	if (*start == '"') {
	    *start++ = '\0';
	    *(*input)++ = '\0';
	    *string = *input;
	    *input = start;
	    if (optype != NULL) {
		if ((*optype & SAM_OP_TYPE_LABEL) != 0) {
		    *optype = SAM_OP_TYPE_LABEL;
		} else {
		    *optype = SAM_OP_TYPE_STR;
		}
	    }
	    return true;
	}
	++start;
    }
}

static inline bool
sam_try_parse_number(/*@in@*/ char **restrict input,
		     sam_op_value *restrict operand,
		     sam_op_type *restrict optype)
{
    char *endptr;

    if ((*optype & SAM_OP_TYPE_INT) != 0) {
	operand->i = strtol(*input, &endptr, 0);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0')) {
	    *optype = SAM_OP_TYPE_INT;
	    *input = endptr;
	    return true;
	}
    }
    if ((*optype & SAM_OP_TYPE_FLOAT) != 0) {
	operand->f = strtod(*input, &endptr);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0')) {
	    *optype = SAM_OP_TYPE_FLOAT;
	    *input = endptr;
	    return true;
	}
    }

    return false;
}

static inline bool
sam_try_parse_escape_sequence(char **restrict input,
			      int   *restrict c)
{
    char *start = *input;
    char *prev;
    int	  base = 0;
    long  n;

    switch (*(prev = start++)) {
	case '"':  /*@fallthrough@*/
	case '\'': /*@fallthrough@*/
	case '\\':
	    *c = *prev;
	    break;
	case 'a':
	    *c = '\a';
	    break;
	case 'b':
	    *c = '\b';
	    break;
	case 'f':
	    *c = '\f';
	    break;
	case 'n':
	    *c = '\n';
	    break;
	case 'r':
	    *c = '\r';
	    break;
	case 't':
	    *c = '\t';
	    break;
	case 'v':
	    *c = '\v';
	    break;
	case '\0':
	    return false;
	case 'x':
	    base = 16;
	    ++prev;
	    /*@fallthrough@*/
	case '0': /*@fallthrough@*/
	case '1': /*@fallthrough@*/
	case '2': /*@fallthrough@*/
	case '3': /*@fallthrough@*/
	case '4': /*@fallthrough@*/
	case '5': /*@fallthrough@*/
	case '6': /*@fallthrough@*/
	case '7': /*@fallthrough@*/
	case '8': /*@fallthrough@*/
	case '9':
	    n = strtol(prev, &start, base);
	    *c = n;
	    break;
	default:
	    return false;
    }
    *input = start++;

    return true;
}

static inline bool
sam_try_parse_char(/*@in@*/ char **restrict input,
		   int		  *restrict c,
		   sam_op_type	  *restrict optype)
{
    char *start = *input;

    if (*start++ != '\'') {
	return false;
    }
    if (*start == '\\') {
	++start;
	if (!sam_try_parse_escape_sequence(&start, c)) {
	    return false;
	}
    } else if (*start != '\0') {
	*c = *start++;
	if (*c == '\0') {
	    return false;
	}
    } else {
	return false;
    }
    if (*start++ != '\'') {
	return false;
    }
    *input = start;
    *optype = SAM_OP_TYPE_CHAR;
    return true;
}

/**
 *  Advance past the operand of a certain type if we can find one, and set
 *  the type to what we find.
 *
 *  @param input A pointer to the string which should be tested for
 *		 containing an operand. Update to the point after the
 *		 operand if it's found, otherwise print an error.
 *  @param operand A pointer to a #sam_op_value where the result of the
 *		   parse should be stored if successful.
 *  @param optype A pointer to the union of the possible types the
 *		  operand could be; updated to what is found.
 *
 *  @return true if the parse was successful, false otherwise.
 */
static inline bool
sam_try_parse_operand(/*@in@*/ char	    **restrict input,
		      /*@in@*/ sam_op_value  *restrict operand,
		      sam_op_type	     *restrict optype)
{
    return
	((*optype & (SAM_OP_TYPE_INT | SAM_OP_TYPE_FLOAT)) != 0 &&
	 sam_try_parse_number(input, operand, optype)) ||
	((*optype & SAM_OP_TYPE_CHAR) != 0 &&
	 sam_try_parse_char(input, &operand->c, optype)) ||
	((*optype & SAM_OP_TYPE_STR) != 0 &&
	 (sam_try_parse_string(input, &operand->s, optype))) ||
	((*optype & SAM_OP_TYPE_LABEL) != 0 &&
	 (sam_try_parse_string(input, &operand->s, optype) ||
	  sam_try_parse_identifier(input, &operand->s, optype)));
}

/*@null@*/ static inline sam_instruction *
sam_parse_instruction(const sam_es *restrict es,
		      char **restrict input)
{
    char *opcode, *start = *input;

    if (!sam_try_parse_identifier(&start, &opcode, NULL)) {
	sam_error_identifier(es, start);
	return NULL;
    }
    sam_eat_whitespace(&start);

    sam_instruction *restrict i = sam_opcode_get(opcode);
    if (i == NULL) {
	sam_error_opcode(es, opcode);
	return NULL;
    }
    if (i->optype != SAM_OP_TYPE_NONE) {
	if (!sam_try_parse_operand(&start, &i->operand, &i->optype)) {
	    sam_error_operand(es, i->name, start);
	    free(i);
	    return NULL;
	}
    }

    sam_eat_whitespace(&start);
    *input = start;
    return i;
}

static inline bool
sam_check_for_colon(char **restrict s)
{
    char *start = *s;

    if (start == NULL || *start == '\0') {
	return false;
    }
    for (;;) {
	if (*start == ':') {
	    *s = start;
	    return true;
	} else if (isspace(*start)) {
	    ++start;
	} else {
	    return false;
	}
    }
}

/*@null@*/ static inline char *
sam_parse_label(char **restrict input)
{
    char *label, *start = *input, *lookahead;

    if (*start == '"') {
	if (!sam_try_parse_string(&start, &label, NULL)) {
	    return NULL;
	} 
    } else if (!sam_try_parse_identifier(&start, &label, NULL)) {
	return NULL;
    }

    lookahead = start;
    if (!sam_check_for_colon(&lookahead)) {
	return NULL;
    }
    sam_eat_whitespace(&start);
    *start++ = '\0';
    *input = start;

    return label;
}

#if defined(HAVE_MMAN_H)

/*@null@*/ static inline char *
sam_input_read(/*@observer@*/ const char *restrict path,
	       /*@out@*/ sam_string *restrict s)
{
    struct stat sb;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
	perror("open");
	return NULL;
    }
    if (fstat(fd, &sb) < 0) {
	perror("fstat");
	if (close(fd) < 0) {
	    perror("close");
	}
	return NULL;
    }
    if (!S_ISREG(sb.st_mode)) {
	fprintf(stderr, "%s is not a regular file\n", path);
	if (close(fd) < 0) {
	    perror("close");
	}
	return NULL;
    }
    s->len = sb.st_size + 1;
    s->data = mmap(0, s->len, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (s->data == (void *)-1) {
	perror("mmap");
	if (close(fd) < 0) {
	    perror("close");
	}
	return NULL;
    }
    mmapped = true;
    s->alloc = s->len;

    return s->data;
}

#else /* HAVE_MMAN_H */

/*@null@*/ static inline char *
sam_input_read(const char *restrict path,
	       sam_string *restrict s)
{
    FILE *restrict in = fopen(path, "r");
    if (in == NULL) {
	perror("fopen");
	return NULL;
    }

    char *restrict out = sam_string_read(in, s);
    if (out == NULL) {
	return NULL;
    }
    if (fclose(in) < 0) {
	perror("fclose");
    }

    return out;
}

#endif /* HAVE_MMAN_H */

static inline void
sam_input_free(sam_string *restrict s)
{
#ifdef HAVE_MMAN_H
    if (munmap(s->data, s->len) < 0) {
	perror("munmap");
    }
#else /* HAVE_MMAN_H */
    sam_string_free(s);
#endif /* HAVE_MMAN_H */
}

/*@null@*/ bool
sam_parse(sam_es *restrict es,
	  sam_string *restrict string,
	  sam_input_free_func *restrict free_func,
	  const char *restrict file)
{
    char *input;

    if (file == NULL) {
	input = sam_string_read(stdin, string);
	*free_func = sam_string_free;
    } else {
	input = sam_input_read(file, string);
	*free_func = sam_input_free;
    }

    if (input == NULL || *input == '\0') {
	sam_error_empty_input(es);
	return false;
    }

#if defined(SAM_EXTENSIONS)
    /* ignore a shebang line */
    if (input[0] == '#' && input[1] == '!') {
	while (*input != '\0' && *input != '\n') {
	    ++input;
	}
    }
#endif /* SAM_EXTENSIONS */

    /* Parse as many labels as we find, then parse an instruction. */
    sam_pa cur_line = 0;

    while (*input != '\0') {
	sam_eat_whitespace(&input);

	char *label;
	while ((label = sam_parse_label(&input))) {
	    if (!sam_es_labels_ins(es, label, cur_line)) {
		sam_error_duplicate_label(es, label, cur_line);
		return false;
	    }
	    sam_eat_whitespace(&input);
	}

	sam_instruction *restrict i = sam_parse_instruction(es, &input);
	if (i == NULL) {
	    return false;
	}
	sam_es_instructions_ins(es, i);
	++cur_line;
    }

    return true;
}
