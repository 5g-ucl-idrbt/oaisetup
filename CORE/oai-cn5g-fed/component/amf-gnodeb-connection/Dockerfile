FROM ubuntu:bionic


RUN apt-get update
RUN apt install libsctp-dev make gcc net-tools -y
RUN apt install iputils-ping -y
RUN apt install lksctp-tools -y
# Copying source code
WORKDIR /home
COPY sctp_server /home
RUN echo "rebuild from here"
RUN make


# expose ports
EXPOSE 80/tcp 9090/tcp 38412/sctp

#ENTRYPOINT ["/bin/bash","/openair-amf/bin/entrypoint.sh"]
#CMD ["/openair-amf/bin/oai_amf", "-c", "/openair-amf/etc/amf.conf", "-o"]

RUN echo "sudo docker run --rm -it --network="host" --entrypoint bash sctptest"
ENTRYPOINT ["/bin/bash"]
