#include "qsort.h"

/*
 * Quicksort partition function
 *
 * This will give the pivot position for the next iteration.
 *
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
 *
 */
void _quicksort(int *base, long l, long r)
{
        if (l >= r)
                return;

        long j = _partition(base, l, r);   /* pivot position */
        _quicksort(base, j + 1, r);        /* right chunk */
        _quicksort(base, l, j - 1);        /* left chunk */
}
