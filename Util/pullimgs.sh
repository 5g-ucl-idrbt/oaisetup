#!/bin/bash
RED='\033[0;31m'
NC='\033[0m' # No Color

sudo docker pull mysql:5.7
echo -e "${RED} Downloaded mysql:5.7 ${NC}"
sudo docker pull oaisoftwarealliance/oai-amf:v1.4.0
echo -e "${RED} Downloaded oai-amf:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-nrf:v1.4.0
echo -e "${RED} Downloaded oai-nrf:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-spgwu-tiny:v1.4.0
echo -e "${RED} Downloaded oai-spgwu-tiny:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-smf:v1.4.0
echo -e "${RED} Downloaded oai-smf:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-udr:v1.4.0
echo -e "${RED} Downloaded oai-udr:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-udm:v1.4.0
echo -e "${RED} Downloaded oai-udm:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-ausf:v1.4.0
echo -e "${RED} Downloaded oai-ausf:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-upf-vpp:v1.4.0
echo -e "${RED} Downloaded oai-upf-vpp:v1.4.0 ${NC}"
sudo docker pull oaisoftwarealliance/oai-nssf:v1.4.0
echo -e "${RED} Downloaded oai-nssf:v1.4.0 ${NC}"
# Utility image to generate traffic
sudo docker pull oaisoftwarealliance/trf-gen-cn5g:latest
echo -e "${RED} Downloaded trf-gen-cn5g:latest ${NC}"
echo -e "Pull Completed"
