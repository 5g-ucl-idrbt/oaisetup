# ETTUS USRP N310:
## Physical conection:
## Connect jtag and use stream to log into the system
* `sudo screen /dev/serial/by-id/usb-Silicon_Labs_CP2105_Dual_USB_to_UART_Bridge_Controller_007F6CB5-if01-port0 115200` Using `root`

## Connect RJ45 to management port
Use `ifconfig` to see the IP address of `eth0`. 

00:80:2f:33:c5:40

# Configure with Ubuntu 18.04

## Build libuhd3.15.LTS from source
* `mkdir GIT && cd GIT`
* `git clone https://github.com/EttusResearch/liberio` Dependency for UHD
	- `cd liberio`
	- Find dependencies with `apt search libudev` and install suitable versions. (e.g. `libudev-dev`, `libudev1`, etc. 
	- `autoreconf -i`
	- `./configure --prefix=/usr`
	- `make && make install`
* `sudo apt search dpdk=17.11; sudo apt-get install dpdk-dev` DPDK Version 17.11 is a dependency for UHD
* `sudo apt install libuhd3.15.0` Dependency for UHD
* `git clone -b UHD-3.15.LTS https://github.com/EttusResearch/uhd`
	- `cd host; mkdir -p build; cd build`
	- `cmake ../`
	- `sudo make`
	- `sudo make install`
* [https://github.com/subhrendu1987/oaisetup ](Continue GNB compilation using `cd ~/GIT/oaisetup-main/GNB/openairinterface5g/cmake_targets && ./build_oai -I --gNB -x -w USRP`)

