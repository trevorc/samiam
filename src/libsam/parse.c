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

#include "libsam.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libsam/array.h>
#include <libsam/es.h>
#include <libsam/hash_table.h>
#include <libsam/string.h>
#include <libsam/main.h>
#include <libsam/opcode.h>

#include "parse.h"

#if defined(HAVE_MMAN_H)
# include <sys/stat.h>
# include <sys/mman.h>
# include <fcntl.h>
#endif /* HAVE_MMAN_H */

#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif /* HAVE_MMAN_H */

#if 0
#if defined(isspace)
# undef isspace
#endif
#define isspace(c) ((c) == ' ' || (c) == '\n' || (c) == '\t')
#endif

typedef enum {
    SAM_DIRECTIVE_NONE,
    SAM_DIRECTIVE_ROI,
    SAM_DIRECTIVE_ROF,
    SAM_DIRECTIVE_ROS,
    SAM_DIRECTIVE_ROC,
    SAM_DIRECTIVE_GLOBAL,
    SAM_DIRECTIVE_IMPORT,
    SAM_DIRECTIVE_EXPORT,
} sam_directive_symbol;

typedef struct {
    const char *restrict name;
    sam_directive_symbol symbol;
} sam_directive;

/*
 * WHITESPACE ::= [ \n\t]+
 */
static void
sam_parse_whitespace(char *restrict *restrict s)
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
	sam_io_fprintf(es, SAM_IOS_ERR, _("error: empty source file.\n"));
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
		       _("error: couldn't parse identifier: %s.\n"),
		       s);
    }
}

static inline void
sam_error_invalid_directive(const sam_es *restrict es,
			    char *s)
{
    if (!sam_es_options_get(es, SAM_QUIET)) {
	sam_truncate(s);
	sam_io_fprintf(es,
		       SAM_IOS_ERR,
		       _("error: invalid directive: %s.\n"),
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
		       _("error: unknown opcode found: %s.\n"),
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
		       _("error: couldn't parse operand for %s: %s.\n"),
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
		       _("error: duplicate label \"%s\" was found in module "
			 "number %hu, line %hu.\n"),
		       label,
		       pa.m,
		       pa.l);
    }
}

/*
 *  IDENT ::= [A-Za-z_]+
 */
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

/*
 *  STRING ::= " CHAR* "
 */
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

/*
 *  FLOAT and INT as defined by C99
 *
 *  NUMBER ::= FLOAT | INT
 */
