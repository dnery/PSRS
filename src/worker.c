#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"

void exchange_values(int *arr, int *pivots, int p, int n)
{
        int start, my_start;    /**/
        int end, my_end;        /**/
        int iproc;              /**/
        int isublist = 0;       /**/

        /* my process rank */
        int my_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* Exchange: send between processes */
        for (iproc = 0; iproc < p; iproc++) {

                start = isublist;
                //printf("%d: has start %d\n", my_rank, start);

                if (iproc == p - 1) {
                        end = n;
                        //printf("%d: lastone: has end %d\n", my_rank, end);
                } else {
                        while (arr[isublist] <= pivots[iproc] && isublist < n)
                                isublist++;
                        end = isublist;
                        //printf("%d: has end %d\n", my_rank, end);
                }

                if (iproc == my_rank) {

                        my_start = start;
                        my_end = end;

                        //printf("%d: myself, start %d, end %d\n", my_rank, my_start, my_end);

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
        for (iproc = 0; iproc < p; iproc++) {

                int *sendbuf;
                int nsendbuf;

                if (iproc == my_rank) {

                        sendbuf = arr + my_start;
                        nsendbuf = my_end - my_start;

                } else {

                        int nrecbuf;
                        MPI_Recv(&nrecbuf, 1, MPI_INT, iproc, iproc, MPI_COMM_WORLD, NULL);

                        int *recbuf = malloc(nrecbuf * sizeof(*recbuf));
                        MPI_Recv(recbuf, nrecbuf, MPI_INT, iproc, iproc, MPI_COMM_WORLD, NULL);

                        sendbuf = recbuf;
                        nsendbuf = nrecbuf;
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

        /* status */
        //printf("%d: pivs:\n", my_rank);
        //print_vector(pivots, p - 1);

        exchange_values(arr, pivots, p, my_n);

        MPI_Finalize();
        return EXIT_SUCCESS;
}
