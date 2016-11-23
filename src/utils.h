#include <stdlib.h>
// #include <stdio.h>

#ifndef __UTILS_H__
#define __UTILS_H__

/*
 * Block indexing macros
 *
 * block_lo    - returns index of first element covered by process 'id'
 * block_hi    - returns index of last element covered by process 'id'
 * block_size  - returns length of domain covered by process 'id'
 * block_owner - returns owner process of array index 'idx'
 *
 * For any of the macros, p and n stand for the number of processes and
 * number of elements in the _original_ input array, respectively.
 *
 */
#define block_lo(id, p, n) ((id) * (n) / (p))
#define block_hi(id, p, n) (block_lo((id) + 1, p, n) - 1)
#define block_size(id, p, n) (block_lo((id) + 1, p, n) - block_lo(id, p, n))
#define block_owner(idx, p, n) (((p) * ((idx) + 1) - 1) / (n))

/*
 * Cheap RAII semantics
 *
 * Prepend 'autofree' to malloc'd vectors of fundamental types to have
 * their memory free'd when they exit any given context.
 *
 */
#define autofree __attribute__((cleanup(sfree)))

/*
 * Define the cleanup function
 *
 * GCC will force this cleanup function to be called for the ptr object
 * when (if defined as 'autofree') it goes out of scope.
 *
 * This should not be called by the user.
 *
 */
__attribute__((always_inline))
inline void sfree(void *ptr)
{
        void *m_ptr = *(void **)ptr;

        // printf("Free %p ->", m_ptr);

        free(m_ptr);
        m_ptr = NULL;

        // printf(" %p\n", m_ptr);
}

/*
 * Generate random vector of ints of length 'size'
 *
 * Returns: the allocated random vector.
 *
 */
int *random_ints(int size);

/*
 * Convert a string of digits to a along integer
 *
 * Returns: the converted number.
 *
 */
int char_to_int(const char *digits);

/*
 * Print vector of ints of length 'size' as a sequence of numbers
 *
 * Returns: nothing.
 *
 */
void print_vector(int *base, int size);

#endif /* ifndef __UTILS_H__ */
