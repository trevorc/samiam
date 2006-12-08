#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#define SCALE 100
#define NUM 5000000

static const char *ops[] = { "ADDF", "TIMESF", "SUBF", "DIVF" };

int main() {
    unsigned long i;
    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0) {
	perror("gettimeofday");
    }
    srand48((long)tv.tv_sec);

    for (i = 1; i <= NUM; ++i) {
	printf("PUSHIMMF %f\n", drand48());
    }
    while (--i > 1) {
	puts(ops[lrand48() % 4]);
    }
    puts("FTOIR\nSTOP");

    return 0;
}
