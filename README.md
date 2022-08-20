# oaisetup in full virtualized environment
Consists of two steps; (a) gNodeb and (b)core

## gNodeb Setup
* Download https://releases.ubuntu.com/16.04/  64bit
* Create VM with the image
* Inside VM install openssh server, git
'''
sudo apt update; sudo apt install openssh-server git
'''
* Use the following commands
'''
mkdir ~/GIT
cd GIT
git clone -b develop https://gitlab.eurecom.fr/oai/openairinterface5g.git
cd openairinterface5g
'''

## core Setup
* Download https://releases.ubuntu.com/18.04/  64bit
* Create VM with the image
* Inside VM install openssh server, git
'''
sudo apt update; sudo apt install openssh-server git
'''