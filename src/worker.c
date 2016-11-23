#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"

int *padded_pivots(int *pivots, int p)
{
        int *p_pivots = malloc(p * sizeof(*p_pivots));

        int i;
        for (i = 0; i < p - 1; i++)
                p_pivots[i] = pivots[i];
        p_pivots[p - 1] = INT_MAX;

        return p_pivots;
}

void exchange_values(int *arr, int *pivots, int p, int n)
{
        int start, my_start;    /**/
        int end, my_end;        /**/
        int isublist;           /**/
        int iproc;              /**/

        /* my process rank */
        int my_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* Exchange: send between processes */
        for (iproc = 0, isublist = 0; iproc < p; iproc++) {

                start = isublist;

                while (arr[isublist] <= pivots[iproc] && isublist < n)
                        isublist++;

                end = isublist;

                if (iproc == my_rank) {

                        my_start = start;
                        my_end = end;

                } else {

                        int nsendbuf = end - start;
                        MPI_Send(&nsendbuf, 1, MPI_INT, iproc, my_rank, MPI_COMM_WORLD);
                        MPI_Send(arr + start, nsendbuf, MPI_INT, iproc, my_rank, MPI_COMM_WORLD);
                }
        }

        /* MPI parent intercomm */
        MPI_Comm parent;
        MPI_Comm_get_parent(&parent);

        /* Exchange: send between processes */
        for (iproc = 0, isublist = 0; iproc < p; iproc++) {

                int *sendbuf;
                int nsendbuf;

                if (iproc != my_rank) {
                        int nrecbuf;
                        MPI_Recv(&nrecbuf, 1, MPI_INT, iproc, iproc, MPI_COMM_WORLD, NULL);

                        int *recbuf = malloc(nrecbuf * sizeof(*recbuf));
                        MPI_Recv(recbuf, nrecbuf, MPI_INT, iproc, iproc, MPI_COMM_WORLD, NULL);

                        sendbuf = recbuf;
                        nsendbuf = nrecbuf;

                } else {

                        sendbuf = arr + my_start;
                        nsendbuf = my_end - my_start;
                }

                MPI_Send(&nsendbuf, 1, MPI_INT, 0, my_rank, parent);
                MPI_Send(sendbuf, nsendbuf, MPI_INT, 0, my_rank, parent);
        }
}

int main(int argc, char *argv[])
{
        MPI_Init(&argc, &argv);

        MPI_Comm parent;
        MPI_Comm_get_parent(&parent);

        int my_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* ready args */
        int n = char_to_int(argv[1]);
        int p = char_to_int(argv[2]);

        /* ready data */
        int my_n = block_size(my_rank, p, n);
        int *arr = malloc(my_n * sizeof(*arr));

        /* Receive array */
        MPI_Status status;
        MPI_Recv(arr, my_n, MPI_INT, 0, 0, parent, &status);

        /* Receive pivot */
        int *pivots = malloc((p - 1) * sizeof(*pivots));
        MPI_Bcast(pivots, p - 1, MPI_INT, 0, parent);
        int *p_pivots = padded_pivots(pivots, p);

        /* status */
        //printf("vec, pivs:\n");
        //print_vector(arr, my_n);
        //print_vector(pivots, p - 1);

        exchange_values(arr, p_pivots, p, n);

        MPI_Finalize();
        return EXIT_SUCCESS;
}
