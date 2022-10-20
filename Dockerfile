FROM ubuntu:18.04

RUN apt-get update && apt-get -y --no-install-recommends install \
    build-essential \
    gcc \
    g++ \
    cmake \
    gdb \
    wget