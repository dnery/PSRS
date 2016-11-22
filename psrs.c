/* worker program */
#include<stdlib.h>
#include<stdio.h>

/* Parallelism */
#include<mpi.h>
#include<omp.h>

/* indexing macros */
#define block_lo(id, p, n) ((id) * (n) / (p))
#define block_hi(id, p, n) (block_lo((id) + 1, p, n) - 1)
#define block_size(id, p, n) (block_lo((id) + 1, p, n) - block_lo(id, p, n))
#define block_owner(idx, p, n) (((p) * ((idx) + 1) - 1) / (n))

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
 * Print 'size' sized base vector
 */
void print_vector(int *base, long size)
{
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        printf("\nworker %d:\n", rank);

        long i;
        for (i = 0; i < size; i++) {
                printf("%d", base[i]);
                printf(i == size - 1 ? ".\n" : " ");
        }
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
        MPI_Init(&argc, &argv);

        MPI_Comm parent;
        MPI_Comm_get_parent(&parent);

        int my_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* ready args */
        long n = char_to_long(argv[1]);
        long p = char_to_long(argv[2]);

        /* ready data */
        int my_n = block_size(my_rank, p, n);
        int *arr = malloc(my_n * sizeof(*arr));

        /* Receive */
        MPI_Status status;
        MPI_Recv(arr, my_n, MPI_INT, 0, 0, parent, &status);

        /* status */
        print_vector(arr, my_n);

        /* sorting */
        quicksort(arr, my_n);

        /* results */
        print_vector(arr, my_n);

        free(arr);
        MPI_Finalize();
        return EXIT_SUCCESS;
}
