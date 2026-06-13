FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    gcc \
    g++ \
    cmake \
    python3 \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

RUN git clone https://gitlab.com/nsnam/ns-3-dev.git ns-3

WORKDIR /opt/ns-3

COPY src/routing-sim.cc scratch/routing-sim.cc

RUN ./ns3 configure
RUN ./ns3 build

COPY run.sh /opt/ns-3/run.sh

RUN chmod +x /opt/ns-3/run.sh

ENTRYPOINT ["./run.sh"]
