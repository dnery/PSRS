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

int _partition(int *base, int l, int r);

void _quicksort(int *base, int l, int r);

#endif /* ifndef __QSORT_H__ */
