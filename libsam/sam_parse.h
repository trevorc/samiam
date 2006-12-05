#ifndef SAM_PARSE_H
#define SAM_PARSE_H

/*@null@*/ extern sam_bool sam_parse(/*@out@*/ sam_string *s,
				     /*@in@*/ /*@null@*/ const char *file,
				     /*@out@*/ sam_array  *instructions,
				     /*@out@*/ sam_array  *labels);
extern void sam_file_free(sam_string *s);

#endif /* SAM_PARSE_H */
