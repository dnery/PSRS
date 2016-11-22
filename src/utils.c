#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>

#include "utils.h"

/*
 * Generate 'size' sized random vector of ints
 *
 */
int *random_ints(long size)
{
        int *base = malloc(size * sizeof(*base));

        /* populate array */
        long i;
        srand(time(NULL));
        for (i = 0; i < size; i++)
                base[i] = rand() % 100;

        return base;
}

/*
 * Convert a string of digits to long integer
 *
 * The use of conversion function of our own lets us to store a number as
 * great as long allows.
 *
 */
long char_to_long(const char *digits)
{
        char *d = (char *)digits;
        long number = 0;

        /* char_to_long ascii char to number, digit by digit */
        do number = number * 10 + (*d - 48);
        while (*(++d) != '\0');

        return number;
}

/*
 * Print 'size' sized base vector of ints
 *
 */
void print_vector(int *base, long size)
{
        long i;
        for (i = 0; i < size; i++) {
                printf("%d", base[i]);
                printf(i == size - 1 ? ".\n" : " ");
        }
}
