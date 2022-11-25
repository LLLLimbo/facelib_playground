FROM ubuntu:18.04

RUN apt-get update
RUN apt-get -y --no-install-recommends install build-essential gcc curl g++ gdb wget libssl-dev git ca-certificates libcurl4-gnutls-dev libtiff-dev libjpeg-dev

WORKDIR /seeiner
COPY include ./facelib-playground/include
COPY os ./facelib-playground/os
COPY src ./facelib-playground/src
COPY CMakeLists.txt ./facelib-playground/CMakeLists.txt
COPY conf.json ./facelib-playground/conf.json
RUN cp -r ./facelib-playground/os/cpu/ubuntu18/usr/lib/. /usr/lib

#install cmake 3.16.3
WORKDIR /seeiner
RUN wget https://cmake.org/files/v3.16/cmake-3.16.3.tar.gz --no-check-certificate
RUN tar xf cmake-3.16.3.tar.gz
WORKDIR /seeiner/cmake-3.16.3
RUN /bin/bash -c './configure'
RUN make
RUN make install
RUN hash -r

#install nats.c
WORKDIR /seeiner
RUN git clone https://github.91chi.fun/https://github.com/nats-io/nats.c.git
WORKDIR /seeiner/nats.c/build
RUN cmake .. -DNATS_BUILD_WITH_TLS=OFF -DNATS_BUILD_STREAMING=OFF
RUN make install
RUN cp src/libnats.so /seeiner/facelib-playground/os/cpu/ubuntu18/usr/lib/
RUN cp src/libnats.so.* /seeiner/facelib-playground/os/cpu/ubuntu18/usr/lib/

#install libcurl
WORKDIR /seeiner
RUN wget https://curl.se/download/curl-7.85.0.tar.gz && tar xf curl-7.85.0.tar.gz
WORKDIR /seeiner/curl-7.85.0
RUN ./configure --without-ssl --prefix=/seeiner/curl-7.85.0/build
RUN make
RUN make install
RUN cp build/lib/libcurl.so /seeiner/facelib-playground/os/cpu/ubuntu18/usr/lib/
RUN cp build/lib/libcurl.so.* /seeiner/facelib-playground/os/cpu/ubuntu18/usr/lib/

WORKDIR /seeiner/facelib-playground/build
RUN cmake ..
RUN make
RUN mv facelib_playgroundtest ../
WORKDIR /seeiner/facelib-playground
ENTRYPOINT ["/bin/bash","-c","./facelib_playgroundtest"]