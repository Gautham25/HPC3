#1/bin/sh

PBS_O_WORKDIR=/home/$USER/HPC3

cd $PBS_O_WORKDIR

mpicc -o HPC3_p1 HPC3_p1.c -lm
mpicc -o HPC3_p2 HPC3_p2.c -lm

jobid=$(qsub jobfile_part1_node1)
Second=$(qsub -W depend=afterany:${jobid} jobfile_part1_node2)
Third=$(qsub -W depend=afterany:${Second} jobfile_part1_node4)
Fourth=$(qsub -W depend=afterany:${Third} jobfile_part1_node8)

PFirst=$(qsub -W depend=afterany:${Fourth} jobfile_part2_node1)
PSecond=$(qsub -W depend=afterany:${PFirst} jobfile_part2_node2)
PThird=$(qsub -W depend=afterany:${PSecond} jobfile_part2_node4)
qsub -W depend=afterany:${PThird} jobfile_part2_node8
