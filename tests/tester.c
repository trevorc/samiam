/*
 * tester.c         run the samiam testcases
 *
 * part of samiam - the fast sam interpreter
 *
 * Copyright (c) 2006 Trevor Caira, Jimmy Hartzell
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 600
#include "../config.h"

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <sam.h>

#include "../sam_util.h"

typedef struct {
    char *filename;
    long  rv;
} sam_test;

sam_array tests;

static int
parse_db(const char *path,
	 size_t     *len,
	 char      **s)
{
    int fd;
    char *p;
    struct stat sb;
    size_t size;

    if ((fd = open(path, O_RDONLY)) < 0) {
	perror("open");
	return -1;
    }
    if (fstat(fd, &sb) < 0) {
	perror("fstat");
	if (close(fd) < 0) {
	    perror("close");
	}
	return -1;
    }
    if (!S_ISREG(sb.st_mode)) {
	fprintf(stderr, "%s is not a regular file\n", path);
	if (close(fd) < 0) {
	    perror("close");
	}
	return -1;
    }
    size = (size_t)sb.st_size;
    p = mmap((void *)0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, (off_t)0);
    if (p == MAP_FAILED) {
	perror("mmap");
	if (close(fd) < 0) {
	    perror("close");
	}
	return -1;
    }
    *len = size;
    *s = p;

    sam_array_init(&tests);
    while (*p != '\0') {
	char *name = p;
	long  rv;
	sam_test *test;

	while (*p != '\0' && isspace(*p)) {
	    if (*p == '\0') {
		break;
	    }
	    ++p;
	}
	if (isdigit(*p)) {
	    break;
	}
	while (*p != '\0' && !isdigit(*p)) {
	    if (*p == '\0') {
		break;
	    }
	    *p++ = '\0';
	}
	rv = strtol(p, &p, 0);
	test = sam_malloc(sizeof (sam_test));
	test->rv = rv;
	test->filename = name;
	sam_array_ins(&tests, test);
    }

    return 0;
}

int
main(int	       argc,
     const char *const argv[])
{
    size_t i, s;
    char *p;
    const char *filename = "tests.db";
    sam_test **tests_arr;

    if (argc > 1) {
	filename = argv[1];
    }
    if (parse_db(filename, &s, &p) < 0) {
	return 2;
    }

    tests_arr = (sam_test **)tests.arr;
    puts("Actual\tExpected\tTest case");
    for (i = 0; i < tests.len; ++i) {
	pid_t p = fork();
	if (p == 0) {
	    long rv = sam_main(1, tests_arr[i]->filename);
	    if (rv != tests_arr[i]->rv) {
		printf("%ld\t%ld\t%s\n", rv, tests_arr[i]->rv, tests_arr[i]->filename);
	    }
	    return 0;
	}
	if (p == -1) {
	    perror("fork");
	    return 1;
	}
    }
    munmap(p, s);
    sam_array_free(&tests);

    return 0;
}
