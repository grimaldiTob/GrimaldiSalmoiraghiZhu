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

# Host input paths (originals). We'll copy these into a per-job temp input
# directory so the job can modify files without touching your originals.
HOST_INPUT_DIR="$HOME/astralog/input"
HOST_RULES_FILE="$HOME/astralog/rules.json"

# Per-job working input (copied from host)
WORK_INPUT_DIR="$OUTDIR/input"

mkdir -p "$LOG_DIR" "$OUTDIR" "$WORK_INPUT_DIR"

# Copy inputs and rules into the per-job input directory (preserve attrs).
if [ -d "$HOST_INPUT_DIR" ]; then
  cp -a "$HOST_INPUT_DIR/." "$WORK_INPUT_DIR/" 2>/dev/null || true
fi
if [ -f "$HOST_RULES_FILE" ]; then
  cp -a "$HOST_RULES_FILE" "$WORK_INPUT_DIR/" 2>/dev/null || true
fi

# Run from $HOME inside the container. Bind only the per-job input and output
# directories to avoid accidental writes to originals. Use absolute input
# path inside the container to avoid relative-path confusion.
singularity exec \
  --bind "$WORK_INPUT_DIR":/opt/astralog/input:rw \
  --bind "$OUTDIR":/opt/astralog/output:rw \
  --pwd "$HOME" \
  "$IMAGE" \
  /opt/astralog/build/astralog --csv --input /opt/astralog/input --rules /opt/astralog/input/rules.json >"$LOG_DIR/astralog.stdout" 2>"$LOG_DIR/astralog.stderr"

# Copy the output folder from $HOME/output to the per-job output directory
if [ -d "$HOME/output" ]; then
  mkdir -p "$OUTDIR/output"
  cp -a "$HOME/output/." "$OUTDIR/output/" 2>/dev/null || true
  rm -rf "$HOME/output"
fi