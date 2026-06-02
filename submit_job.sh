#!/bin/bash
#SBATCH --job-name=cineca_test_pipeline
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=00:05:00
#SBATCH --partition=g100_all_serial
#SBATCH --account=tra26_TRNPLM
#SBATCH --output=simple_test_output.log

echo "=========================================="
echo "SUCCESS: The Slurm batch script executed!"
echo "Current Node: $(hostname)"
echo "Current Time: $(date)"
echo "=========================================="
