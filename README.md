# oaisetup in full virtualized environment
Consists of two steps; (a) gNodeb and (b)core

## gNodeb Setup
* Download https://releases.ubuntu.com/16.04/  64bit
* Create VM with the image
* Inside VM install openssh server, git
```
sudo apt update; sudo apt install openssh-server git cmake
```
* Use the following commands to compile the gNodeB source
```
mkdir ~/GIT
cd GIT
#git clone -b develop https://gitlab.eurecom.fr/oai/openairinterface5g.git
#git branch -a |grep "*"  # Check branch

git clone https://github.com/subhrendu1987/oaisetup

sudo apt-get install linux-image-4.15.0.142-lowlatency linux-headers-4.15.0.142-lowlatency
cd oaisetup/BBU/openairinterface5g/
source oaienv
cd cmake_targets/
./build_oai -I --eNB -x --install-system-files -w USRP
./build_oai --gNB -x -w USRP
cd ran_build/build
sudo ./nr-softmodem -E --sa -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --continuous -tx
```
* Debug
 * check detected USRP `uhd_find_devices`
 * Check USB version used `lsusb -v |grep USB` or `lsusb -D /dev/bus/usb/<busno>/<devno>`



## core Setup
* Download https://releases.ubuntu.com/18.04/  64bit
   * Testing Ubuntu 20.04LTS 64bit
* Create VM with the image
* Inside VM install openssh server, git, docker
```
sudo apt update
sudo apt install openssh-server git docker
# INSTALL DEPENDENCIES
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin docker-compose
sudo apt install docker.io docker-compose


```
* Initial configuration pulling
```
mkdir ~/GIT
cd GIT/
git clone --branch v1.4.0 https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed.git

git clone https://github.com/subhrendu1987/oaisetup
cd oaisetup/Util/
sudo bash pullimgs.sh
sudo bash imgsTag.sh
sudo bash oaisetup/Util/pullimgs.sh
sudo bash oaisetup/Util/imgsTag.sh
```
* VM configurations
```
sudo apt install inetutils-tools
sudo apt install net-tools
# UPDATE ROUTING TABLE
sudo -s
sysctl net.ipv4.conf.all.forwarding=1
echo "net.ipv4.ip_forward=1           # For IPv4" >> /etc/sysctl.conf
echo "net.ipv6.conf.all.forwarding=1  # For IPv6" >> /etc/sysctl.conf
exit
sudo sysctl -p /etc/sysctl.conf
sudo iptables -P FORWARD ACCEPT
```
* VNF configurations
```
cd oai-cn5g-fed
git checkout -f v1.4.0
cd ~/GIT/oaisetup/CORE/oai-cn5g-fed
./scripts/syncComponents.sh
./scripts/syncComponents.sh --nrf-branch v1.4.0 --amf-branch v1.4.0 \
                              --smf-branch v1.4.0 --spgwu-tiny-branch v1.4.0 \
                              --udr-branch v1.4.0 --udm-branch v1.4.0 \
                              --ausf-branch v1.4.0 --upf-vpp-branch v1.4.0 \
                              --nssf-branch v1.4.0
```
* Start VNFs
```
#sudo docker network create   --driver=bridge   --subnet=192.168.70.128/26   -o "com.docker.network.bridge.name"="demo-oai"   demo-oai-public-net

cd docker-compose
sudo python3 core-network.py --type start-basic
sudo docker-compose -f docker-compose-basic-nrf.yaml up -d
```
