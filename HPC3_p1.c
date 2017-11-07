
// Part 1: Removal of all even numbers from 'n'

#include "mpi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MIN(a,b)  ((a)<(b)?(a):(b))

// Defining Block Low, Block High and Block Size

#define BLOCK_LOW(id, p, n) ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id)+1, p,n)-1)
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id)+1,p,n)-BLOCK_LOW((id),p,n))

int main(int argc, char *argv[])
{
	int    count;        /* Local prime count */
	double elapsed_time; /* Parallel execution time */
	int    global_count; /* Global prime count */
	int    id;           /* Process ID number */
	int    index;        /* Index of current prime */
	char  *marked;       /* Portion of 2,...,'n' */
	int    p;            /* Number of processes */
	int64_t  i;
	int64_t  proc0_size;   /* Size of proc 0's subarray */
	int64_t  prime;        /* Current prime */
	int64_t  size;         /* Elements in 'marked' */
	int64_t  n;            /* Sieving from 2, ..., 'n' */
	int64_t  first;        /* Index of first multiple */
	int64_t  high_value;   /* Highest value on this proc */
	int64_t  low_value;    /* Lowest value on this proc */

	MPI_Init(&argc, &argv);

	/* Start the timer */

	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Barrier(MPI_COMM_WORLD);
	elapsed_time = -MPI_Wtime();

	if (argc != 2) {
		if (!id) printf("Command line: %s <m>\n", argv[0]);
		MPI_Finalize();
		exit(1);
	}

	n = atol(argv[1]);

	/* Figure out this process's share of the array, as
	well as the integers represented by the first and
	last array elements */

	low_value = 3 + BLOCK_LOW(id, p, n - 2) + BLOCK_LOW(id, p, n - 2) % 2;
	high_value = 3 + BLOCK_HIGH(id, p, n - 2) - BLOCK_HIGH(id, p, n - 2) % 2;
	size = (high_value - low_value) / 2 + 1;

	/* Bail out if all the primes used for sieving are
	not all held by process 0 */

	proc0_size = ((n - 2) / (2 * p));

	if ((3 + proc0_size) < (int)sqrt((double)n)) {
		if (!id) printf("Too many processes\n");
		MPI_Finalize();
		exit(1);
	}

	/* Allocate this process's share of the array. */

	marked = (char *)malloc(size);

	if (marked == NULL) {
		printf("Cannot allocate enough memory\n");
		MPI_Finalize();
		exit(1);
	}

	for (i = 0; i < size; i++) marked[i] = 0;
	if (!id) index = 0;

	// Start with prime number 3

	prime = 3;
	do
	{
		if (prime*prime > low_value)
			first = (prime*prime - low_value) / 2;
		else
		{
			if (!(low_value%prime))
				first = 0;
			else
			{
				if ((low_value%prime) % 2 == 0)
				{
					first = prime - (low_value%prime) / 2;
				}
				else
				{
					first = (prime - (low_value%prime)) / 2;
				}
			}
		}


		for (i = first; i < size; i += prime) marked[i] = 1;
		if (!id) {
			while (marked[++index]);
			prime = 2 * index + 3;
		}

		if (p > 1) MPI_Bcast(&prime, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}

	while (prime * prime <= n);
	count = 0;
	for (i = 0; i < size; i++)
		if (!marked[i]) count++;
	if (p > 1) MPI_Reduce(&count, &global_count, 1, MPI_INT, MPI_SUM,
		0, MPI_COMM_WORLD);

	/* Stop the timer */

	elapsed_time += MPI_Wtime();


	/* Print the results */

	if (!id) {
		global_count++;
		printf("There are %d primes less than or equal to %ld\n", global_count, n);
		printf("Elapsed Time (Number of Processes: %d): %10.6f sec\n", p, elapsed_time);
	}
	MPI_Finalize();
	return 0;
}
