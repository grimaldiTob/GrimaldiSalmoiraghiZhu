#!/bin/bash
# /bin/poll_and_deploy.sh
set -euo pipefail

# ── Config ──────────────────────────────────────────────
GITHUB_REPO="grimaldiTob/GrimaldiSalmoiraghiZhu"
GITHUB_TOKEN=$(cat "$HOME/.secrets/github_pat")
SIF_DEST="$HOME/containers/astralog.sif"
LAST_SHA_FILE="$HOME/.astralog_last_sha"
SLURM_SCRIPT="$HOME/astralog/singularity/job.slurm"
LOG="$HOME/logs/deploy.log"
LOCKFILE="/tmp/astralog_deploy.lock"
# ────────────────────────────────────────────────────────

mkdir -p "$(dirname "$SIF_DEST")" "$(dirname "$LOG")" # creates containers and logs dirs
exec >> "$LOG" 2>&1 # redirect std out and err to the log file

echo "=== $(date -Iseconds) ==="

# Lock to avoid paralel execution
exec 9>"$LOCKFILE"
flock -n 9 || {
  echo "Another deploy is already running, skip."
  exit 0
}

# THIS SHOULD BE THE MAIN PROBLEM --> no SHA is returned 
REMOTE_SHA=$(curl -sf \
  --max-time 30 \
  -H "Authorization: token $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  "https://api.github.com/repos/${GITHUB_REPO}/releases/tags/latest-sif" \
  -o /tmp/release_info.json && \
  python3 -c "
import json
with open('/tmp/release_info.json') as f:
    d = json.load(f)
print(d['assets'][0]['digest'])
" || echo "") # checks if the api response by github (which we get) si different from the one recevied previously
# atm having problem with this python lines of code since galileo uses an old stable version (3.6)

echo "Remote SHA: $REMOTE_SHA"

if [[ -z "$REMOTE_SHA" ]]; then
  echo "No release found on GitHub, skip."
  exit 0
fi

LAST_SHA=$(cat "$LAST_SHA_FILE" 2>/dev/null || echo "")

if [[ "$REMOTE_SHA" == "$LAST_SHA" ]]; then
  echo "No changes ($REMOTE_SHA), skip."
  exit 0
fi

echo "New deploy found: $REMOTE_SHA (previous: ${LAST_SHA:-none})"

# request of download (it should work)
echo "Downloading astralog.sif..."
curl -fL \
  --max-time 300 \
  --retry 3 \
  --retry-delay 10 \
  -H "Authorization: token $GITHUB_TOKEN" \
  -H "Accept: application/octet-stream" \
  -o "${SIF_DEST}.tmp" \
  "https://github.com/${GITHUB_REPO}/releases/download/latest-sif/astralog.sif"

mv "${SIF_DEST}.tmp" "$SIF_DEST"
echo "Download completed: $(du -sh "$SIF_DEST" | cut -f1)"

# fethc the original github folder
cd "$HOME/astralog"
git fetch --quiet origin main
git reset --hard origin/main --quiet

JOB_ID=$(sbatch "$SLURM_SCRIPT" | awk '{print $NF}')
echo "Job submitted: $JOB_ID"

# 5. Salva il SHA
echo "$REMOTE_SHA" > "$LAST_SHA_FILE"
echo "Deploy completed."
