FROM ubuntu:bionic

# Copying source code
WORKDIR /home
COPY sctp_server /home

RUN apt-get update
RUN apt install libsctp-dev make gcc net-tools -y

RUN make


# expose ports
EXPOSE 80/tcp 9090/tcp 38412/sctp

#ENTRYPOINT ["/bin/bash","/openair-amf/bin/entrypoint.sh"]
#CMD ["/openair-amf/bin/oai_amf", "-c", "/openair-amf/etc/amf.conf", "-o"]

ENTRYPOINT ["/bin/bash"]