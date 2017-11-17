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
#define BLOCK_OWNER(index,p,n) ((((p) * index)+1)-1)/(n)

int main(int argc, char *argv[])
{
    //int *arrA = (int*)calloc(pow(10,10),sizeof(int));
    double elapsed_time;
    int id,index,p,count, nodes;
    unsigned long long int n;
    unsigned long long int k;
    unsigned long long int low_value;
    unsigned long long int high_value;
    unsigned long long int size;
    unsigned long long int proc0_size;
    unsigned long long int i;
    unsigned long long int prime;
    unsigned long long int first;
    char *marked;
    unsigned long long int global_count;
    unsigned long long int localLow;
    unsigned long long int localHigh; 
    unsigned long long int localSize;
    unsigned long long int localFirst;
    char *localMarked;
    unsigned long long int cacheSize = 2000000;
    unsigned long long int cStart = 0;
    unsigned long long int cEnd;
    unsigned long long int cSize;
    unsigned long long int cLow;
    unsigned long long int cHigh;
    //variable declaration

    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    elapsed_time = -MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    if (argc != 3) {
          if (!id) printf ("Command line: %s <m>\n", argv[0]);
          MPI_Finalize(); exit(1);
    }
    n = atoll(argv[1]);
    low_value = 3 + BLOCK_LOW(id,p,n-2) + BLOCK_LOW(id,p,n-2) % 2;
    high_value = 3 + BLOCK_HIGH(id,p,n-2) - BLOCK_HIGH(id,p,n-2) % 2;
    size = (high_value - low_value) / 2 + 1;
    proc0_size = ((n-2)/(2*p));

    localLow = 3;
    localHigh = 3 + BLOCK_HIGH(0,p,n-2) - BLOCK_HIGH(0,p,n-2) % 2;
    localSize = (localHigh - localLow) / 2 + 1;
    localMarked = (char*)malloc(localSize);

    if (localMarked == NULL) {
        printf("Cannot allocate memory to local array for seiving primes\n");
        MPI_Finalize();
        exit(1);
    }

    for(k=0;k<localSize;k++){
        localMarked[k] = 0;
    }

    if ((3 + proc0_size) < (int) sqrt((double) n)) {
        if (!id) printf ("Too many processes\n");
        MPI_Finalize();
        exit(1);
    }

    marked = (char *) malloc (size);
    if (marked == NULL) {
        printf("Cannot allocate enough memory\n");
        MPI_Finalize();
        exit(1);
    }
    for (i = 0; i < size; i++){
        marked[i] = 0;
    }

    index = 0;
    prime = 3;
    do{
        localFirst = (prime*prime - localLow) / 2;
        for(i=localFirst;i<localSize;i+=prime){
            localMarked[i] = 1;
        }
        while(localMarked[++index]);
        prime = 2 * index + 3;
    }while(prime*prime <= n);

    cLow = low_value;
    cHigh = high_value;
    cStart = 0;
    count = 0;
    cEnd = (size%cacheSize == 0) ? (size/cacheSize) : (size/cacheSize + 1);
    // if(size%cacheSize == 0){
    //     cEnd = size/cacheSize;
    // }
    // else{
    //     cEnd = (size/cacheSize)+1;
    // }

    do{
        low_value = ((cLow)+(cStart * cacheSize * 2));
        high_value = MIN(cHigh, (low_value + (2*cacheSize -2)));
        cSize = (high_value - low_value) / 2 + 1;

        for(i=0;i<cSize;i++){
            marked[i] = 0;
        }

        index = 0;
        prime = 3;
        do{
            if(localMarked[index] == 0){
                prime = 2 * index + 3;

                if (prime * prime > low_value){
                    first = (prime * prime - low_value)/2;
                }
                else{
                    if (!(low_value % prime)){
                        first = 0;
                    }
                    else{
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
                for (i = first; i < cSize; i += prime){
                    marked[i] = 1;
                }
            }
            index++;
        }while(index < (((localHigh - 3) / 2) + 1));

        for (i = 0; i < cSize; i++){
            if (!marked[i]){
                count++;
            }
        }
        cStart++;
    }while (cStart < cEnd);


    if(p>1){
        MPI_Reduce (&count, &global_count, 1, MPI_INT, MPI_SUM,0, MPI_COMM_WORLD);
    }
    elapsed_time += MPI_Wtime();
    if (!id) {
        global_count++;
        printf("Total number of primes: %llu, Total time: %10.6f sec, Total nodes: %s\n",global_count,elapsed_time,argv[2]);
        // printf ("Total elapsed time: %10.6f\n", elapsed_time);
    }
    MPI_Finalize();
    return 0;
}
