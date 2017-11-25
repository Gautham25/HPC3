#include "mpi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MIN(a,b)  ((a)<(b)?(a):(b))

// Defining MPI.h

#define BLOCK_LOW(id, p, n) ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id)+1, p,n)-1)
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id)+1,p,n)-BLOCK_LOW((id),p,n))
#define BLOCK_OWNER(index,p,n) ((((p) * index)+1)-1)/(n)

int main(int argc, char *argv[])
{
    double elapsed_time;
    int id, index,p,count,nodes;
    unsigned long long int n,k,low_value, high_value, size, proc0_size,i,prime,first;
    char *marked;
    unsigned long long int global_count;
    unsigned long long int localLowSieve,localHighSieve,localSizeSieve,localFirstSieve;
    char *localMarked;
    unsigned long long int cacheSize, cacheStart, cacheEnd, cacheElements, cacheLow, cacheHigh;
    cacheSize = 2000000;//cache size
    cacheStart=0;
    //Declaration of variables

    nodes = atoi(argv[2]);
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

    localLowSieve = 3;
    localHighSieve = (unsigned long long int)sqrt(n);
    localSizeSieve = (localHighSieve - localLowSieve) / 2 + 1;
    localMarked = (char*)malloc(localSizeSieve);

    if (localMarked == NULL) {
        printf("Cannot allocate memory to local array for seiving primes\n");
        MPI_Finalize();
        exit(1);
    }

    for(k=0;k<localSizeSieve;k++){
        localMarked[k] = 0;
    }

    if ((3 + proc0_size) < (int) sqrt((double) n)) {  //number of process should be less that square root of number of elements
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
        localFirstSieve = (prime*prime - localLowSieve) / 2;
        for(i=localFirstSieve;i<localSizeSieve;i+=prime){
            localMarked[i] = 1;
        }
        while(localMarked[++index]);
        prime = 2 * index + 3;
    }while(prime*prime <= n);

    cacheLow = low_value;
    cacheHigh = high_value;
    cacheStart = 0;
    count = 0;
    if(size%cacheSize == 0){
        cacheEnd = size/cacheSize;
    }
    else{
        cacheEnd = (size/cacheSize)+1;
    }

    do{
        low_value = ((cacheLow)+(cacheStart * cacheSize * 2));
        high_value = MIN(cacheHigh, (low_value + (2*cacheSize -2)));
        cacheElements = (high_value - low_value) / 2 + 1;

        for(k=0;k<cacheElements;k++){
            marked[k] = 0;
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
                for (i = first; i < cacheElements; i += prime){
                    marked[i] = 1;
                }
            }
            index++;
        }while(index < (((localHighSieve - 3) / 2) + 1));

        for (i = 0; i < cacheElements; i++){
            if (!marked[i]){
                count++;
            }
        }
        cacheStart++;
    }while (cacheStart < cacheEnd);

    if(p>1){
        MPI_Reduce (&count, &global_count, 1, MPI_INT, MPI_SUM,0, MPI_COMM_WORLD);
    }
    elapsed_time += MPI_Wtime();
    if (!id) {
        global_count++;
        printf("Total number of primes: %llu, Total time: %10.6f sec, number of nodes:%s\n",global_count,elapsed_time,argv[2]);
    }
    MPI_Finalize();
    return 0;
}
