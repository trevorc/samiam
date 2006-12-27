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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sam.h>
#include <libsam/config.h>
#include <libsam/types.h>
#include <libsam/util.h>

#if defined(HAVE_MMAN_H)
# include <sys/stat.h>
# include <sys/mman.h>
# include <fcntl.h>
#endif /* HAVE_MMAN_H */

#if defined(HAVE_UNISTD_H) || defined(HAVE_MMAN_H)
# include <unistd.h>
#endif /* HAVE_UNISTD_H || HAVE_MMAN_H */

#include "sam_execute.h"
#include "sam_parse.h"
#include "sam_main.h"

static sam_bool mmapped = FALSE;

static void
sam_eat_whitespace(char **s)
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

    while (isspace(*tail)) {
	*tail-- = '\0';
	--len;
    }

    if (len > TRUNC_HEAD + 3 + TRUNC_TAIL) {
	sprintf(s + TRUNC_HEAD, "...%s", s + len - TRUNC_TAIL);
    }
}

/** The sourcefile provided contains no instructions or labels. */
static void
sam_error_empty_input(void)
{
    if ((options & quiet) == 0) {
	fputs("error: empty source file.\n", stderr);
    }
}

/** There was a problem parsing an identifier. */
static void
sam_error_identifier(const char *s)
{
    if ((options & quiet) == 0) {
	fprintf(stderr, "error: couldn't parse identifier \"%s\"\n", s);
    }
}

/** An unknown opcode was encountered. */
static void
sam_error_opcode(const char *s)
{
    if ((options & quiet) == 0) {
	fprintf(stderr, "error: unknown opcode found: %s.\n", s);
    }
}

/** There was a problem parsing an operand. */
static void
sam_error_operand(const char *opcode,
		  char	     *operand)
{
    if ((options & quiet) == 0) {
	sam_truncate(operand);

	fprintf(stderr,
		"error: couldn't parse operand for %s: %s.\n",
		opcode, operand);
    }
}

/** The same label was found twice. */
static void
sam_error_duplicate_label(const char *label,
			  sam_pa      pa)
{
    if ((options & quiet) == 0) {
	fprintf(stderr,
		"error: duplicate label \"%s\" was found at program "
		"address %lu.\n",
		label,
		(unsigned long)pa);
    }
}

/*@null@*/ static sam_instruction *
sam_instruction_new(/*@in@*/ /*@dependent@*/ const char *name)
{
    sam_instruction *i;
    int j;

    for (j = 0; sam_instructions[j].handler != NULL; ++j) {
	if (strcmp(name, sam_instructions[j].name) == 0) {
	    i = sam_malloc(sizeof (sam_instruction));
	    i->name = name;
	    i->optype = sam_instructions[j].optype;
	    i->handler = sam_instructions[j].handler;
	    i->operand.i = 0;
	    return i;
	}
    }

    return NULL;
}

static sam_bool
sam_try_parse_identifier(char **input,
			 /*@out@*/ char **identifier,
			 /*@null@*/ sam_op_type *optype)
{
    char *start = *input;

    if (!isalpha(*start)) {
	*identifier = NULL;
	return FALSE;
    }
    for (;;) {
	if (!isalnum(*start) && *start != '_') {
	    *identifier = *input;
	    *input = start;
	    if (optype != NULL) {
		*optype = SAM_OP_TYPE_LABEL;
	    }
	    return TRUE;
	}
	++start;
    }
}

static sam_bool
sam_try_parse_string(/*@in@*/  char **input,
		     /*@out@*/ char **string,
		     /*@null@*/ sam_op_type *optype)
{
    char *start = *input + 1;

    if(**input != '"') {
	*string = NULL;
	return FALSE;
    }

    for (;;) {
	if (*start == '\0') {
	    *string = NULL;
	    return FALSE;
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
	    return TRUE;
	}
	++start;
    }
}

static sam_bool
sam_try_parse_number(/*@in@*/ char     **input,
		     sam_op_value *operand,
		     sam_op_type  *optype)
{
    char *endptr;

    if ((*optype & SAM_OP_TYPE_INT) != 0) {
	operand->i = strtol(*input, &endptr, 0);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0')) {
	    *optype = SAM_OP_TYPE_INT;
	    *input = endptr;
	    return TRUE;
	}
    }
    if ((*optype & SAM_OP_TYPE_FLOAT) != 0) {
	operand->f = strtod(*input, &endptr);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0')) {
	    *optype = SAM_OP_TYPE_FLOAT;
	    *input = endptr;
	    return TRUE;
	}
    }

    return FALSE;
}

static sam_bool
sam_try_parse_escape_sequence(char **input,
			      int   *c)
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
	    return FALSE;
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
	    return FALSE;
    }
    *input = start++;

    return TRUE;
}

