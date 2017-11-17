
// Part 3: Speeding up the computation by using Cache

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

						   // Declaring variables and array for computation of prime by each process
	int64_t p_low;
	int64_t p_high;
	int64_t p_size;
	int64_t p_first;
	char *p_marked;

	// Declaring variables for Cache optimization
	int64_t cache_size = 2000000;  // We use roughly 2 MB of cache)
	int64_t loop_start = 0, loop_end;
	int64_t new_size;
	int64_t low_old;
	int64_t high_old;

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

	proc0_size = ((n - 2) / (2 * p));
	low_value = 3 + BLOCK_LOW(id, p, n - 2) + BLOCK_LOW(id, p, n - 2) % 2;
	high_value = 3 + BLOCK_HIGH(id, p, n - 2) - BLOCK_HIGH(id, p, n - 2) % 2;
	size = (high_value - low_value) / 2 + 1;

	// Abort MPI 

	if ((3 + proc0_size) < (int)sqrt((double)n)) {
		if (!id) printf("Too many processes\n");
		MPI_Finalize();
		exit(1);
	}

	// Duplicating the prime finding process for each process
	p_low = 3;
	p_high = (int)sqrt((double)n);
	p_size = (p_high - p_low) / 2 + 1;

	p_marked = (char*)malloc(p_size);
	if (p_marked == NULL)
	{
		printf("Cannot allocate enough memory for prime array\n");
		MPI_Finalize();
		exit(1);
	}

	for (i = 0; i<p_size; i++)
		p_marked[i] = 0;

	//Find prime number on each processor
	index = 0;
	prime = 3;
	do {
		p_first = (prime*prime - p_low) / 2;
		for (i = p_first; i < p_size; i += prime)
			p_marked[i] = 1;
		while (p_marked[++index]);
		prime = 2 * index + 3;

	} while (prime*prime <= n);

	/* Allocate a process's share of the array in marked array. */

	marked = (char *)malloc(cache_size);

	if (marked == NULL) {
		printf("Cannot allocate enough memory\n");
		MPI_Finalize();
		exit(1);
	}

	low_old = low_value;
	high_old = high_value;
	loop_start = 0;

	loop_end = (size%cache_size == 0) ? (size / cache_size) : (size / cache_size + 1);

	do
	{
		low_value = ((low_old)+(loop_start*cache_size * 2));
		high_value = MIN(high_old, (low_value + (2 * cache_size - 2)));
		new_size = (high_value - low_value) / 2 + 1;

		for (i = 0; i < new_size; i++) marked[i] = 0;

		// Index will be zero initially for all processes 
		index = 0;

		// Start with prime number 3

		prime = 3;

		do
		{

			if (p_marked[index] == 0)
			{
				prime = 2 * index + 3;

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


				for (i = first; i<new_size; i += prime)
					marked[i] = 1;

			}
			index++;
		} while (index < (((p_high - 3) / 2) + 1));


		// Number of primes in marked array
		for (i = 0; i<new_size; i++)
			if (!marked[i])
				count++;

		loop_start++;
	} while (loop_start<loop_end);

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
