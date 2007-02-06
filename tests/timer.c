#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libsam/sdk.h>

#define INTERVAL 1

static volatile bool lock = false;

static void
sighandler(void)
{
    lock = true;
    alarm(INTERVAL);
}

static void
commit_changes(sam_es *restrict es,
	       size_t stack_items)
{
    printf("number of items on the stack: %d.\n"
	   "pc: %lu\n"
	   "fbr: %lu\n"
	   "sp: %lu\n\n",
	   stack_items,
	   sam_es_pc_get(es),
	   sam_es_fbr_get(es),
	   sam_es_sp_get(es));
}

int
main(void)
{
    size_t stack_items = 0;
    signal(SIGALRM, (sighandler_t)sighandler);
    sighandler();

    sam_es *restrict es = sam_es_new(0, 0);
    sam_parse(es, NULL);

    for (sam_error err = SAM_OK;
	 sam_es_pc_get(es) < sam_es_instructions_len(es) && err == SAM_OK;
	 sam_es_pc_pp(es)) {
	err = sam_es_instructions_cur(es)->handler(es);

	for (sam_es_change ch; sam_es_change_get(es, &ch);) {
	    if (ch.stack) {
		stack_items += ch.add? 1: -1;
	    }
	    if (lock) {
		commit_changes(es, stack_items);
		lock = false;
	    }
	}
    }

    printf("Return value: %d\n", sam_es_stack_len(es) >= 1?
	   sam_es_stack_get(es, 0)->value.i: -1);
    sam_es_free(es);
    return 0;
}
