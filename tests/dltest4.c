#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sam.h>
#include <libsam/sdk.h>

#include "dltest4.h"

#define TEMP "dltest4-temp.sam"

sam_error
iheartdis(sam_execution_state *s)
{
    sam_ml_value v;
    FILE *fh;

    fh = fopen(TEMP, "w");
    fputs("PUSHIMM 123 STOP\n", fh);
    fclose(fh);

    v.i = sam_main(0, TEMP, NULL);
    remove(TEMP);
    v.i <<= 1;

    if (!sam_push(s, sam_ml_new(v, SAM_ML_TYPE_INT))) {
	return sam_error_stack_overflow();
    }

    return SAM_OK;
}
