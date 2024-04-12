# UE
We have used usim provided by opencells[^1] in our project. The SIM editing procedure is available in [link](https://open-cells.com/index.php/uiccsim-programing/)

[^1]: https://open-cells.com/index.php/sim-cards/
## Check existing values of the sim
`sudo ./program_uicc`

## Ensure PLMN consistency
* Check consistency for `MCC` and `MNC` in the following files.
	* <GnodeB repo> `ci-scripts/yaml_files/sa_b200_gnb/docker-compose.yml` [Sample](https://github.com/subhrendu1987/oai-gnodeb/blob/main/ci-scripts/yaml_files/sa_b200_gnb/docker-compose.yml)
	* <Core VM repo> `docker-compose/docker-compose-basic-nrf.yaml` [Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/docker-compose-basic-nrf.yaml)
	* <Core VM repo> `docker-compose/database/oai_db3.sql` [Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/database/oai_db3.sql)

Use the same values at the time of SIM writing

## Ensure operator key consistency
* Get the operator key from <Core VM repo> using the following command `cat  docker-compose-basic-nrf.yaml |grep "OPERATOR_KEY"`
[Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/docker-compose-basic-nrf.yaml) and use it as `<OperatorKeyfrom_OAI-AMF>` in the next step

## Start writing to SIM
* `sudo ./program_uicc --adm <PrintedOnSIM> --imsi <MCC><MNC>0100001101 --isdn 00000001 --acc 0001 --key 6874736969202073796d4b2079650a73 --opc <OperatorKeyfrom_OAI-AMF> -spn "OpenCells01" --authenticate --noreadafter`

* An example is given below
`sudo ./program_uicc --adm 0c008508 --imsi 001010000000011 --key 6874736969202073796d4b2079650a73 --opc 504f20634f6320504f50206363500a4f -spn "OpenCells01" --authenticate`

## ADD SIM info in the OAI-core
This parts should be executed in the Core VM

* Edit `docker-compose/database/oai_db3.sql` 
	* In "AuthenticationSubscription" Table [Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/database/oai_db3.sql#L154) add a new value as follows.
	(`ueid=UE.IMSI`, `authenticationMethod `, `encPermanentKey=UE.key`, `protectionParameterId=UE.key`, `sequenceNumber`, `authenticationManagementField`, `algorithmId`, `encOpcKey=OperatorKey_OAI-AMF`, `encTopcKey`, `vectorGenerationInHss`, `n5gcAuthMethod`, `rgAuthenticationInd`, `supi=UE.IMSI`)

	* In "SessionManagementSubscriptionData" Table [Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/database/oai_db3.sql#L309) add a new value as follows.
	(`ueid=UE.IMSI`, `servingPlmnid=UE.MCC_MNC`, `singleNssai=SST,SD`, `dnnConfigurations`)

Now restart the core to enable SIM registration.
## APN setup
After plugging the sim in use the following APN in your handset
```
Name: <Choice>
APN: OpenCells01 # SPN
```
### For Google Pixel phones
* Use the code `*#*#4636#*#*` to open the Testing configuration
* `Phone Information` > `Set preferred Network Type` = `NR only` (To save the changes left swipe)
* `Phone Information` > `...` (i.e. Settings icon in top right) > `Select Radio Band` > `Europe`

### If UE is connected, but not connecting to the Internet
Check the oai-core logs using 
```
sudo docker -f docker-compose-basic-nrf.yam logs --follow
```
An error related to SMF means some issue in APN settings. To fix that, use the following steps.
* In `oai_db3.sql` [file](https://github.com/5g-ucl-idrbt/oai-core/blob/main/docker-compose/database/oai_db3.sql)
  	- Find the values of `SST` and `SD`. e.g.
  	- <pre><p>
  	  INSERT INTO `SessionManagementSubscriptionData` (`ueid`, `servingPlmnid`, `singleNssai`, `dnnConfigurations`) VALUES<br> ('001010000000037', '00101', <b>'{\"sst\": 1, \"sd\": \"1\"}'</b>,<br>'{\"default\":{\"pduSessionTypes\":{ \"defaultSessionType\": \"IPV4\"},\"sscModes\": {\"defaultSscMode\": \"SSC_MODE_1\"},\"5gQosProfile\": {\"5qi\": 6,\"arp\":{\"priorityLevel\": 1,\"preemptCap\": \"NOT_PREEMPT\",\"preemptVuln\":\"NOT_PREEMPTABLE\"},\"priorityLevel\":1},\"sessionAmbr\":{\"uplink\":\"100Mbps\", \"downlink\":\"100Mbps\"},\"staticIpAddress\":[{\"ipv4Addr\": \"12.1.1.10\"}]}}');
	</p></pre>
 * In `docker-compose` [file](https://github.com/5g-ucl-idrbt/oai-core/blob/main/docker-compose/docker-compose-basic-nrf.yaml)
   	- Check `SST` and `SD` values in `oai-amf` service
   	- <pre><p>
		oai-amf:
	        container_name: "oai-amf"
	        image: oai-amf:v1.4.0
	        environment:
	            - TZ=Asia/India
	            - INSTANCE=0
	            - PID_DIRECTORY=/var/run
	            - MCC=001
	            - MNC=01
	            - REGION_ID=128
	            - AMF_SET_ID=1
	            - SERVED_GUAMI_MCC_0=001
	            - SERVED_GUAMI_MNC_0=01 # 95
	            - SERVED_GUAMI_REGION_ID_0=128
	            - SERVED_GUAMI_AMF_SET_ID_0=1
	            - SERVED_GUAMI_MCC_1=001  # 460
	            - SERVED_GUAMI_MNC_1=01  # 11
	            - SERVED_GUAMI_REGION_ID_1=10
	            - SERVED_GUAMI_AMF_SET_ID_1=1
	            - PLMN_SUPPORT_MCC=001
	            - PLMN_SUPPORT_MNC=01
	            - PLMN_SUPPORT_TAC=1 #0xa000 to connect gNB
	            - SST_0=1
	            - SD_0=1  #0xFFFFFF
	           <b> - SST_1=1
	            - SD_1=1 </b>
	            - SST_2=222
	            - SD_2=123 
	            - AMF_INTERFACE_NAME_FOR_NGAP=eth0
	            - AMF_INTERFACE_NAME_FOR_N11=eth0
	            - SMF_INSTANCE_ID_0=1
	            - SMF_FQDN_0=oai-smf
	            - SMF_IPV4_ADDR_0=192.168.70.133
	            - SMF_HTTP_VERSION_0=v1
	            - SELECTED_0=true
	            - SMF_INSTANCE_ID_1=2
	            - SMF_FQDN_1=oai-smf
	            - SMF_IPV4_ADDR_1=0.0.0.0
	            - SMF_HTTP_VERSION_1=v1
	            - SELECTED_1=false
	            - MYSQL_SERVER=192.168.70.131
	            - MYSQL_USER=root
	            - MYSQL_PASS=linux
	            - MYSQL_DB=oai_db
	            - OPERATOR_KEY=504f20634f6320504f50206363500a4f
	            - NRF_IPV4_ADDRESS=192.168.70.130
	            - NRF_PORT=80
	            - EXTERNAL_NRF=no
	            - NF_REGISTRATION=yes
	            - SMF_SELECTION=yes
	            - USE_FQDN_DNS=yes
	            - EXTERNAL_AUSF=yes
	            - EXTERNAL_UDM=no
	            - EXTERNAL_NSSF=no
	            - USE_HTTP2=no
	            - NRF_API_VERSION=v1
	            - NRF_FQDN=oai-nrf
	            - AUSF_IPV4_ADDRESS=192.168.70.138
	            - AUSF_PORT=80
	            - AUSF_API_VERSION=v1
	            - AUSF_FQDN=oai-ausf
	            - UDM_IPV4_ADDRESS=192.168.70.137
	            - UDM_PORT=80
	            - UDM_API_VERSION=v2
	            - UDM_FQDN=oai-udm
	            - HTTP_PROXY=${http_proxy}
	            - HTTPS_PROXY=${https_proxy}
	</p></pre>
 * In `docker-compose` [file](https://github.com/5g-ucl-idrbt/oai-core/blob/main/docker-compose/docker-compose-basic-nrf.yaml)
   	- Check `DNN_NI` in `oai-smf` service. (Here the `DNN_NI` Name should be the `APN`)
   	- <pre><p>
		oai-smf:
	        container_name: "oai-smf"
	        image: oai-smf:v1.4.0
	        environment:
	            - TZ=Asia/India
	            - INSTANCE=0
	            - PID_DIRECTORY=/var/run
	            - SMF_INTERFACE_NAME_FOR_N4=eth0
	            - SMF_INTERFACE_NAME_FOR_SBI=eth0
	            - SMF_INTERFACE_PORT_FOR_SBI=80
	            - SMF_INTERFACE_HTTP2_PORT_FOR_SBI=9090
	            - SMF_API_VERSION=v1
	            - DEFAULT_DNS_IPV4_ADDRESS=172.21.3.100
	            - DEFAULT_DNS_SEC_IPV4_ADDRESS=8.8.8.8
	            - AMF_IPV4_ADDRESS=192.168.70.132
	            - AMF_PORT=80
	            - AMF_API_VERSION=v1
	            - AMF_FQDN=oai-amf
	            - UDM_IPV4_ADDRESS=192.168.70.137
	            - UDM_PORT=80
	            - UDM_API_VERSION=v2
	            - UDM_FQDN=oai-udm
	            - UPF_IPV4_ADDRESS=192.168.70.134
	            - UPF_FQDN_0=oai-spgwu
	            - NRF_IPV4_ADDRESS=192.168.70.130
	            - NRF_PORT=80
	            - NRF_API_VERSION=v1
	            - USE_LOCAL_SUBSCRIPTION_INFO=yes  #Set to yes if SMF uses local subscription information instead of from an UDM
	            - USE_NETWORK_INSTANCE=no  #Set yes if network instance is to be used for given UPF
	            - NRF_FQDN=oai-nrf
	            - REGISTER_NRF=yes
	            - DISCOVER_UPF=yes
	            - USE_FQDN_DNS=yes
	            - HTTP_VERSION=1        # Default: 1
	            - UE_MTU=1500
	            - DNN_NI0=oai </b>
	            - TYPE0=IPv4
	            - DNN_RANGE0=12.1.1.151 - 12.1.1.253
	            - NSSAI_SST0=1
	            - NSSAI_SD0=0xFFFFFF
	            - SESSION_AMBR_UL0=200Mbps
	            - SESSION_AMBR_DL0=400Mbps
	            <b>- DNN_NI1=oai.ipv4</b>
	            - TYPE1=IPv4
	            - DNN_RANGE1=12.1.1.51 - 12.1.1.150
	            - NSSAI_SST1=1
	            - NSSAI_SD1=1
	            - SESSION_AMBR_UL1=100Mbps
	            - SESSION_AMBR_DL1=200Mbps
	            - DNN_NI2=default
	            - TYPE2=IPv4
	            - DNN_RANGE2=12.1.1.2 - 12.1.1.50
	            - NSSAI_SST2=1  # 222
	            - NSSAI_SD2=1  # 123
	            - SESSION_AMBR_UL2=50Mbps
	            - SESSION_AMBR_DL2=100Mbps
	            - DNN_NI3=oai.ipv4  # ims
	            - TYPE3=IPv4  #IPv4v6
	            - DNN_RANGE3=14.1.1.2 - 14.1.1.253
	            - DEFAULT_CSCF_IPV4_ADDRESS=127.0.0.1  # only needed when ims is being used
	            - ENABLE_USAGE_REPORTING=no # Set yes if UE USAGE REPORTING is to be done at UPF
	            - HTTP_PROXY=${http_proxy}
	            - HTTPS_PROXY=${https_proxy}
	</p></pre>
 



# Terminologies used here
```
MCC:	Mobile country code:						[Ref](https://en.wikipedia.org/wiki/Mobile_country_code)
MNC:	Mobile area code:							[Ref](https://en.wikipedia.org/wiki/Mobile_country_code)
ADM:	Administrative Key
ISDN:	Integrated Service Digital Network:			[Ref](https://en.wikipedia.org/wiki/Integrated_Services_Digital_Network)
SPN:	Service Provider Name:
OPC:	Operator Code:
PLMN:	Public Land Mobile Network:					[Ref](https://en.wikipedia.org/wiki/Public_land_mobile_network)
IMSI:	International Mobile Subscriber Identity:	[Ref](https://en.wikipedia.org/wiki/International_mobile_subscriber_identity)
SUPI:	Subscription Permanent Identifier:			[Ref](https://www.techplayon.com/5g-identifiers-supi-and-suci/)
APN:	Access Point Name:							[Ref](https://en.wikipedia.org/wiki/Access_Point_Name)
```
	
## eSIM configuration

eSim QR contains the following information
1. Activation Code: A unique code that identifies the eSIM activation process. It is used to link the eSIM profile to your device.
1. SM-DP+ Address: The address of the Subscription Manager Data Preparation (SM-DP+) server. This server is responsible for securely managing the eSIM profiles and provisioning them onto your device.
1. SM-DP+ Protocol: The protocol used to communicate with the SM-DP+ server, such as HTTPS or another secure protocol.
1. Confirmation Code: A code that may be required during the eSIM activation process to validate your ownership of the eSIM.
1. ICCID (Integrated Circuit Card Identifier): A unique identifier for the eSIM. It is used to identify the eSIM when connecting to the cellular network.
1. Operator Name: The name of the mobile network operator associated with the eSIM profile.
1. Profile Name: A name or description assigned to the eSIM profile, often indicating the mobile network operator or a specific plan.
1. Other Network-specific Information: Depending on the carrier and eSIM implementation, the QR code may contain additional information specific to the network, such as authentication keys, network settings, and access credentials.
When you scan the eSIM QR code using your device's camera or enter the data manually, your device uses this information to establish a connection with the SM-DP+ server, retrieve the eSIM profile, and activate the eSIM on your device. The specific details contained within the eSIM QR code may vary depending on the carrier, device, and eSIM implementation being used.





