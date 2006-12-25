#include <stdio.h>
#include <stdlib.h>
#include <sam.h>
#include <libsam/sdk.h>

#include "dltest.h"

sam_error
test(__attribute__((unused)) sam_execution_state *s)
{
    puts("hello world!");
    return 0;
}

sam_error
addemup(sam_execution_state *s)
{
    long i;
    sam_ml *m;
    sam_ml_value v = { 0 };

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != SAM_ML_TYPE_INT) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(s, "first", t, SAM_ML_TYPE_INT);
    }
    i = m->value.i;
    free(m);

    while (i-- > 0) {
	if ((m = sam_pop(s)) == NULL) {
	    return sam_error_stack_underflow();
	}
	if (m->type == SAM_ML_TYPE_INT) {
	    v.i += m->value.i;
	}
	free(m);
    }
    if (!sam_push(s, sam_ml_new(v, SAM_ML_TYPE_INT))) {
	return sam_error_stack_overflow();
    }

    return SAM_OK;
}
