#!/bin/bash
#SBATCH --job-name=astralog
#SBATCH --partition=g100_usr_dbg
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=2
#SBATCH --time=00:05:00
#SBATCH --output=/logs/astralog_%j.out
#SBATCH --error=/logs/astralog_%j.err

# ---------------------------------------------------------------
# On the actual Galileo100 cluster:
# module load singularity
# singularity exec astralog.sif /opt/astralog/build/astralog
# ---------------------------------------------------------------
module load singularity
singularity exec $HOME/containers/astralog.siff /opt/astralog/build/astralog