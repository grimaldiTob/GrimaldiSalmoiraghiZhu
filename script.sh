#!/usr/bin/env bash
#SBATCH --job-name=slurm-check
#SBATCH --output=slurm-check-%j.out
#SBATCH --error=slurm-check-%j.err
#SBATCH --time=00:01:00

set -euo pipefail

echo "SLURM job ${SLURM_JOB_ID:-unknown} ran successfully on $(hostname)"
