Run the following commands and save a Personal Access Token on Github

```bash
mkdir -p ~/bin ~/logs ~/.secrets
chmod 700 ~/.secrets

echo "github_pat_<YOUR-KEY>" > ~/.secrets/github_pat
chmod 600 ~/.secrets/github_pat
```

Create the poll_and_deploy.sh script

```bash
mkdir -p ./bin
cd ~/bin
touch poll_and_deploy.sh
nano poll_and_deploy.sh # then paste the content of poll_and_deploy.sh
chmod +x ~/bin/poll_and_deploy.sh
```

Create the poller

```bash
# in bin directory
touch poller.slurm
nano poller.slurm # copy the content
sbatch poller.slurm
```

Clone the full directory on github

```bash
git clone https://grimaldiTob:$(cat ~/.secrets/github_pat)@github.com/grimaldiTob/GrimaldiSalmoiraghiZhu.git astralog
```
