#1/bin/sh

module purge
module load gcc-4.6.2
module load mvapich2-1.9a2/gnu-4.6.2

mpicc -o HPC3_p1 HPC_p1.c -lm
mpicc -o HPC3_p2 HPC_p2.c -lm

jobid=$(qsub jobfile_part1_node1|cut -d '.' -f 1)
qsub -W depend=afterok:{jobid} jobfile_part1_node2

jobid=$(qsub jobfile_part1_node2|cut -d '.' -f 1)
qsub -W depend=afterok:{jobid} jobfile_part1_node4

jobid=$(qsub jobfile_part1_node4|cut -d '.' -f 1)
qsub -W depend=afterok:{jobid} jobfile_part1_node8

