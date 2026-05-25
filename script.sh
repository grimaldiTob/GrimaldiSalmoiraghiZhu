#!/usr/bin/env bash
module load singularity
srun -N 1 -n 4 singularity exec -mpi astralog.sif /opt/astralog/build/astralog