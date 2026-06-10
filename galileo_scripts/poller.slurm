#!/bin/bash
# ~/astralog/poller.slurm
#SBATCH --job-name=astralog-poller
#SBATCH --time=00:10:00
#SBATCH --partition=g100_usr_dbg
#SBATCH --output=/logs/poller_%j.out
#SBATCH --error=/logs/poller_%j.err

# this is the only way I found to run a script multiple times on cineca cluster
# also I check if there is already an active request to not populate the queue

sbatch --begin=now+600 ./astralog/poller.slurm
echo "Poller called"
./bin/poll_and_deploy.sh || echo "Deploy script failed"
