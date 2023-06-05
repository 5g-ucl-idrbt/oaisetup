# Open Air Interface (OAI) setup in full virtualized environment
Consists of three steps; (a) GnodeB (b)Core and (c) UE preparations
Setup repositories are linked below.
## GnodeB
Moved to [https://github.com/subhrendu1987/oai-gnodeb](https://github.com/subhrendu1987/oai-gnodeb)
## Core
Moved to [https://github.com/subhrendu1987/oai-core](https://github.com/subhrendu1987/oai-core)
## UE preparations
Check UE folder of this repository

# Execute
Once the setup is complete spin-up Core first, and then start GnodeB. Now load the SIM in the mobile and power it up. 
Select NR only in the cell phone.
## Add APN
1. Name: oai.ipv4
2. MCC: 001
3. MNC: 01
4. Type: default
5. APN Protocol: ipv4
6. APN Roaming Protocol: ipv4
7. APN: enabled
## HTC One Plus


## Google pixel
Call `*#*#4636##` and set `NR Only`
