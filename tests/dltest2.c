#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libsam/sdk.h>

#include "dltest2.h"

sam_error
power(sam_es *restrict es)
{
    sam_ml *restrict m2 = sam_es_stack_pop(es);
    if (m2 == NULL) {
	return sam_error_stack_underflow(es);
    }

    sam_ml *restrict m1 = sam_es_stack_pop(es);
    if (m1 == NULL) {
	return sam_error_stack_underflow(es);
    }
    m1->value.f = pow(m1->type == SAM_ML_TYPE_FLOAT?
	      m1->value.f: m1->value.i,
	      m2->type == SAM_ML_TYPE_FLOAT?
	      m2->value.f: m2->value.i);
    m1->type = SAM_ML_TYPE_FLOAT;
    free(m2);

    return sam_es_stack_push(es, m1)?
	SAM_OK: sam_error_stack_overflow(es);
}
