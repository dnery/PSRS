#include <stdlib.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"

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

        /* Receive array */
        MPI_Status status;
        MPI_Recv(arr, my_n, MPI_INT, 0, 0, parent, &status);

        /* Receive pivot */
        int *pivots = malloc((p - 1) * sizeof(*pivots));
        MPI_Bcast(pivots, p - 1, MPI_INT, 0, parent);

        /* status */
        printf("vec, pivs:\n");
        print_vector(arr, my_n);
        print_vector(pivots, p - 1);

        MPI_Finalize();
        return EXIT_SUCCESS;
}
