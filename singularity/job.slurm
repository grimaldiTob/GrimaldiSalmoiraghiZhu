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

# Job-specific directories
BASE_DIR="$HOME/logs/astralog"
OUTPUT_DIR="$BASE_DIR/${SLURM_JOB_ID}"

mkdir -p "$OUTPUT_DIR"

echo "Job ID: $SLURM_JOB_ID"
echo "Output dir: $OUTPUT_DIR"
echo "Running on: $(hostname)"


# Singularity image 
IMAGE="$HOME/containers/astralog.sif"

if [ ! -f "$IMAGE" ]; then
    echo "ERROR: Singularity image not found: $IMAGE"
    exit 1
fi


# Run container with output directory bound
singularity exec \
    --bind "$OUTPUT_DIR:/opt/astralog/output" \ 
    "$IMAGE" \
    /opt/astralog/build/astralog