FROM ubuntu:bionic


RUN apt-get update
RUN apt install libsctp-dev make gcc net-tools -y
RUN apt install iputils-ping netcat -y
RUN apt install lksctp-tools -y
# Copying source code
WORKDIR /home
COPY sctp_server /home
RUN echo "rebuild from here"
RUN make


# expose ports
EXPOSE 80/tcp 9090/tcp 38412/sctp
COPY scripts /home/
RUN echo "sudo docker run --rm -it --network="host" --entrypoint bash sctptest"

ENTRYPOINT ["/bin/bash","/openair-amf/bin/entrypoint.sh"]
