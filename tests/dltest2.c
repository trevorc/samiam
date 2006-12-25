#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sam.h>
#include <libsam/sdk.h>

#include "dltest2.h"

sam_error
power(sam_execution_state *s)
{
    sam_ml *m1, *m2;

    if ((m2 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if ((m1 = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    m1->value.f = pow(m1->type == SAM_ML_TYPE_FLOAT?
	      m1->value.f: m1->value.i,
	      m2->type == SAM_ML_TYPE_FLOAT?
	      m2->value.f: m2->value.i);
    m1->type = SAM_ML_TYPE_FLOAT;
    free(m2);
    if (!sam_push(s, m1)) {
	return sam_error_stack_overflow();
    }

    return SAM_OK;
}
