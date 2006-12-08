/*
 * sam_parse.c      read and parse sam from stdin or a file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(HAVE_MMAN_H)
# include <sys/stat.h>
# include <sys/mman.h>
# include <fcntl.h>
#endif /* HAVE_MMAN_H */

#if defined(HAVE_UNISTD_H) || defined(HAVE_MMAN_H)
# include <unistd.h>
#endif /* HAVE_UNISTD_H || HAVE_MMAN_H */

#if defined(HAVE_DLFCN_H)
# include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#include "sam_util.h"
#include "sam_main.h"
#include "sam_execute.h"
#include "sam_parse.h"

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
	fprintf(stderr, "error: unknown instruction found: %s.\n", s);
    }
}

/** There was a problem parsing an operand. */
static void
sam_error_operand(const char *opcode,
		  char	     *operand)
{
    if ((options & quiet) == 0) {
	char *start = operand;

	if (*start == ':' || *start == '"' || *start == '\'') {
	    *start = '\0';
	}

	fprintf(stderr,
		"error: couldn't parse operand for %s: %s.\n",
		opcode, operand);
    }
}

static sam_label *
sam_label_new(/*@observer@*/ char *name,
	      sam_program_address  pa)
{
    sam_label *l = sam_malloc(sizeof (sam_label));
    l->name = name;
    l->pa = pa;
    return l;
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
			 /*@null@*/ sam_type *optype)
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
		*optype = TYPE_LABEL;
	    }
	    return TRUE;
	}
	++start;
    }
}

static sam_bool
sam_try_parse_string(/*@in@*/  char **input,
		     /*@out@*/ char **string,
		     /*@null@*/ sam_type *optype)
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
		if ((*optype & TYPE_LABEL) != 0) {
		    *optype = TYPE_LABEL;
		} else {
		    *optype = TYPE_STR;
		}
	    }
	    return TRUE;
	}
	++start;
    }
}

static sam_bool
sam_try_parse_number(/*@in@*/ char     **input,
		     sam_value  *operand,
		     sam_type	*optype)
{
    char     *endptr;

    if ((*optype & TYPE_INT) != 0) {
	operand->i = strtol(*input, &endptr, 0);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0')) {
	    *optype = TYPE_INT;
	    *input = endptr;
	    return TRUE;
	}
    }
    if ((*optype & TYPE_FLOAT) != 0) {
	operand->f = (sam_float)strtod(*input, &endptr);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0')) {
	    *optype = TYPE_FLOAT;
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
	    *c = (int)*prev;
	    break;
	case 'a':
	    *c = (int)'\a';
	    break;
	case 'b':
	    *c = (int)'\b';
	    break;
	case 'f':
	    *c = (int)'\f';
	    break;
	case 'n':
	    *c = (int)'\n';
	    break;
	case 'r':
	    *c = (int)'\r';
	    break;
	case 't':
	    *c = (int)'\t';
	    break;
	case 'v':
	    *c = (int)'\v';
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
	    *c = (int)n;
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
		   sam_type *optype)
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
	*c = (int)*start++;
	if (*c == (int)'\0') {
	    return FALSE;
	}
    } else {
	return FALSE;
    }
    if (*start++ != '\'') {
	return FALSE;
    }
    *input = start;
    *optype = TYPE_CHAR;
    return TRUE;
}

/**
 *  Advance past the operand of a certain type if we can find one, and set
 *  the type to what we find.
 *
 *  @param input A pointer to the string which should be tested for
 *		 containing an operand. Update to the point after the
 *		 operand if it's found, otherwise print an error.
 *  @param operand A pointer to a #sam_value where the result of the
 *		   parse should be stored if successful.
 *  @param optype A pointer to the union of the possible types the
 *		  operand could be; updated to what is found.
 *
 *  @return sam_bool#TRUE if the parse was successful, sam_bool#FALSE
 *	    otherwise.
 */
static sam_bool
sam_try_parse_operand(/*@in@*/ char	 **input,
		      /*@in@*/ sam_value  *operand,
		      sam_type	 *optype)
{
    return ((*optype & (TYPE_INT | TYPE_FLOAT)) != 0 &&
	    sam_try_parse_number(input, operand, optype)) ||
	((*optype & TYPE_CHAR) != 0 &&
	 sam_try_parse_char(input, &operand->c, optype)) ||
	((*optype & TYPE_STR) != 0 &&
	 (sam_try_parse_string(input, &operand->s, optype))) ||
	((*optype & TYPE_LABEL) != 0 &&
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
	puts(opcode);
	sam_error_opcode(start);
	return NULL;
    }
    if (i->optype != TYPE_NONE) {
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
    size = (size_t)sb.st_size;
    p = mmap((void *)0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, (off_t)0);
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
    if (s->data[s->len-1] != '\n' && s->data[s->len-1] != '\0') {
	fprintf(stderr, "error: %s does not end with a newline.\n", path);
	sam_file_free(s);
	return NULL;
    }
    s->data[s->len-1] = '\0';

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
sam_parse(sam_string *s,
	  const char *file,
	  sam_array  *instructions,
	  sam_array  *labels)
{
    sam_program_address	 cur_line = 0;
    char		*input;

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
#endif

    sam_array_init(instructions);
    sam_array_init(labels);

    while (*input != '\0') {
	char		*label;
	sam_instruction *i;

	sam_eat_whitespace(&input);
	while ((label = sam_parse_label(&input))) {
	    sam_array_ins(labels, sam_label_new(label, cur_line));
	}
	sam_eat_whitespace(&input);
	if ((i = sam_parse_instruction(&input)) == NULL) {
	    sam_array_free(instructions);
	    sam_array_free(labels);
	    return FALSE;
	}
	sam_array_ins(instructions, i);
	++cur_line;
    }

    return TRUE;
}
