# oaisetup in full virtualized environment
Consists of two steps; (a) gNodeb and (b)core

## gNodeb Setup
* Download https://releases.ubuntu.com/16.04/  64bit
* Create VM with the image
* Inside VM install openssh server, git
```
sudo apt update; sudo apt install openssh-server git cmake
sudo apt update; sudo apt install openssh-server git cmake
```
* Use the following commands
```
mkdir ~/GIT
cd GIT
git clone -b develop https://gitlab.eurecom.fr/oai/openairinterface5g.git
cd openairinterface5g
git branch -a |grep "*"  # Check branch
source oaidev
cd cmake
./build_oai -I --eNB -x --install-system-files -w USRP
./build_oai --gNB -w USRP
cd ran_build/build
```

## core Setup
* Download https://releases.ubuntu.com/18.04/  64bit
* Create VM with the image
* Inside VM install openssh server, git, docker
```
sudo apt update
sudo apt install openssh-server git docker
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin docker-compose
```
* Initial configuration pulling
```
mkdir ~/GIT
cd GIT/
git clone --branch v1.4.0 https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed.git
git clone https://github.com/subhrendu1987/oaisetup
sudo bash oaisetup/Util/pullimgs.sh
sudo bash oaisetup/Util/imgsTag.sh
```
* VM configurations
```
sudo -S
sudo sysctl net.ipv4.conf.all.forwarding=1
echo "net.ipv4.ip_forward=1           # For IPv4" >> /etc/sysctl.conf
echo "net.ipv6.conf.all.forwarding=1  # For IPv6" >> /etc/sysctl.conf
exit
sudo sysctl -p /etc/sysctl.conf
```
* VNF configurations
```
cd oai-cn5g-fed
git checkout -f v1.4.0
./scripts/syncComponents.sh
./scripts/syncComponents.sh --nrf-branch v1.4.0 --amf-branch v1.4.0 \
                              --smf-branch v1.4.0 --spgwu-tiny-branch v1.4.0 \
                              --udr-branch v1.4.0 --udm-branch v1.4.0 \
                              --ausf-branch v1.4.0 --upf-vpp-branch v1.4.0 \
                              --nssf-branch v1.4.0

```
