#include <stdio.h>
#include <stdlib.h>
#include "libjbr.h"

char buffer[20];

int
readInt(int *n)
{
    printf("Processor Input (enter integer): ");
    *n = atoi(fgets(buffer,20,stdin));
    return 0;
}

int
readFloat(double *f)
{
    printf("Processor Input (enter float): ");
    *f = atof(fgets(buffer,20,stdin));
    return 0;
}

int
readChar(char *c)
{
    printf("Processor Input (enter character): ");
    *c = getchar();
    getchar();
    return 0;
}

int
readString(char **s)
{
    *s = NULL;
    return 0;
}

int
printChar(char c)
{
    printf("Processor Output: %c\n",c);
    return 0;
}

int
printFloat(double f)
{
    printf("Processor Output: %f\n",f);
    return 0;
}

int
printInt(int i)
{
    printf("Processor Output: %i\n",i);
    return 0;
}

int
printString(char *s)
{
    printf("Processor Output: %s\n",s);
    return 0;
}
