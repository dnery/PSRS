#include<stdlib.h>
#include<limits.h>
#include<stdio.h>
#include<time.h>

/* Parallelism */
#include<mpi.h>
#include<omp.h>

/* Simplify syntax */
#define quicksort(array, size) _quicksort((array), 0, ((size) - 1))

/*
 * Convert a string of digits to a number
 *
 * The use of conversion function of our own lets
 * us to store a number as great as long allows.
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
 * Quicksort partition function
 *
 * This will give the pivot position for the next iteration.
 */
long _partition(int *base, long l, long r)
{
        long i = l;             /* left approximation index */
        long j = r + 1;         /* right approximation index */
        int pivot = base[l];

        while (i < j) {
                /* left-approx i to pivot */
                do ++i;
                while (base[i] <= pivot && i <= r);

                /* right-approx j to pivot */
                do --j;
                while (base[j] > pivot && j >= l);

                /* do swap if swap is possible */
                if (i < j) {
                        int aux = base[i];
                        base[i] = base[j];
                        base[j] = aux;
                }
        }

        /* replace pivot */
        base[l] = base[j];
        base[j] = pivot;

        return j;
}

/*
 * Quicksort entry point function
 *
 * The sorting for base is done in-place.
 */
void _quicksort(int *base, long l, long r)
{
        if (l >= r)
                return;

        long j = _partition(base, l, r);   /* pivot position */
        _quicksort(base, j + 1, r);        /* right chunk */
        _quicksort(base, l, j - 1);        /* left chunk */
}

int main(int argc, char *argv[])
{
        if (argc != 3) {
                printf("Usage is: GXX_psrs <n> <p>\n\n");
                printf("  <n>: \tSize of random array to sort.\n");
                printf("  <p>: \tNumber of process accross which to run.\n");
                exit(EXIT_FAILURE);
        }

        /* ready data */
        long n = char_to_long(argv[1]);
        long p = char_to_long(argv[2]);
        int *a = malloc(n * sizeof(*a));

        /* populate array */
        long i;
        srand(time(NULL));
        for (i = 0; i < n; i++)
                a[i] = rand() % INT_MAX;

        /* output status */
        for (i = 0; i < n; i++) {
                printf("%d", a[i]);
                printf(i == n - 1 ? ".\n" : " ");
        }

        quicksort(a, n);

        /* output results */
        for (i = 0; i < n; i++) {
                printf("%d", a[i]);
                printf(i == n - 1 ? ".\n" : " ");
        }

        free(a);
        return EXIT_SUCCESS;
}
