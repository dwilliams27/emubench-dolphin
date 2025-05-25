gcloud compute ssh test-vm --zone=us-central1-a

sudo apt-get update
sudo apt-get install -y apt-transport-https ca-certificates curl software-properties-common

curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo "deb [arch=arm64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io

sudo usermod -aG docker $USER
sudo gcloud auth login
sudo gcloud auth configure-docker

sudo docker pull $1
sudo docker run -it --entrypoint="/bin/bash" $1

apt-get update && apt-get install -y gdb xvfb
Xvfb :99 -screen 0 1280x720x24 & export DISPLAY=:99

gdb --args ./dolphin-emu-nogui -e /games/hmoon.rvz
