# UE
As described in [link](https://open-cells.com/index.php/uiccsim-programing/)
## Check existing values of the sim
`sudo ./program_uicc`

## Ensure PLMN consistency
* Check consistency for `MCC` and `MNC` in the following places
	* <GnodeB repo> `ci-scripts/yaml_files/sa_b200_gnb/docker-compose.yml` [Sample](https://github.com/subhrendu1987/oai-gnodeb/blob/main/ci-scripts/yaml_files/sa_b200_gnb/docker-compose.yml)
	* <In Core VM repo> `docker-compose/docker-compose-basic-nrf.yaml` [Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/docker-compose-basic-nrf.yaml)
	* <In Core VM repo> `docker-compose/database/oai_db3.sql` [Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/database/oai_db3.sql)

## Ensure operator key consistency
* Get the operator key from <Core VM repo> using the following command `cat  docker-compose-basic-nrf.yaml |grep "OPERATOR_KEY"`
[Sample](https://github.com/subhrendu1987/oai-core/blob/main/docker-compose/docker-compose-basic-nrf.yaml) and use it as `<OperatorKeyfrom_OAI-AMF>` in the next step

## Start wrting to SIM
`sudo ./program_uicc --adm <PrintedOnSIM> --imsi <MCC><MNC>0100001101 --isdn 00000001 --acc 0001 --key 6874736969202073796d4b2079650a73 --opc <OperatorKeyfrom_OAI-AMF> -spn "OpenCells01" --authenticate --noreadafter`

An example is given below
`sudo ./program_uicc --adm 0c008508 --imsi 001010000000011 --key 6874736969202073796d4b2079650a73 --opc 504f20634f6320504f50206363500a4f -spn "OpenCells01" --authenticate`

## ADD SIM info in the OAI-core
* Find "AuthenticationSubscription" Table in CORE/docker-compose/oai_db3.sql and add a new value as 
(`ueid=UE.IMSI`, `authenticationMethod `, `encPermanentKey=UE.key`, `protectionParameterId=UE.key`, `sequenceNumber`, `authenticationManagementField`, `algorithmId`, `encOpcKey=OperatorKey_OAI-AMF`, `encTopcKey`, `vectorGenerationInHss`, `n5gcAuthMethod`, `rgAuthenticationInd`, `supi=UE.IMSI`)

* Find "SessionManagementSubscriptionData" Table in CORE/docker-compose/oai_db3.sql and add a new value as 
(`ueid=UE.IMSI`, `servingPlmnid=UE.MCC_MNC`, `singleNssai=SST,SD`, `dnnConfigurations`)


* APN setup
```
Name: <Choice>
APN: OpenCells01 # SPN

```
# 5G Glossary
```
MCC: Mobile country code
MNC: Mobile area code
ADM: 
ISDN:
SPN:
OPC:

```