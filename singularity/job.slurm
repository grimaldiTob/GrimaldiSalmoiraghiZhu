#!/bin/bash
#SBATCH --job-name=astralog
#SBATCH --partition=debug
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=2
#SBATCH --time=00:05:00
#SBATCH --output=singularity/astralog_%j.out
#SBATCH --error=singularity/astralog_%j.err

# ---------------------------------------------------------------
# On the actual Galileo100 cluster:
# module load singularity
# singularity exec astralog.sif /opt/astralog/build/astralog
# ---------------------------------------------------------------

singularity exec astralog.sif /opt/astralog/build/astralog