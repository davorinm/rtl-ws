# Build image:
#   docker build -t build_system -f Dockerfile .
# Run image:
#   docker run --rm -ti -v $(pwd):/project build_system
# Stop image
#   docker stop -t build_system

FROM ubuntu:22.04

RUN apt update && \
	apt upgrade -y && \
	apt install -y wget && \
	apt install -y build-essential cmake

WORKDIR /usr/local/bin
RUN wget http://releases.linaro.org/components/toolchain/binaries/7.2-2017.11/arm-linux-gnueabihf/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf.tar.xz
RUN tar -xf gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf.tar.xz
ENV PATH=$PATH:/usr/local/bin/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf/bin

WORKDIR /project
