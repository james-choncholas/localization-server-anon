FROM ubuntu:20.04

RUN apt update && apt upgrade -y && apt install -y sudo
RUN DEBIAN_FRONTEND=noninteractive apt install -y g++ make cmake libgmp-dev libssl-dev libboost-all-dev

RUN DEBIAN_FRONTEND=noninteractive apt install -y software-properties-common cmake git build-essential libssl-dev xxd

COPY ./ ./

RUN mkdir build && cd build
RUN cmake ..
RUN make
