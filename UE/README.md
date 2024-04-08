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
### If UE is connected, but not connecting to the Internet
Check the oai-core logs using 
```
sudo docker -f docker-compose-basic-nrf.yam logs --follow
```
An error related to SMF means some issue in APN settings. To fix that, use the following steps.
* In `oai_db3.sql` [file](https://github.com/5g-ucl-idrbt/oai-core/blob/main/docker-compose/database/oai_db3.sql)
  	- ```
  	  INSERT INTO `SessionManagementSubscriptionData` (`ueid`, `servingPlmnid`, `singleNssai`, `dnnConfigurations`) VALUES 
('001010000000037', '00101', '{\"sst\": 1, \"sd\": \"1\"}','{\"default\":{\"pduSessionTypes\":{ \"defaultSessionType\": \"IPV4\"},\"sscModes\": {\"defaultSscMode\": \"SSC_MODE_1\"},\"5gQosProfile\": {\"5qi\": 6,\"arp\":{\"priorityLevel\": 1,\"preemptCap\": \"NOT_PREEMPT\",\"preemptVuln\":\"NOT_PREEMPTABLE\"},\"priorityLevel\":1},\"sessionAmbr\":{\"uplink\":\"100Mbps\", \"downlink\":\"100Mbps\"},\"staticIpAddress\":[{\"ipv4Addr\": \"12.1.1.10\"}]}}');
	```


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





