#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define NUM 5000000

int main() {
    unsigned long i;
    struct timeval tv;
    double *arr = malloc(sizeof (double) * NUM);
    double acc;

    if (arr == NULL) {
	write(2, "no memory\n", 10);
	abort();
    }

    if (gettimeofday(&tv, NULL) < 0) {
	perror("gettimeofday");
    }
    srand((long)tv.tv_sec);

    for (i = 0; i < NUM; ++i) {
	arr[i] = rand() / (RAND_MAX + 1.0);
    }
    acc = arr[0];
    while (--i >= 1) {
	switch (rand() % 7) {
	    case 0:
	    case 1:
		acc += arr[i];
		break;
	    case 2:
	    case 3:
		acc *= arr[i];
		break;
	    case 4:
	    case 5:
		acc -= arr[i];
		break;
	    case 6:
		acc /= arr[i];
		break;
	}
    }

    printf("%f\n", acc);

    return round(acc);
}
