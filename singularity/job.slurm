#!/bin/bash
#SBATCH --job-name=astralog
#SBATCH --partition=g100_usr_dbg
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=2
#SBATCH --time=00:05:00
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

module load singularity

IMAGE="$HOME/containers/astralog.sif"
OUTDIR="$HOME/logs/astralog/${SLURM_JOB_ID}"

mkdir -p "$OUTDIR"

singularity exec \
  --pwd /opt/astralog \
  --bind "$OUTDIR:/opt/astralog/collector_output" \
  "$IMAGE"