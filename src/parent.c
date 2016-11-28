#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"
#include "qsort.h"

int *gather_pivots(int *elems, int nproc, int nelem)
{
        autofree int *samples = malloc((nproc * nproc) * sizeof(*samples));
        int *pivots = malloc((nproc - 1) * sizeof(*pivots));

        int iproc;      /* processor iterator */
        int isampler;   /* sampler loop iterator */
        int isampled;   /* sample vector iterator */
        int ipivots;    /* pivot vector iterator */

        for (isampled = 0, iproc = 0; iproc < nproc; iproc++) {

                /* gather samples from elements */
                int lo = block_lo(iproc, nproc, nelem);
                int hi = lo + ((nproc - 1) * (nelem / (nproc * nproc)));
                int step = (nelem / (nproc * nproc));

                for (isampler = lo; isampler <= hi; isampler += step)
                        samples[isampled++] = elems[isampler];
        }

        /* sort the samples vector */
        quicksort(samples, nproc * nproc);

        /* gather pivots from samples */
        int lo = nproc + (nproc / 2) - 1;
        int hi = (nproc - 1) * nproc + (nproc / 2) - 1;

        for (ipivots = 0, isampled = lo; isampled <= hi; isampled += nproc)
                pivots[ipivots++] = samples[isampled];

        return pivots;
}

void sort_sublists(int *elems, int nproc, int nelem)
{
        int id; /* current thread id */
        int lo; /* current thread start index */
        int hi; /* current thread end index */

        #pragma omp parallel private(id, lo, hi) num_threads(nproc)
        {
                /* who am I? */
                id = omp_get_thread_num();
                lo = block_lo(id, nproc, nelem);
                hi = block_hi(id, nproc, nelem);

                /* sort sublist */
                _quicksort(elems, lo, hi);
        }
}

static inline int inbound(int start, int end, int *idx, int *subsubln)
{
        int count = 0;

        for (int i = start; i < end; ++i)
                if (idx[i] >= subsubln[i])
                        count++;

        return count < (end-start);
}

void merge_results(int **merged, int *nmerged, int **elems, int *nelem, int nproc)
{
        int id;         /* current processor id */
        int lo;         /* local start index */
        int hi;         /* local end index */
        int *partials;  /* local partial vector */
        int npartials;  /* local partial vector length */

        /* sublist index tracking vector */
        autofree int *idxs = calloc(nproc * nproc, sizeof(*idxs));

        #pragma omp parallel private(id, lo, hi, partials, npartials) num_threads(nproc)
        {
                id = omp_get_thread_num();
                lo = id * nproc;
                hi = nproc + lo;
                partials = NULL;
                npartials = 0;

                while (inbound(lo, hi, idxs, nelem)) {
                        int small = INT_MAX;
                        int ismall = INT_MAX;

                        for (int isublist = lo; isublist < hi; ++isublist) {
                                if (idxs[isublist] >= nelem[isublist])
                                        continue;

                                if (small > elems[isublist][idxs[isublist]]) {
                                        small = elems[isublist][idxs[isublist]];
                                        ismall = isublist;
                                }
                        }

                        partials = realloc(partials, (npartials + 1) *
                                        (sizeof(*partials)));
                        partials[npartials] = small;
                        idxs[ismall]++;
                        npartials++;
                }

                merged[id] = partials;
                nmerged[id] = npartials;
        }
}

void print_results(int **results, int *result_lens, int nproc)
{
        static char *sep = "";

        for (int i = 0; i < nproc; i++) {
                for (int j = 0; j < result_lens[i]; j++) {
                        printf("%s%d", sep, results[i][j]);
                        sep = ", ";
                }
        }
        printf(".\n");
}

int main(int argc, char *argv[])
{
        if (argc != 3) {
                printf("Usage is: make run <n> <p>\n\n");
                printf("  <n>: \tSize of random array to sort.\n");
                printf("  <p>: \tNumber of processors accross which to run.\n");
                exit(EXIT_FAILURE);
        }

        int n = char_to_int(argv[1]);
        int p = char_to_int(argv[2]);

        if (p * p > n) {
                printf("(p * p) has to be smaller than or equal to (n)!\n");
                exit(EXIT_FAILURE);
        }

        MPI_Init(&argc, &argv);
        autofree int *elems = random_ints(n);

        MPI_Comm workers;
        MPI_Comm_spawn("./worker", argv + 1, p, MPI_INFO_NULL, 0,
                        MPI_COMM_SELF, &workers, MPI_ERRCODES_IGNORE);

        /* status */
        printf("random:\n");
        print_vector(elems, n);

        /* subsort */
        sort_sublists(elems, p, n);

        for (int iproc = 0; iproc < p; iproc++) {
                int bufsize = block_size(iproc, p, n);
                int *sendbuf = elems + block_lo(iproc, p, n);
                MPI_Send(sendbuf, bufsize, MPI_INT, iproc, 0, workers);
        }

        /* samples, pivots */
        autofree int *pivots = gather_pivots(elems, p, n);
        MPI_Bcast(pivots, p - 1, MPI_INT, MPI_ROOT, workers);

        /* receive sorted sublist arrays */
        autofree int **sublists = malloc((p * p) * sizeof(*sublists));
        autofree int *sublist_lens = malloc((p * p) * sizeof(*sublist_lens));

        for (int iproc = 0; iproc < p; iproc++) {

                for (int jproc = 0; jproc < p; jproc++) {

                        int idx = iproc * p + jproc;

                        MPI_Recv(&(sublist_lens[idx]), 1, MPI_INT, iproc,
                                        iproc, workers, NULL);

                        sublists[idx] = malloc(sublist_lens[idx] *
                                        sizeof(*(sublists[idx])));

                        MPI_Recv(sublists[idx], sublist_lens[idx], MPI_INT,
                                        iproc, iproc, workers, NULL);
                }
        }

        /* merge sublist arrays into results with omp */
        autofree int **results = malloc(p * sizeof(*results));
        autofree int *result_lens = malloc(p * sizeof(*result_lens));
        merge_results(results, result_lens, sublists, sublist_lens, p);

        /* status */
        printf("sorted:\n");
        print_results(results, result_lens, p);

        /* free derref sublists */
        for (int i = 0; i < p * p; i ++)
                free(sublists[i]);

        /* free derref results */
        for (int i = 0; i < p; i ++)
                free(results[i]);

        MPI_Finalize();
        return EXIT_SUCCESS;
}
