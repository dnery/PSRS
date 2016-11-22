/* manager program */
#include<stdlib.h>
#include<limits.h>
#include<stdio.h>
#include<time.h>

/* Parallelism */
#include<mpi.h>
#include<omp.h>

/* indexing macros */
#define block_lo(id, p, n) ((id) * (n) / (p))
#define block_hi(id, p, n) (block_lo((id) + 1, p, n) - 1)
#define block_size(id, p, n) (block_lo((id) + 1, p, n) - block_lo(id, p, n))
#define block_owner(idx, p, n) (((p) * ((idx) + 1) - 1) / (n))

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
 * Generate 'size' sized random vector
 */
int *random_ints(long size)
{
        int *base = malloc(size * sizeof(*base));

        /* populate array */
        long i;
        srand(time(NULL));
        for (i = 0; i < size; i++)
                base[i] = rand() % 100;

        return base;
}

/*
 * Print 'size' sized base vector
 */
void print_vector(int *base, long size)
{
        printf("\nparent:\n");

        long i;
        for (i = 0; i < size; i++) {
                printf("%d", base[i]);
                printf(i == size - 1 ? ".\n" : " ");
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
        MPI_Comm_spawn("./psrs", argv + 1, p, MPI_INFO_NULL, 0, MPI_COMM_SELF,
                        &workers, MPI_ERRCODES_IGNORE);

        /* status */
        print_vector(arr, n);

        long i;
        for (i = 0; i < p; i++) {
                int bufsize = block_size(i, p, n);
                int *sendbuf = arr + block_lo(i, p, n);
                MPI_Send(sendbuf, bufsize, MPI_INT, i, 0, workers);
        }

        free(arr);
        MPI_Finalize();
        return EXIT_SUCCESS;
}
