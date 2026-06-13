# Configuring the automatic deployment on CINECA Galileo100 cluster

## 1. Obtain a Personal Access Token from Github

To create a GitHub Personal Access Token (PAT), navigate to **Settings** → **Developer settings** → **Personal access tokens** → **Fine-grained tokens**, click **Generate new token**, select the target account as the resource owner, leave Repository access set to Only select repositories without selecting any repository (or No repositories if available), do not grant any User permissions or Repository permissions, and generate the token. Save the provided token, since it won't be showned again.

## 2. Saving the Personal Access Token on Galileo100

On Galielo100, run the following commands from the `$HOME` directory to save the Personal Access Token generated on Github:

```bash
mkdir -p ~/bin ~/logs ~/.secrets
chmod 700 ~/.secrets

echo "github_pat_<YOUR-KEY>" > ~/.secrets/github_pat
chmod 600 ~/.secrets/github_pat
```

## 3. Creating the poller script

Create the [poll_and_deploy.sh](poll_and_deploy.sh) script:

```bash
mkdir -p ./bin
cd ~/bin
touch poll_and_deploy.sh
nano poll_and_deploy.sh # then paste the content of poll_and_deploy.sh
chmod +x ~/bin/poll_and_deploy.sh
```

## 4. Create the poller job

Create the [poller script](poller.slurm) and submit it to SLURM:

```bash
# in bin directory
touch poller.slurm
nano poller.slurm # copy the content from poller.slurm
sbatch poller.slurm
```

## 5. Clone the directory

Back in the `$HOME` directory, clone the full GitHub project:

```bash
cd $HOME
git clone https://grimaldiTob:$(cat ~/.secrets/github_pat)@github.com/grimaldiTob/GrimaldiSalmoiraghiZhu.git astralog
```
