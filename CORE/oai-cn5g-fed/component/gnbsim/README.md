# gnbsim

gnbsim is a 5G SA gNB/UE simulator for testing 5G System.

This project is fork of `https://github.com/hhorai/gnbsim`.
It contains gnbsim tests for oai-5gcn.

## Build gnbsim docker image
```bash
docker build --tag gnbsim:develop --target gnbsim --file docker/Dockerfile.ubuntu.22.04 .
```
  #### Note:- To build on RHEL8
```bash
docker build --tag gnbsim:develop --target gnbsim --file docker/Dockerfile.gnbsim.rhel8 .
```
## Configure gnbsim

Edit test configuration in /example/example.json

## Build oai-cn5g docker images
* Build AMF and SMF
Build instructions are taken from -> [oai-cn5g-fed](https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed/-/blob/master/docs/BUILD_IMAGES.md) for building AMF, SMF, UPF (develop branch). <br/>
Below test is done with [vpp-upf (Travelping)](https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-upf-vpp/-/blob/vpp-upf/docs/BUILD_IMAGE.md) and SMF (vpp-upf branch)

```bash
$ git clone https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed.git
$ cd oai-cn5g-fed
$ git checkout master
$ git pull origin master
./scripts/syncComponents.sh --smf-branch vpp-upf
---------------------------------------------------------
OAI-AMF    component branch : develop
OAI-SMF    component branch : vpp-upf
OAI-SPGW-U component branch : master
---------------------------------------------------------
....
$ docker build --target oai-amf --tag oai-amf:production \
               --file component/oai-amf/docker/Dockerfile.ubuntu.18.04 \
               --build-arg NEEDED_GIT_PROXY="http://proxy.eurecom.fr:8080" \
               component/oai-amf
$ docker build --target oai-smf --tag oai-smf:production \
               --file component/oai-smf/docker/Dockerfile.ubuntu.18.04 \
               --build-arg NEEDED_GIT_PROXY="http://proxy.eurecom.fr:8080" \
               component/oai-smf

```
* Build vpp-upf
```bash
$ git clone https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-upf-vpp.git
$ cd oai-cn5g-upf-vpp
$ git checkout vpp-upf
$ docker build --target vpp-upg --tag vpp-upg:latest \
               --file docker/Dockerfile.ubuntu.18.04 \
               --build-arg EURECOM_PROXY="http://proxy.eurecom.fr:8080" . 
```
* Clean intermediate containers -
```bash
docker image prune --force
```


## Deploy oai-cn5g docker

