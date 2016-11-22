#ifndef __QSORT_H__
#define __QSORT_H__

/*
 * Quicksort
 *
 * Sort an array of integers of length 'size' using the quicksort
 * algorithm, as proposed by Cormen et al.
 *
 */
#define quicksort(array, size) _quicksort((array), 0, ((size) - 1))

long _partition(int *base, long l, long r);

void _quicksort(int *base, long l, long r);

#endif /* ifndef __QSORT_H__ */