static sam_bool
sam_try_parse_char(/*@in@*/ char **input,
		   int  *c,
		   sam_op_type *optype)
{
    char *start = *input;

    if (*start++ != '\'') {
	return FALSE;
    }
    if (*start == '\\') {
	++start;
	if (!sam_try_parse_escape_sequence(&start, c)) {
	    return FALSE;
	}
    } else if (*start != '\0') {
	*c = *start++;
	if (*c == '\0') {
	    return FALSE;
	}
    } else {
	return FALSE;
    }
    if (*start++ != '\'') {
	return FALSE;
    }
    *input = start;
    *optype = SAM_OP_TYPE_CHAR;
    return TRUE;
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
 *  @return sam_bool#TRUE if the parse was successful, sam_bool#FALSE
 *	    otherwise.
 */
static sam_bool
sam_try_parse_operand(/*@in@*/ char	 **input,
		      /*@in@*/ sam_op_value  *operand,
		      sam_op_type	     *optype)
{
    return ((*optype & (SAM_OP_TYPE_INT | SAM_OP_TYPE_FLOAT)) != 0 &&
	    sam_try_parse_number(input, operand, optype)) ||
	((*optype & SAM_OP_TYPE_CHAR) != 0 &&
	 sam_try_parse_char(input, &operand->c, optype)) ||
	((*optype & SAM_OP_TYPE_STR) != 0 &&
	 (sam_try_parse_string(input, &operand->s, optype))) ||
	((*optype & SAM_OP_TYPE_LABEL) != 0 &&
	 (sam_try_parse_string(input, &operand->s, optype) ||
	  sam_try_parse_identifier(input, &operand->s, optype)));
}

/*@null@*/ static sam_instruction *
sam_parse_instruction(char **input)
{
    char *opcode, *start = *input;
    sam_instruction *i;

    if (!sam_try_parse_identifier(&start, &opcode, NULL)) {
	sam_error_identifier(start);
	return NULL;
    }
    sam_eat_whitespace(&start);
    if ((i = sam_instruction_new(opcode)) == NULL) {
	sam_error_opcode(opcode);
	return NULL;
    }
    if (i->optype != SAM_OP_TYPE_NONE) {
	if (!sam_try_parse_operand(&start, &i->operand, &i->optype)) {
	    sam_error_operand(i->name, start);
	    free(i);
	    return NULL;
	}
    }

    sam_eat_whitespace(&start);
    *input = start;
    return i;
}

static sam_bool
sam_check_for_colon(char **s)
{
    char *start = *s;

    if (start == NULL || *start == '\0') {
	return FALSE;
    }
    for (;;) {
	if (*start == ':') {
	    *s = start;
	    return TRUE;
	} else if (isspace(*start)) {
	    ++start;
	} else {
	    return FALSE;
	}
    }
}

/*@null@*/ static char *
sam_parse_label(char **input)
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

/*@null@*/ static char *
sam_input_read(/*@observer@*/ const char *path,
	       /*@out@*/ sam_string *s)
{
    int fd;
    void *p;
    struct stat sb;
    size_t size;

    if ((fd = open(path, O_RDONLY)) < 0) {
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
    size = sb.st_size + 1;
    p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (p == (void *)-1) {
	perror("mmap");
	if (close(fd) < 0) {
	    perror("close");
	}
	return NULL;
    }
    mmapped = TRUE;
    s->len = size;
    s->alloc = size;
    s->data = p;

    return s->data;
}

#else /* HAVE_MMAN_H */

/*@null@*/ static char *
sam_input_read(const char *path,
	       sam_string *s)
{
    FILE *in;
    char *out;

    if ((in = fopen(path, "r")) == NULL) {
	perror("fopen");
	return NULL;
    }
    if ((out = sam_string_read(in, s)) == NULL) {
	return NULL;
    }
    if (fclose(in) < 0) {
	perror("fclose");
    }

    return out;
}

#endif /* HAVE_MMAN_H */

void
sam_file_free(sam_string *s)
{
    if (mmapped) {
#ifdef HAVE_MMAN_H
	if (munmap(s->data, s->len) < 0) {
	    perror("munmap");
	}
#endif /* HAVE_MMAN_H */
    } else {
	sam_string_free(s);
    }
}

/*@null@*/ sam_bool
sam_parse(sam_string	 *s,
	  /*@null@*/ const char *file,
	  sam_array	 *instructions,
	  sam_hash_table *labels)
{
    sam_pa  cur_line = 0;
    char   *input;

    if (file == NULL) {
	input = sam_string_read(stdin, s);
    } else {
	input = sam_input_read(file, s);
    }
    if (input == NULL || *input == '\0') {
	sam_error_empty_input();
	return FALSE;
    }

#if defined(SAM_EXTENSIONS)
    /* ignore a shebang line */
    if (input[0] == '#' && input[1] == '!') {
	while (*input != '\0' && *input != '\n') {
	    ++input;
	}
    }
#endif /* SAM_EXTENSIONS */

    sam_array_init(instructions);
    sam_hash_table_init(labels);

    /* Parse as many labels as we find, then parse an instruction. */
    while (*input != '\0') {
	sam_instruction *i;
	char		*label;

	sam_eat_whitespace(&input);
	while ((label = sam_parse_label(&input))) {
	    if (!sam_hash_table_ins(labels, label, cur_line)) {
		sam_error_duplicate_label(label, cur_line);
		goto err;
	    }
	    sam_eat_whitespace(&input);
	}
	if ((i = sam_parse_instruction(&input)) == NULL) {
	    goto err;
	}
	sam_array_ins(instructions, i);
	++cur_line;
    }

    return TRUE;

err:
    sam_array_free(instructions);
    sam_hash_table_free(labels);
    return FALSE;
}
