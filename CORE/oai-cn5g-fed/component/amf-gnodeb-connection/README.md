## Build
`sudo docker build -t sctptest .`
## Run
`docker run --rm -it --entrypoint bash sctptest`

## SCTP test
### Enable SCTP for telnet
`withsctp telnet <TargetIP> <TargetPort>`

## Docker termination
### Kill "Exited" Instances
`sudo docker ps -a |grep Exited |awk '{ print $1 }' > /tmp/out.tmp ; xargs --null sudo docker rm < <(tr \\n \\0 </tmp/out.tmp)`
### Terminate and remove an Instance
`sudo docker kill <InstanceName/ID>; sudo docker rm <InstanceName/ID>`
### Terminate and remove all Instances
`sudo docker kill $(sudo docker ps -q); sudo docker rm $(sudo docker ps -a -q)`

## Docker-compose
### Build
`sudo docker-compose -f docker-compose.yaml up -d`
### Kill
`sudo docker-compose -f docker-compose.yaml kill`
### Exec particular service
`docker-compose exec sctp-client bash`
`docker-compose exec sctp-server bash`
