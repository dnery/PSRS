#include <stdlib.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"
#include "qsort.h"

int *gather_pivots(int *arr, long p, long n)
{
        int *pivots = calloc(p - 1, sizeof(*pivots));
        int *samples = calloc(p * p,  sizeof(*samples));


        long i, isamples = 0;
        for (i = 0; i < p; i++) {

                long lo = block_lo(i, p, n);

                // p = 8, n = 16
                printf("sampling from %ld to %ld\n", lo, lo + (p - 1) * (n / (p * p)));

                long j;
                for (j = lo; j <= lo + ((p - 1) * (n / (p * p))); j += (n / (p * p))) {
                        printf("j: %ld, isamples: %ld\n", j, isamples);
                        samples[isamples++] = arr[j];
                }
        }

        printf("samples:\n");
        print_vector(samples, p * p);

        quicksort(samples, p * p);

        printf("samples:\n");
        print_vector(samples, p * p);

        long ipivots = 0;
        for (i = p + (p / 2) - 1; i < (p - 1) * p + (p / 2); i += p)
                pivots[ipivots++] = samples[i];

        printf("pivots:\n");
        print_vector(pivots, p - 1);

        //free(samples);
        return pivots;
}

void sort_sublists(int *arr, long p, long n)
{
        long id; /**/
        long lo; /**/
        long hi; /**/
        #pragma omp parallel private(id, lo, hi) num_threads(p)
        {
                /* who am I? */
                id = omp_get_thread_num();
                lo = block_lo(id, p, n);
                hi = block_hi(id, p, n);

                /* sort sublist */
                _quicksort(arr, lo, hi);
        }
}

int main(int argc, char *argv[])
{
        MPI_Init(&argc, &argv);

        if (argc != 3) {
                printf("Usage is: GXX_psrs <n> <p>\n\n");
                printf("  <n>: \tSize of random array to sort.\n");
                printf("  <p>: \tNumber of process accross which to run.\n");
                exit(EXIT_FAILURE);
        }

        long n = char_to_long(argv[1]);
        long p = char_to_long(argv[2]);
        int *arr = random_ints(n);

        MPI_Comm workers;
        MPI_Comm_spawn("./worker", argv + 1, p, MPI_INFO_NULL, 0,
                        MPI_COMM_SELF, &workers, MPI_ERRCODES_IGNORE);

        /* subsort, send */
        sort_sublists(arr, p, n);

        /* status */
        print_vector(arr, n);

        /* samples, pivots */
        int *pivots = gather_pivots(arr, p, n);

        long i;
        for (i = 0; i < p; i++) {
                int bufsize = block_size(i, p, n);
                int *sendbuf = arr + block_lo(i, p, n);
                MPI_Send(sendbuf, bufsize, MPI_INT, i, 0, workers);
        }

        MPI_Barrier(workers);

        free(arr);
        free(pivots);
        MPI_Finalize();
        return EXIT_SUCCESS;
}
