FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y wget && \
    wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.1-1_all.deb -O /tmp/cuda-keyring_1.1-1_all.deb && \
    dpkg -i /tmp/cuda-keyring_1.1-1_all.deb && \
    apt-get update

RUN apt-get -y install cuda-toolkit-11-8
RUN apt-get update && apt-get install -y build-essential git
