#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libsam/sdk.h>

#define INTERVAL 1

static volatile bool lock = false;
static size_t stack_items = 0;
static size_t heap_items = 0;

static void
sighandler(void)
{
    lock = true;
    alarm(INTERVAL);
}

static void
buffer_changes(const sam_es_change *restrict ch)
{
    if (ch->stack) {
	stack_items += ch->add;
	stack_items -= ch->remove;
    } else {
	heap_items += ch->add;
	heap_items -= ch->remove;
    }
}
static void
commit_changes(sam_es *restrict es)
{
    printf("number of items on the stack: %lu.\n"
	   "number of items on the heap: %lu.\n"
	   "pc: %lu\n"
	   "fbr: %lu\n"
	   "sp: %lu\n\n",
	   stack_items,
	   heap_items,
	   sam_es_pc_get(es),
	   sam_es_fbr_get(es),
	   sam_es_sp_get(es));
}

int
main(void)
{
    signal(SIGALRM, (sighandler_t)sighandler);
    sighandler();

    sam_es *restrict es = sam_es_new(0, 0);
    sam_parse(es, NULL);

    for (sam_error err = SAM_OK;
	 sam_es_pc_get(es) < sam_es_instructions_len(es) && err == SAM_OK;
	 sam_es_pc_pp(es)) {
	err = sam_es_instructions_cur(es)->handler(es);

	for (sam_es_change ch; sam_es_change_get(es, &ch);) {
	    buffer_changes(&ch);
	    if (lock) {
		commit_changes(es);
		lock = false;
	    }
	}
    }

    printf("Return value: %d\n", sam_es_stack_len(es) >= 1?
	   sam_es_stack_get(es, 0)->value.i: -1);
    sam_es_free(es);
    return 0;
}
