#!/bin/bash

# Configuration
REPO="grimaldiTob/GrimaldiSalmoiraghiZhu"
BRANCH="dev" # just for testing, ideally this should be "main"
CHECK_INTERVAL=60 # Check every 60 seconds what the status of the branch is
TOKEN = $GITHUB_TOKEN # GitHub Personal Access Token with repo access (configured as a variable on Cineca)
if [ -z "$TOKEN" ]; then
    echo "Error: GITHUB_API_TOKEN environment variable is not set."
    exit 1
fi

echo "Galileo CI/CD Listener started (CI Status Aware). Watching branch: $BRANCH..."

while true; do # Let's hope they do not ban us...
    # Fetch latest remote tracking branch without merging
    git fetch origin $BRANCH &> /dev/null
    
    LOCAL_HASH=$(git rev-parse HEAD)
    REMOTE_HASH=$(git rev-parse origin/$BRANCH)
    
    if [ "$LOCAL_HASH" != "$REMOTE_HASH" ]; then
        echo "------------------------------------------------"
        echo "New commit detected: $REMOTE_HASH"
        echo "Checking GitHub Actions CI status..."
        
        # Query the GitHub API for the combined status of this specific commit
        API_URL="https://api.github.com/repos/$REPO/commits/$REMOTE_HASH/status"
        STATUS=$(curl -s -H "Authorization: token $TOKEN" "$API_URL" | jq -r '.state')
        
        if [ "$STATUS" = "success" ]; then
            echo "GitHub Actions CI passed!"
            echo "Pulling latest changes..."
            git pull origin $BRANCH
            
            echo "Submitting deployment job to Slurm..."
            JOB_ID=$(sbatch script.sh | awk '{print $4}')
            echo "Job $JOB_ID submitted to Galileo cluster successfully."
            echo "------------------------------------------------"
        elif [ "$STATUS" = "failure" ]; then
            echo "GitHub Actions CI failed on GitHub. Skipping deployment."
            # Update local tracking hash anyway so no loop on the failed commit
            git merge origin/$BRANCH &> /dev/null
            echo "Advanced pointer past failed commit."
            echo "------------------------------------------------"
        else
            # Status might be "pending" while GitHub Actions is still running its tests
            echo "GitHub Actions CI is still '$STATUS'. Waiting for completion..."
            echo "------------------------------------------------"
        fi
    fi
    
    sleep $CHECK_INTERVAL
done