#!/bin/bash
#SBATCH --job-name=astralog-poller
#SBATCH --time=00:10:00
#SBATCH --partition=g100_all_serial
#SBATCH --output=%h/logs/poller_%j.out
#SBATCH --error=%h/logs/poller_%j.err

# Schedule the next check in 10 minutes (600 seconds)
sbatch --begin=now+600 $HOME/bin/poller.slurm

echo "Poller called at $(date)"

# Run the actual deployment script using its absolute path
$HOME/bin/poll_and_deploy.sh || echo "Deploy script failed"
