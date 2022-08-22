#!/bin/bash
RED='\033[0;31m'
NC='\033[0m' # No Color

docker pull oaisoftwarealliance/oai-amf:v1.4.0
print "${RED}Downloaded oai-amf:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-nrf:v1.4.0
print "${RED}Downloaded oai-nrf:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-spgwu-tiny:v1.4.0
print "${RED}Downloaded oai-spgwu-tiny:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-smf:v1.4.0
print "${RED}Downloaded oai-smf:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-udr:v1.4.0
print "${RED}Downloaded oai-udr:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-udm:v1.4.0
print "${RED}Downloaded oai-udm:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-ausf:v1.4.0
print "${RED}Downloaded oai-ausf:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-upf-vpp:v1.4.0
print "${RED}Downloaded oai-upf-vpp:v1.4.0${NC}"
docker pull oaisoftwarealliance/oai-nssf:v1.4.0
print "${RED}Downloaded oai-nssf:v1.4.0${NC}"
# Utility image to generate traffic
docker pull oaisoftwarealliance/trf-gen-cn5g:latest
print "${RED}Downloaded trf-gen-cn5g:latest${NC}"
print "Pull Completed"
