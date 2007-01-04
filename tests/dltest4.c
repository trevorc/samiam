#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libsam/sdk.h>

#include "dltest4.h"

#define TEMP "dltest4-temp.sam"

sam_error
me_in_the_morning(sam_es *restrict es)
{
    FILE *fh;

    fh = fopen(TEMP, "w");
    fputs("PUSHIMM 123 STOP\n", fh);
    fclose(fh);

    sam_ml_value v = {.i = sam_main(0, TEMP, NULL)};
    remove(TEMP);
    v.i <<= 1;

    return sam_es_stack_push(es, sam_ml_new(v, SAM_ML_TYPE_INT))?
	SAM_OK: sam_error_stack_overflow(es);
}
