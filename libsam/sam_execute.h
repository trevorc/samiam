#ifndef SAM_EXECUTE_H
#define SAM_EXECUTE_H

extern const sam_instruction sam_instructions[];
extern sam_exit_code sam_execute(/*@in@*/ sam_array    *instructions,
				 /*@in@*/ sam_array    *labels,
				 /*@in@*/ sam_io_funcs *io_funcs);

#endif /* SAM_EXECUTE_H */
