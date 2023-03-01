# UE
https://open-cells.com/index.php/uiccsim-programing/
## Check existing values of the sim
`sudo ./program_uicc`

## Ensure operator key consistency
`<In CORE> cat  docker-compose-basic-nrf.yaml |grep "OPERATOR_KEY"`

`MCC` and `MNC` gnodeB/target/PROJECTS.../, CORE/docker-compose/ .yaml file, CORE/docker-compose/.db

`sudo ./program_uicc --adm <PrintedOnSIM> --imsi <MCC><MNC>0100001101 --isdn 00000001 --acc 0001 --key 6874736969202073796d4b2079650a73 --opc <OperatorKeyfrom_OAI-AMF> -spn "OpenCells01" --authenticate --noreadafter`

`sudo ./program_uicc --adm 0c008508 --imsi 001010000000011 --key 6874736969202073796d4b2079650a73 --opc 504f20634f6320504f50206363500a4f -spn "OpenCells01" --authenticate`

* Find "AuthenticationSubscription" Table in CORE/docker-compose/oai_db2.sql and add a new value as 
(`ueid=UE.IMSI`, `authenticationMethod `, `encPermanentKey=UE.key`, `protectionParameterId=UE.key`, `sequenceNumber`, `authenticationManagementField`, `algorithmId`, `encOpcKey=OperatorKey_OAI-AMF`, `encTopcKey`, `vectorGenerationInHss`, `n5gcAuthMethod`, `rgAuthenticationInd`, `supi=UE.IMSI`)

* Find "SessionManagementSubscriptionData" Table in CORE/docker-compose/oai_db2.sql and add a new value as 
(`ueid=UE.IMSI`, `servingPlmnid=UE.MCC_MNC`, `singleNssai=SST,SD`, `dnnConfigurations`)


* APN setup
```
Name: <Choice>
APN: OpenCells01 # SPN

```