Refer this page -> [oai-cn5g-fed](https://gitlab.eurecom.fr/oai/cn5g/oai-cn5g-fed/-/blob/master/docs/BUILD_IMAGES.md) to deploy oai-cn5g components.<br/>
or <br/>
Refer oai-cn5g-fed [sample docker-compose.](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/docker/docker-compose.yaml)<br/>
`cd docker/` 
* `docker-compose -f docker/docker-compose-vpp-upf.yaml up -d mysql`   [Mysql logs](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/logs/mysql_log.txt) 
* `docker-compose -f docker/docker-compose-vpp-upf.yaml up -d vpp-upf` [VPP-UPF logs](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/logs/vpp_upf_log.txt)
* `docker-compose -f docker/docker-compose-vpp-upf.yaml up -d oai-smf` [SMF logs](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/logs/smf_upf_log.txt)
* `docker-compose -f docker/docker-compose-vpp-upf.yaml up -d oai-amf` [AMF logs](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/logs/amf_upf_log.txt)
* `docker-compose -f docker/docker-compose-vpp-upf.yaml up -d oai-nat` [DN logs](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/logs/nat_upf_log.txt)


### Run gnbsim 

* `docker-compose up -d gnbsim`

### Test traffic to UE

* Ping to UE from nat container
```bash
$ docker exec -it oai-nat ping 12.1.1.2
 PING 12.1.1.2 (12.1.1.2) 56(84) bytes of data.
64 bytes from 12.1.1.2: icmp_seq=358 ttl=63 time=865 ms
64 bytes from 12.1.1.2: icmp_seq=359 ttl=63 time=0.755 ms
64 bytes from 12.1.1.2: icmp_seq=360 ttl=63 time=0.772 ms
64 bytes from 12.1.1.2: icmp_seq=361 ttl=63 time=0.833 ms
^C
--- 12.1.1.2 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 39 ms
rtt min/avg/max/mdev = 0.247/33.961/865.616/166.331 ms
```
* Iperf test
Run server on UE or DN. Here server is  running on DN.
```bash
$ docker exec -it oai-nat iperf3 -s -B 192.168.76.205
-----------------------------------------------------------
Server listening on 5201
-----------------------------------------------------------
Accepted connection from 12.1.1.2, port 54695
[  5] local 192.168.74.205 port 5201 connected to 192.168.74.199 port 59061
[ ID] Interval           Transfer     Bandwidth
[  5]   0.00-1.00   sec  78.5 MBytes   658 Mbits/sec                  
[  5]   1.00-2.00   sec  76.9 MBytes   645 Mbits/sec                  
[  5]   2.00-3.00   sec  76.9 MBytes   645 Mbits/sec                  
[  5]   3.00-4.00   sec  78.2 MBytes   656 Mbits/sec                  
[  5]   4.00-5.00   sec  75.0 MBytes   629 Mbits/sec                  
[  5]   5.00-6.00   sec  76.6 MBytes   643 Mbits/sec                  
[  5]   6.00-7.00   sec  67.9 MBytes   570 Mbits/sec                  
[  5]   7.00-8.00   sec  67.9 MBytes   569 Mbits/sec                  
[  5]   8.00-9.00   sec  72.4 MBytes   607 Mbits/sec                  
[  5]   9.00-10.00  sec  70.8 MBytes   594 Mbits/sec                  
[  5]  10.00-10.00  sec   161 KBytes   412 Mbits/sec                  
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bandwidth
[  5]   0.00-10.00  sec  0.00 Bytes  0.00 bits/sec                  sender
[  5]   0.00-10.00  sec   741 MBytes   622 Mbits/sec                  receiver
-----------------------------------------------------------

```
Run client on gnbsim UE address
```bash
$ docker exec -it gnbsim iperf3 -c 192.168.76.205 -B 12.1.1.2     
Connecting to host 192.168.76.205, port 5201
[  5] local 12.1.1.2 port 59061 connected to 192.168.76.205 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec  79.5 MBytes   667 Mbits/sec  100    212 KBytes       
[  5]   1.00-2.00   sec  77.0 MBytes   646 Mbits/sec   88    164 KBytes       
[  5]   2.00-3.00   sec  76.7 MBytes   644 Mbits/sec   78    174 KBytes       
[  5]   3.00-4.00   sec  78.5 MBytes   658 Mbits/sec   77    153 KBytes       
[  5]   4.00-5.00   sec  75.2 MBytes   631 Mbits/sec   30    184 KBytes       
[  5]   5.00-6.00   sec  76.3 MBytes   640 Mbits/sec   84    133 KBytes       
[  5]   6.00-7.00   sec  68.2 MBytes   572 Mbits/sec   52    167 KBytes       
[  5]   7.00-8.00   sec  67.8 MBytes   569 Mbits/sec   74    188 KBytes       
[  5]   8.00-9.00   sec  72.3 MBytes   606 Mbits/sec  146    168 KBytes       
[  5]   9.00-10.00  sec  70.8 MBytes   594 Mbits/sec   34    156 KBytes       
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-10.00  sec   742 MBytes   623 Mbits/sec  763             sender
[  5]   0.00-10.00  sec   741 MBytes   622 Mbits/sec                  receiver

```
## Current status 
### Procedure tested
* gNB registration  
   -  NGSetup_Req/Resp
* UE registration
   -  NAS Auth req/resp IA_2
   -  Initial context setup req/resp
   -  UL NAS registration complete
* PDU session
   -  PDU session extablishment request
   -  PFCP session extablishment req/resp
   -  PDU session resource setup req + PDU session extablishment accept  
   -  PDU session resource setup resp
   -  PFCP session modification req/resp
   -  GTP UL/DL flow (with/without gtp extension header)
* UE deregistration

### Procedure pending
   -  PDU session release

## Troubleshoot
* Remove tunnel interface <br/>
`ip tuntap del mode tun gtp-gnb`

* Log files and [setup diagram](https://gitlab.eurecom.fr/kharade/gnbsim/-/blob/master/logs/gnbsim_oai_5gcn.png) are [located here.](https://gitlab.eurecom.fr/kharade/gnbsim/-/tree/master/logs)
