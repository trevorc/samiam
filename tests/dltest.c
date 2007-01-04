#include <stdio.h>
#include <stdlib.h>
#include <libsam/sdk.h>

#include "dltest.h"

sam_error
test(__attribute__((unused)) const sam_es *restrict es)
{
    puts("hello world!");
    return SAM_OK;
}

sam_error
addemup(sam_es *restrict es)
{
    long i;

    {
	sam_ml *restrict m = sam_es_stack_pop(es);

	if (m == NULL) {
	    return sam_error_stack_underflow(es);
	}
	if (m->type != SAM_ML_TYPE_INT) {
	    sam_ml_type t = m->type;
	    free(m);
	    return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_INT);
	}
	i = m->value.i;
	free(m);
    }

    sam_ml_value v = {.i = 0};
    while (i-- > 0) {
	sam_ml *restrict m = sam_es_stack_pop(es);

	if (m == NULL) {
	    return sam_error_stack_underflow(es);
	}
	if (m->type == SAM_ML_TYPE_INT) {
	    v.i += m->value.i;
	}
	free(m);
    }
    return sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_INT))?
	SAM_OK: sam_error_stack_overflow(es);
}
