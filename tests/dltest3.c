#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sam.h>
#include <libsam/sdk.h>

#include "dltest3.h"

sam_error
printy(sam_execution_state *s)
{
    sam_ml *m;
    sam_ha ha;
    sam_error err;
    char *str;

    if ((m = sam_pop(s)) == NULL) {
	return sam_error_stack_underflow();
    }
    if (m->type != SAM_ML_TYPE_HA) {
	sam_ml_type t = m->type;
	free(m);
	return sam_error_stack_input(s, "first", t, SAM_ML_TYPE_HA);
    }
    ha = m->value.ha;
    free(m);
    if ((err = sam_string_to_char_array(s, &str, ha)) != SAM_OK) {
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
