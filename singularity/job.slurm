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

# Create a job-specific host output directory using the SLURM job ID.
OUTPUT_DIR=/logs/astralog_${SLURM_JOB_ID}
mkdir -p "$OUTPUT_DIR"

# Bind the host directory to the container output directory.
# Files written to /opt/astralog/output inside the container will appear there.
singularity exec --bind "$OUTPUT_DIR":/opt/astralog/output \
    $HOME/containers/astralog.sif \
    /opt/astralog/build/astralog