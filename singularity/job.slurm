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

# Image path
IMAGE="$HOME/containers/astralog.sif"

# Per-job directories (logs and astralog outputs)
LOG_DIR="$HOME/logs/${SLURM_JOB_ID}"
OUTDIR="$HOME/logs/astralog/${SLURM_JOB_ID}"

mkdir -p "$LOG_DIR" "$OUTDIR"

# Run from $HOME inside the container. Bind $HOME so the container can access
# your working tree, and bind the output directory so files written to
# /opt/astralog/output inside the container appear on the host.
singularity exec \
  --bind "$HOME":"$HOME" \
  --bind "$OUTDIR":/opt/astralog/output \
  --pwd "$HOME" \
  "$IMAGE" \
  bash -lc "./build/astralog --csv --input ./input" >"$LOG_DIR/astralog.stdout" 2>"$LOG_DIR/astralog.stderr"