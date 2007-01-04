#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libsam/sdk.h>

#include "dltest3.h"

sam_error
printy(sam_es *restrict es)
{
    sam_ha ha;
    {
	sam_ml *restrict m = sam_es_stack_pop(es);
	if (m == NULL) {
	    return sam_error_stack_underflow(es);
	}
	if (m->type != SAM_ML_TYPE_HA) {
	    sam_ml_type t = m->type;
	    free(m);
	    return sam_error_stack_input(es, "first", t, SAM_ML_TYPE_HA);
	}
	ha = m->value.ha;
	free(m);
    }

    sam_error err;
    char *str;
    if ((err = sam_es_string_get(es, &str, ha)) != SAM_OK) {
	return err;
    }
    if (strcmp(str, "printy") == 0) {
	puts("PRINTY.");
    } else {
	puts("no printy:(");
    }
    free(str);

    return SAM_OK;
}
