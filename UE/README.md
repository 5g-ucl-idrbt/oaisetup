# UE
As described in [link](https://open-cells.com/index.php/uiccsim-programing/)
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
# Terminologies used here
```
MCC: Mobile country code
MNC: Mobile area code
ADM: 
ISDN:
SPN:
OPC:
PLMN:
IMSI:
SUPI:
APN:
```