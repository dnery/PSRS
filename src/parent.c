#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"
#include "qsort.h"

int *gather_pivots(int *arr, int p, int n)
{
        int *pivots = calloc(p - 1, sizeof(*pivots));
        int *samples = calloc(p * p,  sizeof(*samples));


        int i, isamples = 0;
        for (i = 0; i < p; i++) {

                int lo = block_lo(i, p, n);

                // printf("sampling from %ld to %ld\n", lo, lo + (p - 1) * (n / (p * p)));

                int j;
                for (j = lo; j <= lo + ((p - 1) * (n / (p * p))); j += (n / (p * p)))
                        samples[isamples++] = arr[j];
        }

        quicksort(samples, p * p);

        int ipivots = 0;
        for (i = p + (p / 2) - 1; i < (p - 1) * p + (p / 2); i += p)
                pivots[ipivots++] = samples[i];

        return pivots;
}

void sort_sublists(int *arr, int p, int n)
{
        int id; /**/
        int lo; /**/
        int hi; /**/

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

int inbound(int start, int end, int *idx, int *subsubln){
        int count = 0;

        for (int i = start; i < end; ++i) {
                if (idx[i] >= subsubln[i])
                        count++;
        }
        return count < (end-start);
}

int **merge_result(int **subsubl, int *subsubln, int p, int *nres)
{
        int **res = malloc(p * sizeof(**res));
        int *idx = calloc(p * p, sizeof(*idx));

        #pragma omp parallel num_threads(p)
        {
                int my_id = omp_get_thread_num();
                int start = my_id * p;
                int end = p + start;
                int *partial_res = NULL;
                int size_partial = 0;

                while (inbound(start, end, idx, subsubln)){
                        int min = INT_MAX;
                        int minidx = INT_MAX;
                        for (int i = start; i < end; ++i) {
                                if (idx[i] >= subsubln[i])
                                        continue;
                                if (min > subsubl[i][idx[i]]){
                                        min = subsubl[i][idx[i]];
                                        minidx = i;
                                }
                        }
                        idx[minidx]++;
                        partial_res = realloc(partial_res, ++size_partial * (sizeof(*partial_res)));
                        partial_res[size_partial-1] = min;
                }
                res[my_id] = partial_res;
                nres[my_id] = size_partial;
        }
        return res;
}

void print_result(int **result, int *nresult, int p)
{
        int i, j;
        char *c = "";

        for (i = 0; i < p; i++){
                for (j = 0; j < nresult[i]; j++){
                        printf("%s%d", c, result[i][j]);
                        c = ", ";
                }
        }
        printf(".");
}

int main(int argc, char *argv[])
{
        if (argc != 3) {
                printf("Usage is: GXX_psrs <n> <p>\n\n");
                printf("  <n>: \tSize of random array to sort.\n");
                printf("  <p>: \tNumber of process accross which to run.\n");
                exit(EXIT_FAILURE);
        }

        int n = char_to_int(argv[1]);
        int p = char_to_int(argv[2]);

        if (p * p > n) {
                printf("(p * p) has to be smaller or equal than (n)!\n");
                exit(EXIT_FAILURE);
        }

        MPI_Init(&argc, &argv);

        int *arr = random_ints(n);

        MPI_Comm workers;
        MPI_Comm_spawn("./worker", argv + 1, p, MPI_INFO_NULL, 0,
                        MPI_COMM_SELF, &workers, MPI_ERRCODES_IGNORE);

        int my_rank;
        MPI_Comm_rank(workers, &my_rank);

        /* subsort, send */
        sort_sublists(arr, p, n);

        /* status */
        //print_vector(arr, n);

        int i;
        for (i = 0; i < p; i++) {
                int bufsize = block_size(i, p, n);
                int *sendbuf = arr + block_lo(i, p, n);
                MPI_Send(sendbuf, bufsize, MPI_INT, i, 0, workers);
        }

        /* samples, pivots */
        int *pivots = gather_pivots(arr, p, n);
        MPI_Bcast(pivots, p - 1, MPI_INT, MPI_ROOT, workers);

        /* RECEIVE SUBLISTS */
        int *subsubln = malloc((p * p) * sizeof(*subsubln));
        int **subsubl = malloc((p * p) * sizeof(*subsubl));

        for (i = 0; i < p; i++) {

                int j;
                for (j = 0; j < p; j++) {

                        int idx = i * p + j;
                        MPI_Recv(&(subsubln[idx]), 1, MPI_INT, i, i, workers, NULL);

                        subsubl[idx] = malloc(subsubln[idx] * sizeof(*(subsubl[idx])));
                        MPI_Recv(subsubl[idx], subsubln[idx], MPI_INT, i, i, workers, NULL);

                        //printf("%d: received %d elems\n", i, subsubln[idx]);
                        //print_vector(subsubl[idx], subsubln[idx]);
                }
        }

        int *nresult = malloc(p * sizeof(*nresult));
        int **result = merge_result(subsubl, subsubln, p, nresult);
        print_result(result, nresult, p);

        MPI_Finalize();
        return EXIT_SUCCESS;
}
