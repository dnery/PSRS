#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

/* Parallel API's */
#include <mpi.h>
#include <omp.h>

/* Local includes */
#include "utils.h"

void exchange_values(int *arr, int *pivots, int nproc, int nelem)
{
        int start, my_start;    /* start index, local processor end index */
        int end, my_end;        /* end index, local processor end index */
        int isublist;           /* local elem subarray iterator */
        int iproc;              /* processor iterator */

        /* my process rank */
        int my_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        /* Exchange: send between processes */
        for (isublist = 0, iproc = 0; iproc < nproc; iproc++) {

                start = isublist;

                if (iproc < nproc - 1) {        /* before the last pivot */

                        while (arr[isublist] <= pivots[iproc] &&
                                        isublist < nelem)
                                isublist++;

                        end = isublist;
                } else {                        /* on the last pivot */
                        end = nelem;
                }

                if (iproc == my_rank) {
                        my_start = start;
                        my_end = end;
                } else {
                        int *sendbuf = arr + start;
                        int nsendbuf = end - start;

                        MPI_Send(&nsendbuf, 1, MPI_INT, iproc, my_rank, MPI_COMM_WORLD);
                        MPI_Send(sendbuf, nsendbuf, MPI_INT, iproc, my_rank, MPI_COMM_WORLD);
                }
        }

        /* MPI parent intercomm */
        MPI_Comm parent;
        MPI_Comm_get_parent(&parent);

        /* Exchange: send between processes */
        for (iproc = 0; iproc < nproc; iproc++) {

                if (iproc == my_rank) {
                        int *sendbuf = arr + my_start;
                        int nsendbuf = my_end - my_start;

                        /* just send my data if I own it... */
                        MPI_Send(&nsendbuf, 1, MPI_INT, 0, my_rank, parent);
                        MPI_Send(sendbuf, nsendbuf, MPI_INT, 0, my_rank, parent);
                } else {
                        int nrecbuf;
                        MPI_Recv(&nrecbuf, 1, MPI_INT, iproc, iproc, MPI_COMM_WORLD, NULL);

                        autofree int *recbuf = malloc(nrecbuf * sizeof(*recbuf));
                        MPI_Recv(recbuf, nrecbuf, MPI_INT, iproc, iproc, MPI_COMM_WORLD, NULL);

                        /* ...otherwise receive and just send */
                        MPI_Send(&nrecbuf, 1, MPI_INT, 0, my_rank, parent);
                        MPI_Send(recbuf, nrecbuf, MPI_INT, 0, my_rank, parent);
                }

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
        int nelem = char_to_int(argv[1]);
        int nproc = char_to_int(argv[2]);

        /* ready data */
        int my_nproc = block_size(my_rank, nproc, nelem);
        autofree int *arr = malloc(my_nproc * sizeof(*arr));

        /* Receive array */
        MPI_Status status;
        MPI_Recv(arr, my_nproc, MPI_INT, 0, 0, parent, &status);

        /* Receive pivot */
        autofree int *pivots = malloc((nproc - 1) * sizeof(*pivots));
        MPI_Bcast(pivots, nproc - 1, MPI_INT, 0, parent);

        exchange_values(arr, pivots, nproc, my_nproc);

        MPI_Finalize();
        return EXIT_SUCCESS;
}