static inline bool
sam_try_parse_number(/*@in@*/ char **restrict input,
		     sam_op_value *restrict operand,
		     sam_op_type *restrict optype)
{
    char *endptr;

    if ((*optype & SAM_OP_TYPE_INT) != 0) {
	operand->i = strtol(*input, &endptr, 0);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0' ||
		(endptr[0] == '/' && endptr[1] == '/'))) {
	    *optype = SAM_OP_TYPE_INT;
	    *input = endptr;
	    return true;
	}
    }
    if ((*optype & SAM_OP_TYPE_FLOAT) != 0) {
	operand->f = strtod(*input, &endptr);
	if (*input != endptr &&
	    (*endptr == '"' || isspace(*endptr) || *endptr == '\0' ||
		(endptr[0] == '/' && endptr[1] == '/'))) {
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

/*
 * CHAR as in C99
 *
 * CHAR-TOKEN ::= ' CHAR '
 */
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
 *  OPERAND ::= INT | FLOAT | CHAR | STRING | IDENT
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

/*
 *  INSTRUCTION ::= IDENT OPERAND?
 */
/*@null@*/ static inline sam_instruction *
sam_parse_instruction(const sam_es *restrict es,
		      char **restrict input)
{
    char *opcode, *start = *input;

    if (!sam_try_parse_identifier(&start, &opcode, NULL)) {
	sam_error_identifier(es, start);
	return NULL;
    }
    sam_parse_whitespace(&start);

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

    sam_parse_whitespace(&start);
    *input = start;
    return i;
}

/*
 *  Returns true if there is a colon in s before the next non-whitespace
 *  character, otherwise returns false.
 */
static inline bool
sam_check_for_colon(char *restrict s)
{
    if (s == NULL || *s == '\0') {
	return false;
    }
    for (;;) {
	if (*s == ':') {
	    return true;
	} else if (isspace(*s)) {
	    ++s;
	} else {
	    return false;
	}
    }
}

/*
 *  GENERIC-STRING ::= IDENT || STRING
 *
 *  LABEL ::= GENERIC-STRING :
 */
/*@null@*/ static inline char *
sam_parse_label(char **restrict input,
		bool colon_expected)
{
    char *label, *start = *input;

    if (*start == '"') {
	if (!sam_try_parse_string(&start, &label, NULL)) {
	    return NULL;
	} 
    } else if (!sam_try_parse_identifier(&start, &label, NULL)) {
	return NULL;
    }

    if (colon_expected && !sam_check_for_colon(start)) {
	return NULL;
    }
    sam_parse_whitespace(&start);
    *start++ = '\0';
    *input = start;

    return label;
}

#if defined(HAVE_MMAN_H)

/*@null@*/ static inline char *
sam_input_read(const sam_es *restrict es,
	       /*@observer@*/ const char *restrict path,
	       /*@out@*/ sam_string *restrict s)
{
    struct stat sb;
    int fd = open(path, O_RDONLY);

    s->alloc = 0;
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
	sam_io_fprintf(es, SAM_IOS_ERR, _("error: %s is not a regular file\n"), path);
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
    s->alloc = s->len;
    s->mmapped = true;

    return s->data;
}

#else /* HAVE_MMAN_H */

/*@null@*/ static inline char *
sam_input_read(const sam_es *restrict es UNUSED,
	       const char *restrict path,
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

static bool
sam_directive_name(const char *restrict s)
{
    static const sam_directive directives[] = {
	{"roi",	    SAM_DIRECTIVE_ROI},
	{"rof",	    SAM_DIRECTIVE_ROF},
	{"ros",	    SAM_DIRECTIVE_ROF},
	{"roc",	    SAM_DIRECTIVE_ROC},
	{"global",  SAM_DIRECTIVE_GLOBAL},
	{"import",  SAM_DIRECTIVE_IMPORT},
	{"export",  SAM_DIRECTIVE_EXPORT},
	{NULL,	    SAM_DIRECTIVE_NONE},
    };

    printf("sam_directive_name: '%s'\n", s);
    for (size_t i = 0; directives[i].name != NULL; ++i) {
	if (!strcmp(s, directives[i].name)) {
	    return directives[i].symbol;
	}
    }

    puts("none");
    return SAM_DIRECTIVE_NONE;
}

/*
 *  ROI ::= .roi IDENT INT ( , INT )*
 */
static bool
sam_try_parse_roi(sam_es *restrict es UNUSED,
		  char **restrict input UNUSED)
{
    puts("roi");
    return false;
}

/*
 *  ROF ::= .rof IDENT FLOAT ( , FLOAT )*
 */
static bool
sam_try_parse_rof(sam_es *restrict es UNUSED,
		  char **restrict input UNUSED)
{
    return false;
}

/*
 * ROS ::= .ros IDENT STRING ( , STRING )*
 */
static bool
sam_try_parse_ros(sam_es *restrict es UNUSED,
		  char **restrict input UNUSED)
{
    return false;
}

/*
 *  ROC ::= .roc IDENT CHAR ( , CHAR )*
 */
static bool
sam_try_parse_roc(sam_es *restrict es UNUSED,
		  char **restrict input UNUSED)
{
    return false;
}

/*
 *  GLOBAL ::= .global IDENT ( INT )?
 */
static bool
sam_try_parse_global(sam_es *restrict es UNUSED,
		     char **restrict input UNUSED)
{
    return false;
}

/*
 *  IMPORT ::= .import IDENT
 */
static bool
sam_try_parse_import(sam_es *restrict es UNUSED,
		     char **restrict input UNUSED)
{
    return false;
}

/*
 *  EXPORT ::= .export IDENT
 */
static bool
sam_try_parse_export(sam_es *restrict es UNUSED,
		     char **restrict input UNUSED)
{
    return false;
}

/*
 *  DIRECTIVE ::= ROI ||
 *		  ROF ||
 *		  ROC ||
 *		  GLOBAL ||
 *		  IMPORT ||
 *		  EXPORT
 */
static bool
sam_parse_directive(sam_es *restrict es,
		    char **restrict input)
{
    char *start = *input;

    /*
    while (*start != '\0' && !isspace(*start++)) {
	sam_parse_whitespace(&start);
	break;
    }
    */

    sam_directive_symbol symbol = sam_directive_name(start);
    if (symbol == SAM_DIRECTIVE_NONE) {
	return false;
    }

    if (!sam_try_parse_roi(es, &start) &&
	!sam_try_parse_rof(es, &start) &&
	!sam_try_parse_roc(es, &start) &&
	!sam_try_parse_ros(es, &start) &&
	!sam_try_parse_global(es, &start) &&
	!sam_try_parse_export(es, &start) &&
	!sam_try_parse_import(es, &start)) {
	return false;
    }

    *input = start;
    return true;
}

#if defined(SAM_EXTENSIONS)
/* ignore a shebang line */
static void
sam_ignore_shebang(char **restrict s)
{
    if ((*s)[0] == '#' && (*s)[1] == '!') {
	while (**s != '\0' && **s != '\n') {
	    ++*s;
	}
    }
}

/*
 * Parse directives at the beginning of the file.
 *
 * DIRECTIVE-SECTION ::= DIRECTIVE*
 *
 * @return true if all directives were parsed successfully or none were
 *	   parsed at all, otherwise false.
 */
static bool
sam_parse_directives(sam_es *restrict es,
		     char **restrict input)
{
    return true;

    char *start = *input;

    while (*start != '\0') {
	sam_parse_whitespace(&start);

	if (*start == '\0' || *start != '.') {
	    return true;
	}
	++start;

	if (!sam_parse_directive(es, &start)) {
	    sam_error_invalid_directive(es, start);
	    return false;
	}
	sam_parse_whitespace(&start);

	if (*start != '.') {
	    return true;
	}
    }

    return false;
}
#endif /* SAM_EXTENSIONS */

/*
 * PROGRAM ::= DIRECTIVE-SECTION ( LABEL* INSTRUCTION )*
 */
/*@null@*/ bool
sam_parse(sam_es *restrict es,
	  const char *restrict file)
{
    char *input = file == NULL?
	sam_string_read(stdin, sam_es_input_get(es)):
	sam_input_read(es, file, sam_es_input_get(es));

    if (input == NULL || *input == '\0') {
	sam_error_empty_input(es);
	return false;
    }

#if defined(SAM_EXTENSIONS)
    sam_ignore_shebang(&input);

    if (!sam_parse_directives(es, &input)) {
	return false;
    }
#endif /* SAM_EXTENSIONS */

    /* Parse as many labels as we find, then parse an instruction. */
    sam_pa cur_line = {
	.l = 0,
	.m = sam_es_modules_len(es) - 1,
    };

    while (*input != '\0') {
	sam_parse_whitespace(&input);

	char *label;
	while ((label = sam_parse_label(&input, true))) {
	    if (!sam_es_labels_ins(es, label, cur_line)) {
		sam_error_duplicate_label(es, label, cur_line);
		return false;
	    }
	    sam_parse_whitespace(&input);
	}

	sam_instruction *restrict i = sam_parse_instruction(es, &input);
	if (i == NULL) {
	    return false;
	}
	sam_es_instructions_ins(es, i);
	++cur_line.l;
    }

    return true;
}
